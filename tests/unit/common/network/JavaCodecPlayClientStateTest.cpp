#include "common/BaseTestServer.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/item/Items.hpp"
#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/mappings/JavaCustomStatIds.hpp"
#include "common/network/backend/java/mappings/JavaDamageTypeIds.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/command/commands/DialogCommand.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/handshake/RegistryDataBuilder.hpp"
#include "server/network/outbound/PacketBuilders.hpp"
#include "server/network/sync/chunk/PlayerChunkTracker.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/stats/StatisticsManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <limits>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

TEST_F(NetworkTestBase, PlayTabListComponentsHaveNoOuterLengthPrefix)
{
    const TabList packet{text::plainTextToNbtBytes("H"), text::plainTextToNbtBytes("F")};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{120, 8, 0, 1, 'H', 8, 0, 1, 'F'}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<TabList>(decoded.value()), packet);
    auto rich = packet;
    rich.header = text::parseJsonComponentToNbtBytes(R"({"text":"Header","bold":true,"color":"red"})");
    EXPECT_EQ(std::get<TabList>(roundTripGeneric(*tables()->playCb, PlayPacket{rich})), rich);
    for (std::size_t size = 1; size < buf.size(); ++size) {
        buffer::RegistryByteBuf truncated(buf.data(), size, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(truncated).success());
    }
}

TEST_F(LocalServerFixture, ServerPlayerSendsAndClearsTabList)
{
    serverConn()->setPhase(protocol::ConnectionProtocol::Play);
    ServerPlayer player(1, "Player", mc::test::testEcsRegistry());
    player.setConnection(serverConn());
    const text::StringTextComponent header("Header");
    const text::StringTextComponent footer("Footer");
    ASSERT_TRUE(player.sendTabList(header, footer));
    auto received = pumpClient();
    ASSERT_TRUE(received.has_value());
    const auto& packet = std::get<TabList>(std::get<PlayPacket>(received->packet));
    EXPECT_EQ(packet.header, text::plainTextToNbtBytes("Header"));
    EXPECT_EQ(packet.footer, text::plainTextToNbtBytes("Footer"));
    const text::StringTextComponent empty("");
    ASSERT_TRUE(player.sendTabList(empty, empty));
    received = pumpClient();
    ASSERT_TRUE(received.has_value());
    const auto& cleared = std::get<TabList>(std::get<PlayPacket>(received->packet));
    EXPECT_EQ(cleared.header, text::plainTextToNbtBytes(""));
    EXPECT_EQ(cleared.footer, text::plainTextToNbtBytes(""));
}

TEST_F(NetworkTestBase, PlayCooldownUsesGroupIdentifierAndVarIntDuration)
{
    const Cooldown packet{"test:shared", 300};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{22, 11, 't', 'e', 's', 't', ':', 's', 'h', 'a', 'r', 'e', 'd', 0xac, 2}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<Cooldown>(decoded.value()), packet);
    for (const Cooldown invalid : {Cooldown{"", 1}, Cooldown{"test:shared", -1}}) {
        auto malformed = makeBoundBuf();
        ASSERT_TRUE(tables()->playCb->encode(malformed, PlayPacket{invalid}).success());
        EXPECT_FALSE(tables()->playCb->decode(malformed).success());
    }
}

TEST_F(LocalServerFixture, ServerPlayerNotifiesCooldownStartExpiryAndRemoval)
{
    serverConn()->setPhase(protocol::ConnectionProtocol::Play);
    ServerPlayer player(1, "Player", mc::test::testEcsRegistry());
    player.setConnection(serverConn());
    const ResourceLocation group("test:shared");
    player.cooldownTracker().setCooldown(group, 2);
    auto received = pumpClient();
    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(std::get<Cooldown>(std::get<PlayPacket>(received->packet)), (Cooldown{"test:shared", 2}));
    player.cooldownTracker().tick();
    EXPECT_FALSE(pumpClient().has_value());
    player.cooldownTracker().tick();
    received = pumpClient();
    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(std::get<Cooldown>(std::get<PlayPacket>(received->packet)), (Cooldown{"test:shared", 0}));
    player.cooldownTracker().setCooldown(group, 20);
    ASSERT_TRUE(pumpClient().has_value());
    player.cooldownTracker().removeCooldown(group);
    received = pumpClient();
    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(std::get<Cooldown>(std::get<PlayPacket>(received->packet)), (Cooldown{"test:shared", 0}));
}

