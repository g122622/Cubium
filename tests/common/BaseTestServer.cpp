#include "BaseTestServer.hpp"

namespace mc::test {

Result<void> FakeServerConnection::send(mc::network::ir::IrPacket packet)
{
    // 仅记录"发送了一包"：写入阶段 + play 变体下标各一字节，供 sentBytes()>0 断言。
    // 不还原包内容——命令测试只关心是否触发了出站同步。
    m_sentData.push_back(static_cast<u8>(packet.phase));
    m_sentData.push_back(static_cast<u8>(packet.packet.index()));
    (void)packet;
    return Result<void>::ok();
}

void FakeServerConnection::disconnect(const std::string& reason)
{
    m_disconnectReason = reason;
    m_connected = false;
}

BaseTestServer::BaseTestServer()
    : m_playerManager(20)
    , m_inventoryManager(m_playerManager)
    , m_connectionManager(m_playerManager)
    , m_timeManager(0, 1000)
    , m_teleportManager(m_playerManager)
    , m_keepAliveManager(m_playerManager)
    , m_positionTracker(m_playerManager, 10)
    , m_gameModeManager(m_playerManager, m_connectionManager)
    , m_commandRegistry()
    , m_scoreboard(*this)
{}

[[noreturn]] void BaseTestServer::throwUnused()
{
    throw std::logic_error("unused");
}

ServerDimensionManager& BaseTestServer::dimensionManager()
{
    throwUnused();
}

const ServerDimensionManager& BaseTestServer::dimensionManager() const
{
    throwUnused();
}

server::ServerWorld* BaseTestServer::getPlayerWorld(PlayerId)
{
    return m_playerWorld;
}

server::ServerPlayerEntityManager& BaseTestServer::playerEntityManager()
{
    throwUnused();
}

const server::ServerPlayerEntityManager& BaseTestServer::playerEntityManager() const
{
    throwUnused();
}

server::interaction::BlockInteractionManager& BaseTestServer::blockInteractionManager()
{
    throwUnused();
}

const server::interaction::BlockInteractionManager& BaseTestServer::blockInteractionManager() const
{
    throwUnused();
}

server::interaction::MiningManager& BaseTestServer::miningManager()
{
    throwUnused();
}

const server::interaction::MiningManager& BaseTestServer::miningManager() const
{
    throwUnused();
}

server::interaction::ContainerManager& BaseTestServer::containerManager()
{
    throwUnused();
}

const server::interaction::ContainerManager& BaseTestServer::containerManager() const
{
    throwUnused();
}

resource::DataPackRepository& BaseTestServer::dataPackList()
{
    throwUnused();
}

const resource::DataPackRepository& BaseTestServer::dataPackList() const
{
    throwUnused();
}

loot::LootTableManager& BaseTestServer::lootTableManager()
{
    throwUnused();
}

const loot::LootTableManager& BaseTestServer::lootTableManager() const
{
    throwUnused();
}

server::CustomServerBossInfoManager& BaseTestServer::bossBarManager()
{
    throwUnused();
}

const server::CustomServerBossInfoManager& BaseTestServer::bossBarManager() const
{
    throwUnused();
}

server::ServerPlayerData* BaseTestServer::addTestPlayer(PlayerId playerId, const std::string& username)
{
    auto connection = std::make_shared<FakeServerConnection>();
    std::string uuid = util::uuidToString(util::generateOfflineUuid(username));
    // 注入测试桩连接：FakeServerConnection 实现 IServerClientConnection，记录
    // send 的字节数与 disconnect 的原因，使 KickCommand/ClearCommand 等出站
    // 路径在命令测试中可断言。桩所有权由 BaseTestServer 持有，addPlayer 仅存裸指针。
    auto* player = m_playerManager.addPlayer(playerId, uuid, username, connection.get());
    if (player != nullptr) {
        m_connections.push_back(connection);
        m_inventoryManager.initializeInventory(playerId);
    }
    return player;
}

} // namespace mc::test
