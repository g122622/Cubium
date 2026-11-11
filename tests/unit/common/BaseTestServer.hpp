#pragma once

#include "common/item/loot/LootPredicateManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/access/AccessManager.hpp"
#include "server/access/base/BannedIpListTypes.hpp"
#include "server/access/base/BannedPlayerListTypes.hpp"
#include "server/access/base/OpListManagerTypes.hpp"
#include "server/access/base/WhitelistManagerTypes.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/function/TimerQueue.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/network/base/IServerClientConnection.hpp"
#include "server/network/outbound/PacketSender.hpp"
#include "server/network/session/PlayerConnectionManager.hpp"
#include "server/network/session/keepalive/KeepAliveManager.hpp"
#include "server/network/sync/player/TeleportManager.hpp"
#include "server/player/gamemode/GameModeManager.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include "server/player/movement/PlayerMovementManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/time/TimeManager.hpp"

#include <memory>
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
    [[nodiscard]] Result<void> sendShared(std::shared_ptr<const mc::network::pipeline::OutboundPacket> packet) override;

    void close() override
    {
        m_connected = false;
    }

    [[nodiscard]] bool isConnected() const noexcept override
    {
        return m_connected;
    }

    void disconnect(const std::string& reason) override;

    [[nodiscard]] std::string peerAddress() const override
    {
        return {};
    }

    [[nodiscard]] size_t sentBytes() const noexcept
    {
        return m_sentData.size();
    }

    [[nodiscard]] const std::string& disconnectReason() const noexcept
    {
        return m_disconnectReason;
    }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<u8> m_sentData;
};

class BaseTestServer : public server::IServer {
public:
    BaseTestServer();
    ~BaseTestServer() override = default;

    [[nodiscard]] Result<void> initialize() override
    {
        return Result<void>::ok();
    }

    void shutdown() override
    {
        m_running = false;
    }

    void tick() override {}

    [[nodiscard]] bool isRunning() const override
    {
        return m_running;
    }

    [[nodiscard]] bool isIntegrated() const noexcept override
    {
        return false;
    }

    [[nodiscard]] bool isDedicated() const noexcept override
    {
        return true;
    }

    [[nodiscard]] server::AccessManager& accessManager() override
    {
        return m_accessManager;
    }

    [[nodiscard]] const server::AccessManager& accessManager() const override
    {
        return m_accessManager;
    }

    [[nodiscard]] server::PlayerManager& playerManager() override
    {
        return m_playerManager;
    }

    [[nodiscard]] const server::PlayerManager& playerManager() const override
    {
        return m_playerManager;
    }

    [[nodiscard]] server::PlayerConnectionManager& playerConnections() override
    {
        return m_playerConnections;
    }

    [[nodiscard]] const server::PlayerConnectionManager& playerConnections() const override
    {
        return m_playerConnections;
    }

    [[nodiscard]] server::PacketSender& packetSender() override
    {
        return m_packetSender;
    }

    [[nodiscard]] const server::PacketSender& packetSender() const override
    {
        return m_packetSender;
    }

    [[nodiscard]] server::TimeManager& timeManager() override
    {
        return m_timeManager;
    }

    [[nodiscard]] const server::TimeManager& timeManager() const override
    {
        return m_timeManager;
    }

    [[nodiscard]] server::TeleportManager& teleportManager() override
    {
        return m_teleportManager;
    }

    [[nodiscard]] const server::TeleportManager& teleportManager() const override
    {
        return m_teleportManager;
    }

    [[nodiscard]] server::KeepAliveManager& keepAliveManager() override
    {
        return m_keepAliveManager;
    }

    [[nodiscard]] const server::KeepAliveManager& keepAliveManager() const override
    {
        return m_keepAliveManager;
    }

    [[nodiscard]] server::PlayerMovementManager& playerMovementManager() override
    {
        return m_playerMovementManager;
    }