namespace {

class _DamageRecordingWorld : public mc::test::BaseTestWorld {
public:
    void broadcastDamageEvent(EntityInstanceId entityId, const DamageSource& source) override
    {
        events.emplace_back(entityId, source.type());
    }
    std::vector<std::pair<EntityInstanceId, DamageType>> events;
};

class _PacketRecordingConnection : public mc::test::FakeServerConnection {
public:
    Result<void> send(IrPacket packet) override
    {
        packets.push_back(std::move(packet));
        return Result<void>::ok();
    }
    std::vector<IrPacket> packets;
};

class _PositionedDamageSource : public EnvironmentalDamage {
public:
    _PositionedDamageSource()
        : EnvironmentalDamage(DamageType::Fall)
    {}
    std::optional<math::Vector3f> sourcePositionRaw() const override { return Vector3(1.0f, 2.0f, 3.0f); }
};

} // namespace

TEST_F(NetworkTestBase, PlayDamageEventUsesOptionalEntityOffsetsAndPosition)
{
    const DamageEvent empty{1, 10, -1, -1, std::nullopt};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{empty}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{25, 1, 10, 0, 0, 0}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<DamageEvent>(decoded.value()), empty);
    const DamageEvent positioned{300, 21, 0, 300, std::array<f64, 3>{1.0, -2.0, 3.5}};
    EXPECT_EQ(std::get<DamageEvent>(roundTripGeneric(*tables()->playCb, PlayPacket{positioned})), positioned);
    auto encoded = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(encoded, PlayPacket{positioned}).success());
    EXPECT_EQ(encoded.bytes()[4], 1);
    EXPECT_EQ(encoded.bytes()[5], 0xad);
    for (std::size_t size = 1; size < encoded.size(); ++size) {
        buffer::RegistryByteBuf truncated(encoded.data(), size, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(truncated).success());
    }
}

TEST_F(NetworkTestBase, DamageRegistryIdsMatchConfigurationOrder)
{
    const auto registries = server::net::buildConfigurationRegistryData();
    const auto registry = std::find_if(registries.begin(), registries.end(), [](const auto& value) {
        return value.registryKey == "minecraft:damage_type";
    });
    ASSERT_NE(registry, registries.end());
    ASSERT_EQ(registry->entries.size(), 50u);
    for (i32 value = 0; value <= static_cast<i32>(DamageType::GenericKill); ++value) {
        const auto type = static_cast<DamageType>(value);
        auto wire = backend::java::JavaDamageTypeIds::toWireId(type);
        ASSERT_TRUE(wire.success());
        EXPECT_EQ(registry->entries[static_cast<std::size_t>(wire.value())].id,
            DamageTypeNames::getResourceLocation(type).toString());
    }
    EXPECT_EQ(backend::java::JavaDamageTypeIds::toWireId(DamageType::Campfire).value(), 40);
    EXPECT_EQ(backend::java::JavaDamageTypeIds::toWireId(DamageType::Fall).value(), 10);
}

TEST_F(NetworkTestBase, FreshAcceptedDamageBroadcastsButInvulnerabilityFramesDoNot)
{
    _DamageRecordingWorld world;
    Player player(1, "Player", mc::test::testEcsRegistry());
    player.setWorld(&world);
    auto source = DamageSources::fall();
    ASSERT_TRUE(player.hurt(source, 1.0f));
    ASSERT_EQ(world.events.size(), 1u);
    EXPECT_EQ(world.events.front().second, DamageType::Fall);
    EXPECT_FALSE(player.hurt(source, 0.5f));
    EXPECT_EQ(world.events.size(), 1u);
    EXPECT_TRUE(player.hurt(source, 2.0f));
    EXPECT_EQ(world.events.size(), 1u);
    player.setHurtResistantTime(0);
    EXPECT_TRUE(player.hurt(source, 1.0f));
    EXPECT_EQ(world.events.size(), 2u);
}

TEST_F(NetworkTestBase, DamageEventReachesPlayerWithoutTrackingEntry)
{
    _PacketRecordingConnection connection;
    mc::test::BaseTestServer server;
    server::ServerWorld world(server::ServerWorldConfig{2, 2, 0, 1, "damage-packet-test"});
    world.setServer(&server);
    ASSERT_NE(server.playerManager().addPlayer(1, "00000000000000000000000000000001", "Player", &connection), nullptr);
    auto player = std::make_unique<Player>(101, "Player", *world.entityRegistry());
    player->setWorld(&world);
    const auto entityId = world.entityManager().addEntity(std::move(player));
    ASSERT_TRUE(server.playerEntityManager().registerExistingPlayerEntity(1, entityId, world));
    world.entityTracker().untrackEntity(entityId);
    auto source = DamageSources::fall();
    world.broadcastDamageEvent(entityId, source);
    ASSERT_EQ(connection.packets.size(), 1u);
    const auto& event = std::get<DamageEvent>(std::get<PlayPacket>(connection.packets.back().packet));
    EXPECT_EQ(event.entityId, 101);
    EXPECT_EQ(event.sourceType, 10);
    EXPECT_EQ(event.sourceCauseId, -1);
    EXPECT_EQ(event.sourceDirectId, -1);
    EXPECT_FALSE(event.sourcePosition.has_value());
    Player attacker(201, "Attacker", mc::test::testEcsRegistry());
    Player projectile(202, "Projectile", mc::test::testEcsRegistry());
    IndirectEntityDamageSource arrow(DamageType::Arrow, &attacker, &projectile, true);
    world.broadcastDamageEvent(entityId, arrow);
    ASSERT_EQ(connection.packets.size(), 2u);
    const auto& indirect = std::get<DamageEvent>(std::get<PlayPacket>(connection.packets.back().packet));
    EXPECT_EQ(indirect.sourceCauseId, 201);
    EXPECT_EQ(indirect.sourceDirectId, 202);
    EXPECT_FALSE(indirect.sourcePosition.has_value());
    _PositionedDamageSource positioned;
    world.broadcastDamageEvent(entityId, positioned);
    ASSERT_EQ(connection.packets.size(), 3u);
    const auto& explicitPosition = std::get<DamageEvent>(std::get<PlayPacket>(connection.packets.back().packet));
    ASSERT_TRUE(explicitPosition.sourcePosition.has_value());
    EXPECT_EQ(*explicitPosition.sourcePosition, (std::array<f64, 3>{1.0, 2.0, 3.0}));
}

