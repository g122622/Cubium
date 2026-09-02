#pragma once

#include "common/item/loot/LootPredicateManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/function/TimerQueue.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/network/IServerClientConnection.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace mc::test {

/**
 * @brief 测试桩连接：实现 IServerClientConnection，记录发送字节数与断连原因。
 *
 * 网络重写后 ServerPlayerData.connection 收窄为 IServerClientConnection*，
 * 测试桩无需再构造真实 transport/协议表即可注入玩家连接，使 KickCommand
 * 的 disconnect(reason)、ClearCommand 的 sendToPlayer 等出站路径在命令测试
 * 中可被断言。send(IrPacket) 累积每包一个哨兵字节（值=play 变体下标），
 * 仅供 sentBytes()>0 断言，不还原包内容。
 */
class FakeServerConnection : public mc::server::net::IServerClientConnection {
public:
    [[nodiscard]] Result<void> send(mc::network::ir::IrPacket packet) override;
    void close() override { m_connected = false; }
    [[nodiscard]] bool isConnected() const noexcept override { return m_connected; }
    void disconnect(const std::string& reason) override;
    [[nodiscard]] std::string peerAddress() const override { return {}; }

    [[nodiscard]] size_t sentBytes() const noexcept { return m_sentData.size(); }
    [[nodiscard]] const std::string& disconnectReason() const noexcept { return m_disconnectReason; }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

class BaseTestServer : public server::IServer {
public:
    BaseTestServer();
    ~BaseTestServer() override = default;

    [[nodiscard]] Result<void> initialize() override { return Result<void>::ok(); }
    void shutdown() override { m_running = false; }
    void tick() override {}
    [[nodiscard]] bool isRunning() const override { return m_running; }

    [[nodiscard]] bool isIntegrated() const noexcept override { return false; }
    [[nodiscard]] bool isDedicated() const noexcept override { return true; }

    [[nodiscard]] server::core::PlayerManager& playerManager() override { return m_playerManager; }
    [[nodiscard]] const server::core::PlayerManager& playerManager() const override { return m_playerManager; }
    [[nodiscard]] server::core::ConnectionManager& connectionManager() override { return m_connectionManager; }
    [[nodiscard]] const server::core::ConnectionManager& connectionManager() const override
    {
        return m_connectionManager;
    }
    [[nodiscard]] server::core::TimeManager& timeManager() override { return m_timeManager; }
    [[nodiscard]] const server::core::TimeManager& timeManager() const override { return m_timeManager; }
    [[nodiscard]] server::core::TeleportManager& teleportManager() override { return m_teleportManager; }
    [[nodiscard]] const server::core::TeleportManager& teleportManager() const override { return m_teleportManager; }
    [[nodiscard]] server::core::KeepAliveManager& keepAliveManager() override { return m_keepAliveManager; }
    [[nodiscard]] const server::core::KeepAliveManager& keepAliveManager() const override { return m_keepAliveManager; }
    [[nodiscard]] server::core::PositionTracker& positionTracker() override { return m_positionTracker; }
    [[nodiscard]] const server::core::PositionTracker& positionTracker() const override { return m_positionTracker; }
    [[nodiscard]] server::core::GameModeManager& gameModeManager() override { return m_gameModeManager; }
    [[nodiscard]] const server::core::GameModeManager& gameModeManager() const override { return m_gameModeManager; }
    [[nodiscard]] server::core::WhitelistManager& whitelistManager() override { return m_whitelistManager; }
    [[nodiscard]] const server::core::WhitelistManager& whitelistManager() const override { return m_whitelistManager; }
    [[nodiscard]] server::core::BannedPlayerList& bannedPlayerList() override { return m_bannedPlayerList; }
    [[nodiscard]] const server::core::BannedPlayerList& bannedPlayerList() const override { return m_bannedPlayerList; }
    [[nodiscard]] server::core::BannedIpList& bannedIpList() override { return m_bannedIpList; }
    [[nodiscard]] const server::core::BannedIpList& bannedIpList() const override { return m_bannedIpList; }
    [[nodiscard]] server::core::OpListManager& opListManager() override { return m_opListManager; }
    [[nodiscard]] const server::core::OpListManager& opListManager() const override { return m_opListManager; }

