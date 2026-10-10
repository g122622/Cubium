#include "common/entity/entities/player/Player.hpp"
#include "common/network/NetworkTestFixtures.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IntegratedServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/play/ServerPlayHandler.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

namespace {

class _PlayerStateRequestWorld : public server::ServerWorld {
public:
    _PlayerStateRequestWorld()
        : ServerWorld(server::ServerWorldConfig{2, 2, 0, 1, "packet-test"})
    {}

    void broadcastEntityAnimation(EntityInstanceId entityId, u8 animation) override
    {
        animations.emplace_back(entityId, animation);
    }

    std::vector<std::pair<EntityInstanceId, u8>> animations;
};

// 只创建包处理所需的管理器，不启动网络、线程池或生成世界。
class _PlayerStateRequestServer : public server::IntegratedServer {
public:
    _PlayerStateRequestServer()
    {
        m_playerManager = std::make_unique<server::core::PlayerManager>(10);
        m_connectionManager = std::make_unique<server::core::ConnectionManager>(*m_playerManager);
        m_gameModeManager = std::make_unique<server::core::GameModeManager>(*m_playerManager, *m_connectionManager);
        m_opListManager = std::make_unique<server::core::OpListManager>();
    }

    server::ServerWorld* getPlayerWorld(PlayerId) override { return m_world.get(); }

    Player& addPlayerEntity(PlayerId playerId)
    {
        if (!m_world) {
            m_world = std::make_unique<_PlayerStateRequestWorld>();
        }
        auto player = std::make_unique<Player>(EntityInstanceId(100 + playerId), "Player", *m_world->entityRegistry());
        auto* result = player.get();
        result->setWorld(m_world.get());
        const auto entityId = m_world->entityManager().addEntity(std::move(player));
        EXPECT_TRUE(m_playerEntityManager.registerExistingPlayerEntity(playerId, entityId, *m_world));
        return *result;
    }

    void sendPacketToPlayer(PlayerId playerId, const IrPacket& packet) override
    {
        sentPackets.emplace_back(playerId, packet);
    }

    std::vector<std::pair<PlayerId, IrPacket>> sentPackets;
    std::unique_ptr<_PlayerStateRequestWorld> m_world;
};

} // namespace

TEST_F(NetworkTestBase, PlayChangeGameModeWireAndRoundTrip)
{
    for (i32 mode = 0; mode <= 3; ++mode) {
        auto buf = makeBoundBuf();
        ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{ChangeGameMode{mode}}).success());
        EXPECT_EQ(buf.bytes(), (std::vector<u8>{4, static_cast<u8>(mode)}));
        auto decoded = tables()->playSb->decode(buf);
        ASSERT_TRUE(decoded.success());
        EXPECT_EQ(std::get<ChangeGameMode>(decoded.value()).mode, mode);
    }
}

TEST_F(NetworkTestBase, PlaySwingWireValidatesBothHands)
{
    for (i32 hand : {0, 1}) {
        auto buf = makeBoundBuf();
        ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{Swing{hand}}).success());
        EXPECT_EQ(buf.bytes(), (std::vector<u8>{60, static_cast<u8>(hand)}));
        auto decoded = tables()->playSb->decode(buf);
        ASSERT_TRUE(decoded.success());
        EXPECT_EQ(std::get<Swing>(decoded.value()).hand, hand);
    }
    for (i32 hand : {-1, 2}) {
        auto buf = makeBoundBuf();
        buf.writeVarInt(60);
        buf.writeVarInt(hand);
        EXPECT_FALSE(tables()->playSb->decode(buf).success());
    }
}

TEST(PlayerStateRequestTest, SwingRoutesToRealEntityAndBroadcastsSelectedHand)
{
    _PlayerStateRequestServer server;
    server::net::ServerPlayHandler handler(server);
    for (i32 hand : {0, 1}) {
        const PlayerId playerId = static_cast<PlayerId>(hand + 1);
        ASSERT_NE(
            server.playerManager().addPlayer(playerId, "uuid" + std::to_string(hand), "Player", nullptr), nullptr);
        auto& player = server.addPlayerEntity(playerId);
        handler.route(playerId, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{Swing{hand}}});
        EXPECT_TRUE(player.isSwingInProgress());
        EXPECT_EQ(player.swingingHand(), static_cast<Hand>(hand));
        EXPECT_EQ(server.m_world->animations.back().first, player.id());
        EXPECT_EQ(server.m_world->animations.back().second, hand == 0 ? 0 : 3);
    }
    EXPECT_EQ(server.m_world->animations.size(), 2u);
}

TEST_F(NetworkTestBase, PlayPlayerLoadedWireAndRoundTrip)
{
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{PlayerLoaded{}}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{43}));
    auto decoded = tables()->playSb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_TRUE(std::holds_alternative<PlayerLoaded>(decoded.value()));
}