TEST_F(NetworkTestBase, PlayAwardStatsUsesRegistryTriplesAndBoundsCounts)
{
    const AwardStats packet{{StatValue{8, 1, 300}, StatValue{0, 1, 7}}};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{3, 2, 8, 1, 0xac, 2, 0, 1, 7}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<AwardStats>(decoded.value()), packet);
    for (std::size_t size = 1; size < buf.size(); ++size) {
        buffer::RegistryByteBuf truncated(buf.data(), size, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(truncated).success());
    }
    auto invalid = makeBoundBuf();
    invalid.writeVarInt(3);
    invalid.writeVarInt(std::numeric_limits<i32>::max());
    EXPECT_FALSE(tables()->playCb->decode(invalid).success());
}

TEST_F(NetworkTestBase, StatisticsNetworkDirtyIsIndependentAndResetsSendZero)
{
    using server::stats::StatType;
    server::stats::StatisticsManager stats;
    const ResourceLocation playTime("minecraft:play_time");
    stats.setValue(StatType::Custom, playTime, 300);
    auto packet = stats.createNetworkUpdate(false);
    ASSERT_EQ(packet.stats.size(), 1u);
    EXPECT_EQ(packet.stats.front(), (StatValue{8, 1, 300}));
    EXPECT_TRUE(stats.isDirty());
    stats.clearNetworkDirty();
    EXPECT_TRUE(stats.createNetworkUpdate(false).stats.empty());
    EXPECT_TRUE(stats.isDirty());
    EXPECT_EQ(stats.createNetworkUpdate(true).stats, packet.stats);
    stats.reset(StatType::Custom, playTime);
    packet = stats.createNetworkUpdate(false);
    ASSERT_EQ(packet.stats.size(), 1u);
    EXPECT_EQ(packet.stats.front(), (StatValue{8, 1, 0}));
    stats.setValue(StatType::Custom, playTime, std::numeric_limits<i64>::max());
    EXPECT_EQ(stats.createNetworkUpdate(false).stats.front().value, std::numeric_limits<i32>::max());
    stats.clearNetworkDirty();
    stats.resetAll();
    EXPECT_EQ(stats.createNetworkUpdate(false).stats.front().value, 0);
}

TEST_F(NetworkTestBase, StatisticsUseAllNineVanillaTypesAndIgnoreUnrepresentableNames)
{
    using server::stats::StatType;
    server::stats::StatisticsManager stats;
    stats.setValue(StatType::Mined, ResourceLocation("minecraft:stone"), 1);
    stats.setValue(StatType::Crafted, ResourceLocation("minecraft:stone"), 2);
    stats.setValue(StatType::Used, ResourceLocation("minecraft:stone"), 3);
    stats.setValue(StatType::Broken, ResourceLocation("minecraft:stone"), 4);
    stats.setValue(StatType::PickedUp, ResourceLocation("minecraft:stone"), 5);
    stats.setValue(StatType::Dropped, ResourceLocation("minecraft:stone"), 6);
    stats.setValue(StatType::Killed, ResourceLocation("minecraft:pig"), 7);
    stats.setValue(StatType::KilledBy, ResourceLocation("minecraft:pig"), 8);
    stats.setValue(StatType::Custom, ResourceLocation("minecraft:play_time"), 9);
    stats.setValue(StatType::Custom, ResourceLocation("test:unknown"), 20);
    const auto packet = stats.createNetworkUpdate(true);
    ASSERT_EQ(packet.stats.size(), 9u);
    for (i32 type = 0; type < 9; ++type) {
        EXPECT_EQ(packet.stats[static_cast<std::size_t>(type)].type, type);
        EXPECT_EQ(packet.stats[static_cast<std::size_t>(type)].value, type + 1);
    }
    EXPECT_EQ(packet.stats.front().stat, 1);
    EXPECT_EQ(backend::java::JavaCustomStatIds::toWireId("minecraft:jump"), 23);
}