    [[nodiscard]] const server::PlayerMovementManager& playerMovementManager() const override
    {
        return m_playerMovementManager;
    }

    [[nodiscard]] server::GameModeManager& gameModeManager() override
    {
        return m_gameModeManager;
    }

    [[nodiscard]] const server::GameModeManager& gameModeManager() const override
    {
        return m_gameModeManager;
    }

    [[nodiscard]] ServerDimensionManager& dimensionManager() override;
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override;
    [[nodiscard]] server::ServerWorld* getPlayerWorld(PlayerId playerId) override;

    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override
    {
        return m_playerEntityManager;
    }

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

    [[nodiscard]] server::interaction::InventoryManager& inventoryManager() override
    {
        return m_inventoryManager;
    }

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

    [[nodiscard]] command::CommandRegistry& commandRegistry() override
    {
        return m_commandRegistry;
    }

    [[nodiscard]] const command::CommandRegistry& commandRegistry() const override
    {
        return m_commandRegistry;
    }

    [[nodiscard]] command::CommandStorage& commandStorage() override
    {
        return m_commandStorage;
    }

    [[nodiscard]] const command::CommandStorage& commandStorage() const override
    {
        return m_commandStorage;
    }

    [[nodiscard]] resource::DataPackRepository& dataPackList() override;
    [[nodiscard]] const resource::DataPackRepository& dataPackList() const override;
    [[nodiscard]] loot::LootTableManager& lootTableManager() override;
    [[nodiscard]] const loot::LootTableManager& lootTableManager() const override;

    [[nodiscard]] loot::LootPredicateManager& predicateManager() override
    {
        return m_predicateManager;
    }

    [[nodiscard]] const loot::LootPredicateManager& predicateManager() const override
    {
        return m_predicateManager;
    }

    [[nodiscard]] function::FunctionManager& functionManager() override
    {
        return m_functionManager;
    }

    [[nodiscard]] const function::FunctionManager& functionManager() const override
    {
        return m_functionManager;
    }

    [[nodiscard]] function::TimerQueue& functionTimerQueue() override
    {
        return m_functionTimerQueue;
    }

    [[nodiscard]] const function::TimerQueue& functionTimerQueue() const override
    {
        return m_functionTimerQueue;
    }

    [[nodiscard]] world::storage::SingleLevelStorageManager* sharedStorage() override
    {
        return nullptr;
    }

    [[nodiscard]] const world::storage::SingleLevelStorageManager* sharedStorage() const override
    {
        return nullptr;
    }

    [[nodiscard]] bool isSharedStorageReadonlyForeignWorld() const override
    {
        return false;
    }

    // 测试桩不承载真实世界数据：落盘退化为空操作，自动保存仅记录开关状态。
    [[nodiscard]] Result<size_t> saveAllWorldData(bool /*sync*/) override
    {
        return size_t{0};
    }

    void setAutoSaveEnabled(bool enabled) override
    {
        m_autoSaveEnabled = enabled;
    }

    [[nodiscard]] bool isAutoSaveEnabled() const override
    {
        return m_autoSaveEnabled;
    }

    [[nodiscard]] server::ServerScoreboard& scoreboard() override
    {
        return m_scoreboard;
    }

    [[nodiscard]] const server::ServerScoreboard& scoreboard() const override
    {
        return m_scoreboard;
    }

    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override;
    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override;

    [[nodiscard]] i32 viewDistance() const override
    {
        return m_viewDistance;
    }

    [[nodiscard]] i32 simulationDistance() const override
    {
        return m_simulationDistance;
    }

    [[nodiscard]] i32 maxPlayers() const override
    {
        return m_maxPlayers;
    }

    [[nodiscard]] u64 seed() const override
    {
        return m_seed;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return m_timeManager.currentTick();
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return m_difficulty;
    }

    void setDifficulty(Difficulty difficulty) override
    {
        m_difficulty = difficulty;
    }