TEST(PlayerStateRequestTest, PlayerLoadedClearsTimerWithoutBypassingRespawn)
{
    _PlayerStateRequestServer server;
    auto* player = server.playerManager().addPlayer(1, "player-uuid", "Player", nullptr);
    ASSERT_NE(player, nullptr);
    server::net::ServerPlayHandler handler(server);
    const IrPacket request{protocol::ConnectionProtocol::Play, PlayPacket{PlayerLoaded{}}};
    player->restartClientLoadTimerAfterRespawn();
    EXPECT_FALSE(player->hasClientLoaded());
    handler.route(1, request);
    EXPECT_TRUE(player->hasClientLoaded());
    player->markClientUnloadedAfterDeath();
    handler.route(1, request);
    EXPECT_TRUE(player->waitingForRespawn);
    EXPECT_FALSE(player->hasClientLoaded());
    player->restartClientLoadTimerAfterRespawn();
    player->loggedIn = false;
    handler.route(1, request);
    EXPECT_EQ(player->clientLoadedTimeoutTimer, 60);
    handler.route(99, request);
}

TEST_F(NetworkTestBase, PlayChangeGameModeRejectsMalformedModes)
{
    for (i32 mode : {-1, 4, 255}) {
        auto buf = makeBoundBuf();
        buf.writeVarInt(4);
        buf.writeVarInt(mode);
        EXPECT_FALSE(tables()->playSb->decode(buf).success());
    }
    auto truncated = makeBoundBuf();
    truncated.writeVarInt(4);
    EXPECT_FALSE(tables()->playSb->decode(truncated).success());
}

TEST(PlayerStateRequestTest, ChangeGameModeRequiresGameMasterAndLoggedInPlayer)
{
    _PlayerStateRequestServer server;
    auto* player = server.playerManager().addPlayer(1, "player-uuid", "Player", nullptr);
    ASSERT_NE(player, nullptr);
    server::net::ServerPlayHandler handler(server);
    const IrPacket request{protocol::ConnectionProtocol::Play, PlayPacket{ChangeGameMode{1}}};
    handler.route(1, request);
    EXPECT_EQ(player->gameMode, GameMode::Survival);
    server.opListManager().setEntry(
        server::core::OpEntry("player-uuid", "Player", server::core::OpLevel::Moderator, false));
    handler.route(1, request);
    EXPECT_EQ(player->gameMode, GameMode::Survival);
    server.opListManager().setEntry(
        server::core::OpEntry("player-uuid", "Player", server::core::OpLevel::GameMaster, false));
    handler.route(1, request);
    EXPECT_EQ(player->gameMode, GameMode::Creative);
    player->loggedIn = false;
    handler.route(1, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{ChangeGameMode{3}}});
    EXPECT_EQ(player->gameMode, GameMode::Creative);
    handler.route(99, request);
}

TEST_F(NetworkTestBase, PlayServerboundPlayerAbilitiesHasOnlyOneFlagByte)
{
    for (bool flying : {false, true}) {
        auto buf = makeBoundBuf();
        ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{ServerboundPlayerAbilities{flying}}).success());
        EXPECT_EQ(buf.bytes(), (std::vector<u8>{39, static_cast<u8>(flying ? 2 : 0)}));
        auto decoded = tables()->playSb->decode(buf);
        ASSERT_TRUE(decoded.success());
        EXPECT_EQ(std::get<ServerboundPlayerAbilities>(decoded.value()).flying, flying);
    }
    auto buf = makeBoundBuf();
    buf.writeVarInt(39);
    buf.writeU8(0xff);
    auto decoded = tables()->playSb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_TRUE(std::get<ServerboundPlayerAbilities>(decoded.value()).flying);
    auto truncated = makeBoundBuf();
    truncated.writeVarInt(39);
    EXPECT_FALSE(tables()->playSb->decode(truncated).success());
}

TEST(PlayerStateRequestTest, AbilitiesCannotGrantFlightOrOtherPrivileges)
{
    _PlayerStateRequestServer server;
    auto* data = server.playerManager().addPlayer(1, "player-uuid", "Player", nullptr);
    ASSERT_NE(data, nullptr);
    auto& player = server.addPlayerEntity(1);
    server::net::ServerPlayHandler handler(server);
    const IrPacket fly{protocol::ConnectionProtocol::Play, PlayPacket{ServerboundPlayerAbilities{true}}};
    handler.route(1, fly);
    EXPECT_FALSE(player.abilities().flying);
    EXPECT_FALSE(player.abilities().canFly);
    EXPECT_FALSE(player.abilities().invulnerable);
    player.setGameMode(GameMode::Creative);
    handler.route(1, fly);
    EXPECT_TRUE(player.abilities().flying);
    handler.route(1, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{ServerboundPlayerAbilities{false}}});
    EXPECT_FALSE(player.abilities().flying);
    player.setGameMode(GameMode::Survival);
    handler.route(1, fly);
    EXPECT_FALSE(player.abilities().flying);
    player.setGameMode(GameMode::Creative);
    data->loggedIn = false;
    handler.route(1, fly);
    EXPECT_FALSE(player.abilities().flying);
}