TEST_F(LocalServerFixture, StatsSendAcknowledgesOnlySuccessfulNetworkDelivery)
{
    ServerPlayer player(1, "Player", mc::test::testEcsRegistry());
    player.getStats().setValue(server::stats::StatType::Custom, ResourceLocation("minecraft:play_time"), 10);
    EXPECT_FALSE(player.sendStats(false));
    EXPECT_EQ(player.getStats().createNetworkUpdate(false).stats.size(), 1u);
    serverConn()->setPhase(protocol::ConnectionProtocol::Play);
    player.setConnection(serverConn());
    ASSERT_TRUE(player.sendStats(false));
    auto received = pumpClient();
    ASSERT_TRUE(received.has_value());
    const auto& packet = std::get<AwardStats>(std::get<PlayPacket>(received->packet));
    ASSERT_EQ(packet.stats.size(), 1u);
    EXPECT_EQ(packet.stats.front(), (StatValue{8, 1, 10}));
    EXPECT_TRUE(player.getStats().createNetworkUpdate(false).stats.empty());
    EXPECT_TRUE(player.getStats().isDirty());
}

TEST_F(NetworkTestBase, PlayEquipmentIncludesEmptySlotsAndSaddleContinuation)
{
    const SetEquipment packet{1, {{WireEquipmentSlot::MainHand, {}}, {WireEquipmentSlot::Saddle, {}}}};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{100, 1, 0x80, 0, 7, 0}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<SetEquipment>(decoded.value()), packet);
    auto invalid = makeBoundBuf();
    invalid.writeVarInt(100);
    invalid.writeVarInt(1);
    invalid.writeU8(8);
    invalid.writeVarInt(0);
    EXPECT_FALSE(tables()->playCb->decode(invalid).success());
}

TEST_F(NetworkTestBase, NewTrackingPlayerGetsEquipmentAndRemovalClearsTheSlot)
{
    _PacketRecordingConnection connection;
    mc::test::BaseTestServer server;
    server::ServerWorld world(server::ServerWorldConfig{2, 2, 0, 1, "equipment-packet-test"});
    world.setServer(&server);
    ASSERT_NE(server.playerManager().addPlayer(1, "00000000000000000000000000000001", "Watcher", &connection), nullptr);
    auto player = std::make_unique<Player>(101, "Equipped", *world.entityRegistry());
    auto* entity = player.get();
    entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::DIAMOND_SWORD, 1));
    entity->setWorld(&world);
    const auto entityId = world.entityManager().addEntity(std::move(player));
    world.entityTracker().trackEntity(entity);
    world.entityTracker().notifyEntityTracked(server, world, entity);
    const auto equipment = std::find_if(connection.packets.begin(), connection.packets.end(), [](const auto& packet) {
        return std::holds_alternative<SetEquipment>(std::get<PlayPacket>(packet.packet));
    });
    ASSERT_NE(equipment, connection.packets.end());
    const auto& snapshot = std::get<SetEquipment>(std::get<PlayPacket>(equipment->packet));
    ASSERT_EQ(snapshot.entries.size(), 1u);
    EXPECT_EQ(snapshot.entries.front().slot, WireEquipmentSlot::MainHand);
    EXPECT_EQ(snapshot.entries.front().item.count, 1);
    connection.packets.clear();
    entity->setEquipment(EquipmentSlot::MainHand, ItemStack::EMPTY);
    world.broadcastEquipmentChanged(entityId, {EquipmentSlot::MainHand});
    ASSERT_EQ(connection.packets.size(), 1u);
    const auto& removal = std::get<SetEquipment>(std::get<PlayPacket>(connection.packets.front().packet));
    ASSERT_EQ(removal.entries.size(), 1u);
    EXPECT_EQ(removal.entries.front().slot, WireEquipmentSlot::MainHand);
    EXPECT_EQ(removal.entries.front().item.count, 0);
}

TEST_F(NetworkTestBase, PlayMobEffectPreservesRegistryIdFlagsAndInfiniteDuration)
{
    const UpdateMobEffect effect{300, 0, 2, 300, 13};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{effect}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{0x82, 1, 0xac, 2, 0, 2, 0xac, 2, 13}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<UpdateMobEffect>(decoded.value()), effect);
    const UpdateMobEffect infinite{1, 39, 255, -1, 15};
    EXPECT_EQ(std::get<UpdateMobEffect>(roundTripGeneric(*tables()->playCb, PlayPacket{infinite})), infinite);
    for (const UpdateMobEffect invalid :
        {UpdateMobEffect{1, 40, 0, 20, 0}, UpdateMobEffect{1, 0, 256, 20, 0}, UpdateMobEffect{1, 0, 0, -2, 0}}) {
        auto malformed = makeBoundBuf();
        ASSERT_TRUE(tables()->playCb->encode(malformed, PlayPacket{invalid}).success());
        EXPECT_FALSE(tables()->playCb->decode(malformed).success());
    }
}