    [[nodiscard]] bool isDifficultyLocked() const noexcept override
    {
        return m_difficultyLocked;
    }

    void setDifficultyLocked(bool locked) override
    {
        m_difficultyLocked = locked;
    }

    [[nodiscard]] bool isSingleplayerOwner(PlayerId /*playerId*/) const noexcept override
    {
        return false;
    }

    [[nodiscard]] i32 spawnProtectionRadius() const noexcept override
    {
        return 0;
    }

    [[nodiscard]] GameMode defaultGameMode() const override
    {
        return m_defaultGameMode;
    }

    void setDefaultGameMode(GameMode mode) override
    {
        m_defaultGameMode = mode;
    }

    [[nodiscard]] i32 playerIdleTimeoutMinutes() const override
    {
        return m_idleTimeoutMinutes;
    }

    void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) override
    {
        m_idleTimeoutMinutes = timeoutMinutes;
    }

    bool startConfigurationForPlayer(PlayerId /*playerId*/) override
    {
        return false;
    }

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

    void setPlayerWorld(server::ServerWorld* world)
    {
        m_playerWorld = world;
    }

    [[nodiscard]] bool stopRequested() const noexcept
    {
        return m_stopRequested;
    }

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
    bool m_autoSaveEnabled = true;
    bool m_running = true;
    Difficulty m_difficulty = Difficulty::Normal;
    bool m_difficultyLocked = false;
    GameMode m_defaultGameMode = GameMode::Survival;
    i32 m_idleTimeoutMinutes = 0;
    bool m_stopRequested = false;

    server::ServerWorld* m_playerWorld = nullptr;
    server::PlayerManager m_playerManager;
    server::AccessManager m_accessManager;
    server::interaction::InventoryManager m_inventoryManager;
    /// 测试玩家的物品栏（按 playerId）。生产环境的权威数据在玩家实体上，测试不构造实体，
    /// 故由本表持有并经解析器暴露给 m_inventoryManager——每个玩家仍只有一份。
    std::unordered_map<PlayerId, std::unique_ptr<PlayerInventory>> m_testInventories;
    // 真实空玩家实体管理器：默认构造无依赖（mutex + 两个空 map），getPlayerIds() 返回空 vector、
    // getPlayerEntity() 返回 nullptr。命令测试经 PlayerResolver::getSortedPlayerIds 调
    // playerEntityManager().getPlayerIds()，须返回空对象而非抛 "unused"（原 throwUnused 桩致
    // PlayerResolverTest/SpawnPointCommandTest/TellRawCommandTest 共 16 用例崩溃）。
    server::ServerPlayerEntityManager m_playerEntityManager;
    server::PacketSender m_packetSender;
    server::PlayerConnectionManager m_playerConnections{m_playerManager};
    server::TimeManager m_timeManager;
    server::PlayerMovementManager m_playerMovementManager;
    server::TeleportManager m_teleportManager;
    server::KeepAliveManager m_keepAliveManager;
    server::GameModeManager m_gameModeManager;

    command::CommandRegistry m_commandRegistry;
    command::CommandStorage m_commandStorage;
    server::ServerScoreboard m_scoreboard;

    // 维度管理器：测试桩原先对 dimensionManager() 直接抛 "unused"，导致任何需要
    // ServerCommandSource::world() 的用例都无法验证正向路径（/execute if biome、
    // /spreadplayers 实体移动等）。这里提供一个真实实例，用 nullptr 构造——它只在
    // 被显式 initialize() 时才会解引用内部 server 指针，测试里不调用 initialize 即安全。
    // 用例可自行 registerDimension(...) 装入 ServerDimension + 世界。
    ServerDimensionManager m_dimensionManager{nullptr};
    loot::LootPredicateManager m_predicateManager;
    function::FunctionManager m_functionManager;
    function::TimerQueue m_functionTimerQueue;
    std::vector<std::shared_ptr<FakeServerConnection>> m_connections;
};

} // namespace mc::test