TEST(PlayerStateRequestTest, SwitchKeepsLoadSignalsSeparateFromGameModeRequests)
{
    _PlayerStateRequestServer server;
    auto* data = server.playerManager().addPlayer(1, "player-uuid", "Player", nullptr);
    ASSERT_NE(data, nullptr);
    server.opListManager().setEntry(
        server::core::OpEntry("player-uuid", "Player", server::core::OpLevel::GameMaster, false));
    server::net::ServerPlayHandler handler(server);
    data->restartClientLoadTimerAfterRespawn();
    handler.route(1, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{ChangeGameMode{1}}});
    EXPECT_EQ(data->gameMode, GameMode::Creative);
    EXPECT_EQ(data->clientLoadedTimeoutTimer, 60);
    handler.route(1, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{PlayerLoaded{}}});
    EXPECT_TRUE(data->hasClientLoaded());
    EXPECT_EQ(data->gameMode, GameMode::Creative);
    handler.route(1, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{SetTime{}}});
    EXPECT_TRUE(data->hasClientLoaded());
    EXPECT_EQ(data->gameMode, GameMode::Creative);
}

TEST_F(NetworkTestBase, PlayClientInformationSharesConfigurationWire)
{
    configuration::ClientInformation information{"en_us", -1, 2, true, 0x40, 0, true, false, 2};
    const ClientInformation request{information};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{request}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{13, 5, 'e', 'n', '_', 'u', 's', 0xff, 2, 1, 0x40, 0, 1, 0, 2}));
    auto decoded = tables()->playSb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<ClientInformation>(decoded.value()), request);
    auto config = makeBoundBuf();
    ASSERT_TRUE(tables()->configurationSb->encode(config, ConfigurationPacket{information}).success());
    EXPECT_EQ(std::vector<u8>(buf.bytes().begin() + 1, buf.bytes().end()),
        std::vector<u8>(config.bytes().begin() + 1, config.bytes().end()));
    for (i32 invalid : {-1, 3}) {
        auto malformed = makeBoundBuf();
        auto bad = information;
        bad.chatVisibility = invalid;
        ASSERT_TRUE(tables()->playSb->encode(malformed, PlayPacket{ClientInformation{bad}}).success());
        EXPECT_FALSE(tables()->playSb->decode(malformed).success());
    }
    auto malformed = makeBoundBuf();
    information.language = std::string(17, 'a');
    ASSERT_TRUE(tables()->playSb->encode(malformed, PlayPacket{ClientInformation{information}}).success());
    EXPECT_FALSE(tables()->playSb->decode(malformed).success());
}

TEST(PlayerStateRequestTest, ClientInformationUpdatesEntityAndRetainsAllPreferences)
{
    _PlayerStateRequestServer server;
    auto* data = server.playerManager().addPlayer(1, "00000000-0000-0000-0000-000000000001", "Player", nullptr);
    ASSERT_NE(data, nullptr);
    auto& player = server.addPlayerEntity(1);
    server::net::ServerPlayHandler handler(server);
    const configuration::ClientInformation information{"zh_cn", 4, 2, false, 0x01, 0, true, false, 2};
    handler.route(1, IrPacket{protocol::ConnectionProtocol::Play, PlayPacket{ClientInformation{information}}});
    ASSERT_TRUE(data->clientInformation.has_value());
    EXPECT_EQ(*data->clientInformation, information);
    EXPECT_EQ(player.chatVisibility(), ChatVisibility::Hidden);
    EXPECT_EQ(player.playerModelParts(), 0x01);
    EXPECT_EQ(player.getPrimaryHand(), HandSide::Left);
    EXPECT_EQ(data->chunkTracker->viewDistance(), 4);
    ASSERT_EQ(server.sentPackets.size(), 1u);
    const auto& update = std::get<PlayerInfoUpdate>(std::get<PlayPacket>(server.sentPackets.back().second.packet));
    EXPECT_EQ(update.actions, 0x88);
    EXPECT_EQ(update.entries.front().showHat, false);
    EXPECT_EQ(update.entries.front().listed, false);
    data->loggedIn = false;
    handler.route(1,
        IrPacket{protocol::ConnectionProtocol::Play,
            PlayPacket{
                ClientInformation{configuration::ClientInformation{"en_us", 8, 0, true, 0x7f, 1, false, true, 0}}}});
    EXPECT_EQ(*data->clientInformation, information);
}