TEST_F(NetworkTestBase, EffectQueueDistinguishesFirstAdditionFromRefresh)
{
    using entity::effect::EffectInstance;
    using entity::effect::EffectType;
    Player player(1, "Player", mc::test::testEcsRegistry());
    ASSERT_TRUE(player.addEffect(EffectInstance(EffectType::Speed, 100, 0, false, true, true)));
    ASSERT_TRUE(player.addEffect(EffectInstance(EffectType::Speed, 200, 0, false, true, true)));
    auto changes = player.effectManager().takePendingChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_TRUE(changes.front().added);
    EXPECT_TRUE(changes.front().blend);
    ASSERT_TRUE(player.addEffect(EffectInstance(EffectType::Speed, 300, 0, false, true, true)));
    changes = player.effectManager().takePendingChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_TRUE(changes.front().added);
    EXPECT_FALSE(changes.front().blend);
    player.removeEffect(EffectType::Speed);
    changes = player.effectManager().takePendingChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_FALSE(changes.front().added);
    EXPECT_FALSE(changes.front().blend);
}

TEST_F(NetworkTestBase, MobEffectsReachUntrackedOwnerAndCanBeReplayed)
{
    using entity::effect::EffectInstance;
    using entity::effect::EffectType;
    _PacketRecordingConnection connection;
    mc::test::BaseTestServer server;
    server::ServerWorld world(server::ServerWorldConfig{2, 2, 0, 1, "effects-packet-test"});
    world.setServer(&server);
    ASSERT_NE(server.playerManager().addPlayer(1, "00000000000000000000000000000001", "Player", &connection), nullptr);
    auto player = std::make_unique<Player>(101, "Player", *world.entityRegistry());
    auto* entity = player.get();
    entity->setWorld(&world);
    const auto entityId = world.entityManager().addEntity(std::move(player));
    ASSERT_TRUE(server.playerEntityManager().registerExistingPlayerEntity(1, entityId, world));
    world.entityTracker().untrackEntity(entityId);
    const EffectInstance effect(EffectType::Speed, 300, 2, true, false, true);
    world.broadcastMobEffectAdded(entityId, effect, true);
    ASSERT_EQ(connection.packets.size(), 1u);
    const auto& added = std::get<UpdateMobEffect>(std::get<PlayPacket>(connection.packets.back().packet));
    EXPECT_EQ(added.effectId, 0);
    EXPECT_EQ(added.flags, 13);
    world.broadcastMobEffectAdded(entityId, effect, false);
    ASSERT_EQ(connection.packets.size(), 2u);
    EXPECT_EQ(std::get<UpdateMobEffect>(std::get<PlayPacket>(connection.packets.back().packet)).flags, 5);
    world.broadcastMobEffectRemoved(entityId, EffectType::Speed);
    ASSERT_EQ(connection.packets.size(), 3u);
    EXPECT_EQ(std::get<RemoveMobEffect>(std::get<PlayPacket>(connection.packets.back().packet)).effectId, 0);
    ASSERT_TRUE(entity->addEffect(effect));
    world.entityTracker().sendActiveEffects(server, 1, *entity);
    ASSERT_EQ(connection.packets.size(), 4u);
    const auto& snapshot = std::get<UpdateMobEffect>(std::get<PlayPacket>(connection.packets.back().packet));
    EXPECT_EQ(snapshot.duration, 300);
    EXPECT_EQ(snapshot.amplifier, 2);
    EXPECT_EQ(snapshot.flags, 5);
}