    [[nodiscard]] ServerDimensionManager& dimensionManager() override;
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override;
    [[nodiscard]] server::ServerWorld* getPlayerWorld(PlayerId playerId) override;
    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }
    [[nodiscard]] server::interaction::BlockInteractionManager& blockInteractionManager() override;
    [[nodiscard]] const server::interaction::BlockInteractionManager& blockInteractionManager() const override;
    [[nodiscard]] server::interaction::MiningManager& miningManager() override;
    [[nodiscard]] const server::interaction::MiningManager& miningManager() const override;
    [[nodiscard]] server::interaction::ContainerManager& containerManager() override;
    [[nodiscard]] const server::interaction::ContainerManager& containerManager() const override;
    [[nodiscard]] server::interaction::InventoryManager& inventoryManager() override { return m_inventoryManager; }
    [[nodiscard]] const server::interaction::InventoryManager& inventoryManager() const override
    {
        return m_inventoryManager;
    }
    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override
    {
        return m_inventoryManager.getInventory(playerId);
    }
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override
    {
        return m_inventoryManager.getInventory(playerId);
    }
    [[nodiscard]] command::CommandRegistry& commandRegistry() override { return m_commandRegistry; }
    [[nodiscard]] const command::CommandRegistry& commandRegistry() const override { return m_commandRegistry; }
    [[nodiscard]] command::CommandStorage& commandStorage() override { return m_commandStorage; }
    [[nodiscard]] const command::CommandStorage& commandStorage() const override { return m_commandStorage; }
    [[nodiscard]] resource::DataPackRepository& dataPackList() override;
    [[nodiscard]] const resource::DataPackRepository& dataPackList() const override;
    [[nodiscard]] loot::LootTableManager& lootTableManager() override;
    [[nodiscard]] const loot::LootTableManager& lootTableManager() const override;
    [[nodiscard]] loot::LootPredicateManager& predicateManager() override { return m_predicateManager; }
    [[nodiscard]] const loot::LootPredicateManager& predicateManager() const override { return m_predicateManager; }
    [[nodiscard]] function::FunctionManager& functionManager() override { return m_functionManager; }
    [[nodiscard]] const function::FunctionManager& functionManager() const override { return m_functionManager; }
    [[nodiscard]] function::TimerQueue& functionTimerQueue() override { return m_functionTimerQueue; }
    [[nodiscard]] const function::TimerQueue& functionTimerQueue() const override { return m_functionTimerQueue; }
    [[nodiscard]] world::storage::SingleLevelStorageManager* sharedStorage() override { return nullptr; }
    [[nodiscard]] const world::storage::SingleLevelStorageManager* sharedStorage() const override { return nullptr; }
    [[nodiscard]] bool isSharedStorageReadonlyForeignWorld() const override { return false; }
    [[nodiscard]] server::ServerScoreboard& scoreboard() override { return m_scoreboard; }
    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override { return m_scoreboard; }
    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override;
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override;

    [[nodiscard]] i32 viewDistance() const override { return m_viewDistance; }
    [[nodiscard]] i32 simulationDistance() const override { return m_simulationDistance; }
    [[nodiscard]] i32 maxPlayers() const override { return m_maxPlayers; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return m_timeManager.currentTick(); }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty difficulty) override { m_difficulty = difficulty; }
    [[nodiscard]] bool isDifficultyLocked() const noexcept override { return m_difficultyLocked; }
    void setDifficultyLocked(bool locked) override { m_difficultyLocked = locked; }
    [[nodiscard]] bool isSingleplayerOwner(PlayerId /*playerId*/) const noexcept override { return false; }
    [[nodiscard]] i32 spawnProtectionRadius() const noexcept override { return 0; }
    [[nodiscard]] GameMode defaultGameMode() const override { return m_defaultGameMode; }
    void setDefaultGameMode(GameMode mode) override { m_defaultGameMode = mode; }
    [[nodiscard]] i32 playerIdleTimeoutMinutes() const override { return m_idleTimeoutMinutes; }
    void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) override { m_idleTimeoutMinutes = timeoutMinutes; }
    void requestStop() override
    {
        m_stopRequested = true;
        m_running = false;
    }
    [[nodiscard]] Result<void> publishToLan(i32 port, bool allowCheats) override
    {
        (void)port;
        (void)allowCheats;
        return Error(ErrorCode::Unsupported, "Test server does not support LAN publishing");
    }

    [[nodiscard]] server::ServerPlayerData* addTestPlayer(PlayerId playerId, const std::string& username);
    void setPlayerWorld(server::ServerWorld* world) { m_playerWorld = world; }
    [[nodiscard]] bool stopRequested() const noexcept { return m_stopRequested; }

protected:
    [[noreturn]] static void throwUnused();
    [[nodiscard]] std::shared_ptr<FakeServerConnection> lastConnection() const
    {
        return m_connections.empty() ? nullptr : m_connections.back();
    }

    i32 m_viewDistance = 10;
    i32 m_simulationDistance = 10;
    i32 m_maxPlayers = 20;
    u64 m_seed = 0;
    bool m_running = true;
    Difficulty m_difficulty = Difficulty::Normal;
    bool m_difficultyLocked = false;
    GameMode m_defaultGameMode = GameMode::Survival;
    i32 m_idleTimeoutMinutes = 0;
    bool m_stopRequested = false;

    server::ServerWorld* m_playerWorld = nullptr;
    server::core::PlayerManager m_playerManager;
    server::interaction::InventoryManager m_inventoryManager;
    // 真实空玩家实体管理器：默认构造无依赖（mutex + 两个空 map），getPlayerIds() 返回空 vector、
    // getPlayerEntity() 返回 nullptr。命令测试经 PlayerResolver::getSortedPlayerIds 调
    // playerEntityManager().getPlayerIds()，须返回空对象而非抛 "unused"（原 throwUnused 桩致
    // PlayerResolverTest/SpawnPointCommandTest/TellRawCommandTest 共 16 用例崩溃）。
    server::ServerPlayerEntityManager m_playerEntityManager;
    server::core::ConnectionManager m_connectionManager;
    server::core::TimeManager m_timeManager;
    server::core::TeleportManager m_teleportManager;
    server::core::KeepAliveManager m_keepAliveManager;
    server::core::PositionTracker m_positionTracker;
    server::core::GameModeManager m_gameModeManager;
    server::core::WhitelistManager m_whitelistManager;
    server::core::BannedPlayerList m_bannedPlayerList;
    server::core::BannedIpList m_bannedIpList;
    server::core::OpListManager m_opListManager;
    command::CommandRegistry m_commandRegistry;
    command::CommandStorage m_commandStorage;
    server::ServerScoreboard m_scoreboard;
    loot::LootPredicateManager m_predicateManager;
    function::FunctionManager m_functionManager;
    function::TimerQueue m_functionTimerQueue;
    std::vector<std::shared_ptr<FakeServerConnection>> m_connections;
};

} // namespace mc::test
