#include "BaseTestServer.hpp"

#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

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
{
    // 物品栏下发出口：生产的实现挂在 MinecraftServer 上，命令侧统一经
    // InventoryManager::syncToClient 触发。测试桩若不接这条链路，命令改完物品栏后
    // 「客户端收到同步」就无从断言——本桩补上与生产同形的 ContainerSetContent(containerId=0)。
    m_inventoryManager.setOnInventoryUpdate([this](PlayerId playerId, const PlayerInventory& inventory) {
        mc::network::ir::play::ContainerSetContent pkt;
        pkt.containerId = 0; // 玩家物品栏
        const auto* playerData = m_playerManager.getPlayer(playerId);
        pkt.stateId = (playerData != nullptr) ? playerData->incrementPlayerInventoryStateId() : 0;
        pkt.items = mc::buildMenuContent(inventory);
        pkt.carriedItem = mc::network::ir::play::ItemStackView{};
        m_connectionManager.sendToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}});
    });
}

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

// playerEntityManager() 已在头文件内联返回 m_playerEntityManager 成员（真实空对象），
// 不再 throwUnused。详见 BaseTestServer.hpp 中 m_playerEntityManager 成员注释。

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
        // 为该测试玩家准备物品栏。生产环境的权威数据在玩家实体上（Player::m_inventory），
        // 由服务器注入的解析器定位；测试不构造完整玩家实体，故这里直接持有一份并经解析器
        // 暴露出去——对被测代码而言仍是「该玩家唯一的那一份」。
        m_testInventories[playerId] = std::make_unique<PlayerInventory>(nullptr);
        m_inventoryManager.setInventoryResolver([this](PlayerId id) -> PlayerInventory* {
            auto it = m_testInventories.find(id);
            return (it != m_testInventories.end()) ? it->second.get() : nullptr;
        });
    }
    return player;
}

} // namespace mc::test