TEST_F(NetworkTestBase, PlayEntityPositionSyncHasNoRelativeFlagsAndPreservesVelocity)
{
    EntityPositionSync packet{1, 0, 0, 0, 0, 0, 0, 0, 0, true};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    std::vector<u8> golden(59, 0);
    golden[0] = 35;
    golden[1] = 1;
    golden[58] = 1;
    EXPECT_EQ(buf.bytes(), golden);
    for (usize n = 1; n < buf.size(); ++n) {
        buffer::RegistryByteBuf cut(buf.data(), n, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(cut).success());
    }
    packet = {300, 1.25, 64.5, -30000000.25, 0.125, -0.25, 4.5, 90.25f, -45.5f, false};
    EXPECT_EQ(std::get<EntityPositionSync>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
}

TEST_F(NetworkTestBase, AbsoluteTrackingSyncIncludesVelocityAndGroundTransitions)
{
    _PacketRecordingConnection connection;
    mc::test::BaseTestServer server;
    server::ServerWorld world(server::ServerWorldConfig{2, 2, 0, 1, "position-sync-test"});
    world.setServer(&server);
    ASSERT_NE(server.playerManager().addPlayer(1, "00000000000000000000000000000001", "Watcher", &connection), nullptr);
    auto owned = std::make_unique<Player>(101, "Moving", *world.entityRegistry());
    auto* entity = owned.get();
    entity->setWorld(&world);
    world.entityManager().addEntity(std::move(owned));
    world.entityTracker().trackEntity(entity);
    world.entityTracker().notifyEntityTracked(server, world, entity);
    world.entityTracker().tick(server, world);
    connection.packets.clear();
    entity->setPosition(20.0f, 1.0f, 0.0f);
    entity->setVelocity(0.5f, -0.25f, 0.0f);
    entity->setOnGround(true);
    world.entityTracker().tick(server, world);
    const auto found = std::find_if(connection.packets.begin(), connection.packets.end(), [](const auto& packet) {
        return std::holds_alternative<EntityPositionSync>(std::get<PlayPacket>(packet.packet));
    });
    ASSERT_NE(found, connection.packets.end());
    const auto& snapshot = std::get<EntityPositionSync>(std::get<PlayPacket>(found->packet));
    EXPECT_DOUBLE_EQ(snapshot.x, 20.0);
    EXPECT_DOUBLE_EQ(snapshot.deltaX, 0.5);
    EXPECT_DOUBLE_EQ(snapshot.deltaY, -0.25);
    EXPECT_TRUE(snapshot.onGround);
    connection.packets.clear();
    entity->setOnGround(false);
    world.entityTracker().tick(server, world);
    EXPECT_TRUE(std::any_of(connection.packets.begin(), connection.packets.end(), [](const auto& packet) {
        return std::holds_alternative<EntityPositionSync>(std::get<PlayPacket>(packet.packet));
    }));
}

TEST_F(NetworkTestBase, PlayServerLinksSupportsKnownAndCustomLabels)
{
    ServerLinks packet{{{KnownServerLink::Website, "https://example.org"},
        {text::plainTextToNbtBytes("Help"), "https://example.org/help"}}};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    EXPECT_EQ(buf.bytes().at(0), 0x87);
    EXPECT_EQ(buf.bytes().at(1), 1);
    EXPECT_EQ(buf.bytes().at(2), 2);
    EXPECT_EQ(buf.bytes().at(3), 1);
    EXPECT_EQ(buf.bytes().at(4), 6);
    EXPECT_EQ(std::get<ServerLinks>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
    for (usize n = 1; n < buf.size(); ++n) {
        buffer::RegistryByteBuf cut(buf.data(), n, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(cut).success());
    }
}
TEST_F(LocalServerFixture, ServerPlayerReplacesAndClearsServerLinks)
{
    serverConn()->setPhase(protocol::ConnectionProtocol::Play);
    ServerPlayer player(1, "Player", mc::test::testEcsRegistry());
    player.setConnection(serverConn());
    const std::vector<ServerLink> links{{KnownServerLink::Support, "https://example.org"}};
    ASSERT_TRUE(player.sendServerLinks(links));
    auto received = pumpClient();
    ASSERT_TRUE(received);
    EXPECT_EQ(std::get<ServerLinks>(std::get<PlayPacket>(received->packet)).links, links);
    ASSERT_TRUE(player.sendServerLinks({}));
    received = pumpClient();
    ASSERT_TRUE(received);
    EXPECT_TRUE(std::get<ServerLinks>(std::get<PlayPacket>(received->packet)).links.empty());
}

TEST_F(NetworkTestBase, PlayShowDialogSupportsRegisteredAndDirectHolders)
{
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{ShowDialog{2}}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{0x8a, 1, 3}));
    auto built = server::net::buildShowDialogIr(nlohmann::json{
        {"type", "minecraft:notice"}, {"title", "Hello"}, {"action", {{"label", "OK"}, {"width", 150}}}});
    ASSERT_TRUE(built.success());
    const auto& packet = std::get<ShowDialog>(std::get<PlayPacket>(built.value().packet));
    const auto& nbt = std::get<std::vector<u8>>(packet.dialog);
    buffer::ByteBuf nbtBuf(nbt.data(), nbt.size());
    auto parsed = buffer::nbt_io::readRootCompound(nbtBuf);
    ASSERT_TRUE(parsed.success());
    const auto tag = parsed.value();
    const auto& action = tag->get<mc::nbt::tags::compound_tag>("action");
    EXPECT_EQ(action.at("width")->id(), mc::nbt::TagId::Int);
    EXPECT_EQ(std::get<ShowDialog>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
    buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    for (usize n = 1; n < buf.size(); ++n) {
        buffer::RegistryByteBuf cut(buf.data(), n, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(cut).success());
    }
}
TEST_F(LocalServerFixture, ServerPlayerShowsNamedAndInlineDialogs)
{
    serverConn()->setPhase(protocol::ConnectionProtocol::Play);
    ServerPlayer player(1, "Player", mc::test::testEcsRegistry());
    player.setConnection(serverConn());
    ASSERT_TRUE(player.showDialog(ResourceLocation("minecraft:server_links")).success());
    auto packet = pumpClient();
    ASSERT_TRUE(packet);
    EXPECT_EQ(std::get<i32>(std::get<ShowDialog>(std::get<PlayPacket>(packet->packet)).dialog), 0);
    ASSERT_TRUE(player.showDialog(nlohmann::json{{"type", "minecraft:notice"}, {"title", "Hello"}}).success());
    packet = pumpClient();
    ASSERT_TRUE(packet);
    EXPECT_TRUE(
        std::holds_alternative<std::vector<u8>>(std::get<ShowDialog>(std::get<PlayPacket>(packet->packet)).dialog));
}

TEST_F(NetworkTestBase, PlayClearDialogHasEmptyPayload)
{
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{ClearDialog{}}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{0x89, 1}));
    EXPECT_TRUE(std::holds_alternative<ClearDialog>(roundTripGeneric(*tables()->playCb, PlayPacket{ClearDialog{}})));
}
TEST_F(LocalServerFixture, ServerPlayerClearsDialogThroughConnection)
{
    serverConn()->setPhase(protocol::ConnectionProtocol::Play);
    ServerPlayer player(1, "Player", mc::test::testEcsRegistry());
    player.setConnection(serverConn());
    ASSERT_TRUE(player.clearDialog());
    auto packet = pumpClient();
    ASSERT_TRUE(packet);
    EXPECT_TRUE(std::holds_alternative<ClearDialog>(std::get<PlayPacket>(packet->packet)));
    player.setConnection(nullptr);
    EXPECT_FALSE(player.clearDialog());
}

