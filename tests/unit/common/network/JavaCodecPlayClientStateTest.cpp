#include "common/TestWorldHelper.hpp"
#include "common/network/NetworkTestFixtures.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/player/ServerPlayer.hpp"
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
