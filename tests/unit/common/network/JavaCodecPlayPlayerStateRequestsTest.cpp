#include "common/network/NetworkTestFixtures.hpp"
#include "server/application/IntegratedServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/play/ServerPlayHandler.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

namespace {

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