TEST_F(NetworkTestBase, PlayWaypointEncodesIdentifierIconRgbAndAllLocationKinds)
{
    Waypoint packet{
        WaypointOperation::Track, std::string("test"), "minecraft:default", 0x123456u, WaypointPosition{1, 64, -1}};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{packet}).success());
    const std::vector<u8> golden{0x88,
        1,
        0,
        0,
        4,
        't',
        'e',
        's',
        't',
        17,
        'm',
        'i',
        'n',
        'e',
        'c',
        'r',
        'a',
        'f',
        't',
        ':',
        'd',
        'e',
        'f',
        'a',
        'u',
        'l',
        't',
        1,
        0x12,
        0x34,
        0x56,
        1,
        1,
        64,
        255,
        255,
        255,
        255,
        15};
    EXPECT_EQ(buf.bytes(), golden);
    EXPECT_EQ(std::get<Waypoint>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
    for (usize n = 1; n < buf.size(); ++n) {
        buffer::RegistryByteBuf cut(buf.data(), n, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(cut).success());
    }
    packet.identifier = std::array<u8, 16>{};
    packet.operation = WaypointOperation::Update;
    packet.color.reset();
    packet.position = WaypointChunk{-20, 40};
    EXPECT_EQ(std::get<Waypoint>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
    packet.position = -1.25f;
    EXPECT_EQ(std::get<Waypoint>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
    packet.operation = WaypointOperation::Untrack;
    packet.position = std::monostate{};
    EXPECT_EQ(std::get<Waypoint>(roundTripGeneric(*tables()->playCb, PlayPacket{packet})), packet);
}

TEST_F(NetworkTestBase, WaypointsTrackMoveHideChangeRepresentationAndRespectRules)
{
    _PacketRecordingConnection connection;
    mc::test::BaseTestServer server;
    server::ServerWorld world(server::ServerWorldConfig{2, 2, 0, 1, "waypoint-test"});
    auto* data = server.playerManager().addPlayer(1, "00000000000000000000000000000001", "Watcher", &connection);
    ASSERT_NE(data, nullptr);
    data->markClientLoaded();
    data->chunkTracker = std::make_shared<server::sync::PlayerChunkTracker>(1);
    data->chunkTracker->setViewDistance(2);
    auto watcherOwned = std::make_unique<Player>(101, "Watcher", *world.entityRegistry());
    auto* watcher = watcherOwned.get();
    watcher->setWorld(&world);
    watcher->setPosition(0, 0, 0);
    const auto watcherId = world.entityManager().addEntity(std::move(watcherOwned));
    ASSERT_TRUE(server.playerEntityManager().registerExistingPlayerEntity(1, watcherId, world));
    auto sourceOwned = std::make_unique<Player>(102, "Source", *world.entityRegistry());
    auto* source = sourceOwned.get();
    source->setWorld(&world);
    source->setPosition(1, 0, 1);
    world.entityManager().addEntity(std::move(sourceOwned));
    auto& manager = world.waypointManager();
    RecordProperty("stage", "tick_1");
    manager.tick(server, world);
    ASSERT_EQ(connection.packets.size(), 1u);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(
        std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation, WaypointOperation::Track);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_TRUE(std::holds_alternative<WaypointPosition>(
        std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).position));
    connection.packets.clear();
    RecordProperty("stage", "tick_2");
    manager.tick(server, world);
    EXPECT_TRUE(connection.packets.empty());
    source->setPosition(2, 0, 1);
    RecordProperty("stage", "tick_3");
    manager.tick(server, world);
    ASSERT_EQ(connection.packets.size(), 1u);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation,
        WaypointOperation::Update);
    connection.packets.clear();
    source->setSneaking(true);
    RecordProperty("stage", "tick_4");
    manager.tick(server, world);
    ASSERT_EQ(connection.packets.size(), 1u);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation,
        WaypointOperation::Untrack);
    source->setSneaking(false);
    source->setPosition(100, 0, 0);
    RecordProperty("stage", "tick_5");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_TRUE(std::holds_alternative<WaypointChunk>(
        std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).position));
    source->setPosition(400, 0, 0);
    RecordProperty("stage", "tick_6");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_NEAR(std::get<f32>(std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).position),
        -math::PI / 2.0,
        0.00001);
    source->attributes().setBaseValue(entity::attribute::Attributes::WAYPOINT_TRANSMIT_RANGE, 10.0);
    RecordProperty("stage", "tick_7");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation,
        WaypointOperation::Untrack);
    source->attributes().setBaseValue(entity::attribute::Attributes::WAYPOINT_TRANSMIT_RANGE, 60000000.0);
    RecordProperty("stage", "tick_8");
    manager.tick(server, world);
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::LOCATOR_BAR, false, nullptr);
    RecordProperty("stage", "tick_9");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation,
        WaypointOperation::Untrack);
    connection.packets.clear();
    RecordProperty("stage", "tick_10");
    manager.tick(server, world);
    EXPECT_TRUE(connection.packets.empty());
    world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::LOCATOR_BAR, true, nullptr);
    source->setWaypointIcon(ResourceLocation("minecraft:bowtie"), 0x123456u);
    RecordProperty("stage", "tick_11");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    const auto& icon = std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet));
    EXPECT_EQ(icon.style, "minecraft:bowtie");
    EXPECT_EQ(icon.color, 0x123456u);
    manager.clearReceiver(server, 1);
    RecordProperty("stage", "tick_12");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(
        std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation, WaypointOperation::Track);
    source->setEquipment(EquipmentSlot::Head, ItemStack(Items::CARVED_PUMPKIN, 1));
    RecordProperty("stage", "tick_13");
    manager.tick(server, world);
    ASSERT_FALSE(connection.packets.empty());
    EXPECT_EQ(std::get<Waypoint>(std::get<PlayPacket>(connection.packets.back().packet)).operation,
        WaypointOperation::Untrack);
}

