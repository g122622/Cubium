#include "BaseTestServer.hpp"

namespace mc::test {

void FakeServerConnection::send(const u8* data, size_t size)
{
    if (data != nullptr && size > 0) {
        m_sentData.insert(m_sentData.end(), data, data + size);
    }
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
    , m_keepAliveManager(m_playerManager, 15000, 30000)
    , m_positionTracker(m_playerManager, 10)
    , m_packetHandler(m_playerManager,
          m_connectionManager,
          m_teleportManager,
          m_keepAliveManager,
          m_positionTracker,
          m_timeManager,
          GameMode::Survival)
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

void BaseTestServer::sendSoundToPlayer(PlayerId playerId,
    const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 volume,
    f32 pitch)
{
    m_lastSoundPlayerId = playerId;
    m_lastSoundEvent = soundEventId;
    m_lastSoundCategory = category;
    m_lastSoundPosition = position;
    m_lastSoundVolume = volume;
    m_lastSoundPitch = pitch;
    m_soundSent = true;
}

server::ServerPlayerData* BaseTestServer::addTestPlayer(PlayerId playerId, const std::string& username)
{
    auto connection = std::make_shared<FakeServerConnection>();
    std::string uuid = util::uuidToString(util::generateOfflineUuid(username));
    // 新网络层 addPlayer 接受 mc::server::net::ServerClientConnection*；测试桩 FakeServerConnection
    // 仍为旧 IServerConnection 派生（Step5 删旧体系时统一重构测试桩），此处传 nullptr。
    // BaseTestServer 重写了 sendSoundToPlayer 等发送路径，不依赖 player.connection 真发包。
    auto* player = m_playerManager.addPlayer(playerId, uuid, username, nullptr);
    if (player != nullptr) {
        m_connections.push_back(connection);
        m_inventoryManager.initializeInventory(playerId);
    }
    return player;
}

} // namespace mc::test