TEST_F(NetworkTestBase, WaypointIconPersistsAcrossEntitySave)
{
    Player source(200, "Source", mc::test::testEcsRegistry());
    source.setWaypointIcon(ResourceLocation("minecraft:bowtie"), 0xabcdefu);
    mc::nbt::tags::compound_tag tag;
    source.addAdditionalSaveData(tag);
    Player restored(201, "Restored", mc::test::testEcsRegistry());
    ASSERT_TRUE(restored.readAdditionalSaveData(tag).success());
    EXPECT_EQ(restored.waypointStyle(), "minecraft:bowtie");
    EXPECT_EQ(restored.waypointColor(), 0xabcdefu);
}

TEST_F(NetworkTestBase, DialogCommandExposesShowAndClearAndRequiresPermission)
{
    command::CommandDispatcher<command::ServerCommandSource> dispatcher;
    command::DialogCommand::registerTo(dispatcher);
    const auto root = dispatcher.getRoot()->getChild("dialog");
    ASSERT_NE(root, nullptr);
    EXPECT_NE(root->getChild("show"), nullptr);
    EXPECT_NE(root->getChild("clear"), nullptr);
    command::ServerCommandSource low(nullptr, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "", nullptr);
    EXPECT_FALSE(dispatcher.execute("dialog clear @a", low).success());
    EXPECT_FALSE(dispatcher.execute("dialog show @a minecraft:server_links", low).success());
    EXPECT_FALSE(server::net::buildShowDialogIr(nlohmann::json{{"title", "Missing type"}}).success());
}
