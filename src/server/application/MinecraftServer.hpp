/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "IServer.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/storage/GlobalStorageManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/advancement/AdvancementEventHandler.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/function/TimerQueue.hpp"
#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/interaction/MiningManager.hpp"
#include "server/registry/RegistryBootstrap.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace mc {
class BlockPos;
enum class ContainerType : u8;
namespace particle {
enum class ParticleTypeId : u16;
}
namespace command {
class CommandRegistry;
}
namespace gameevent {
class PositionSource;
}
namespace network::ir::play {
struct LevelChunkWithLight;
}
} // namespace mc

namespace mc::server {

// 前向声明
class ServerWorld;
class ServerChunkManager;
class ServerScriptManager;

namespace sync {
class WeatherSyncService;
} // namespace sync

namespace net {
class ServerClientConnection;
class PlayerBroadcaster;
class LoginFlow;
class ServerPlayHandler;
} // namespace net

/**
 * @brief 服务端调试统计信息
 *
 * 使用原子变量实现线程安全的服务端统计读取。
 * 服务端线程写入，客户端线程读取，无需加锁。
 */
struct ServerDebugStats {
    std::atomic<f32> smoothedTickTimeMs{0.0f}; ///< 平滑后的tick耗时（毫秒）
    std::atomic<f32> targetMsPerTick{50.0f};   ///< 目标每tick毫秒数
    std::atomic<i32> forcedChunkCount{0};      ///< 当前维度的强制加载区块数
    std::atomic<bool> isSprinting{false};      ///< 服务端是否正在加速执行
};

/**
 * @brief Minecraft 服务器抽象基类
 *
 * 提供所有服务器类型的共享实现：
 * - 持有所有核心 Manager
 * - 实现 tick 主循环框架
 * - 提供对 Manager 的直接访问
 *
 * 子类只需实现：
 * - 网络层（经 ServerNetwork：Local 模式或 Wire/TCP accept）
 * - 特定的数据包处理
 */
class MinecraftServer : public IServer {
public:
    /**
     * @brief 构造函数
     * @param settings 服务端设置引用
     */
    explicit MinecraftServer(ServerSettings& settings);

    /**
     * @brief 析构函数
     */
    ~MinecraftServer() override;

    // 移动操作
    MinecraftServer(MinecraftServer&&) noexcept = default;
    MinecraftServer& operator=(MinecraftServer&&) noexcept = default;

    // 禁止拷贝
    MinecraftServer(const MinecraftServer&) = delete;
    MinecraftServer& operator=(const MinecraftServer&) = delete;

    // ========== IServer 接口实现 ==========

    [[nodiscard]] bool isRunning() const noexcept override { return m_running.load(); }
    void shutdown() override;
    void tick() override;

    // ========== 核心管理器 ==========

    [[nodiscard]] core::PlayerManager& playerManager() override { return *m_playerManager; }
    [[nodiscard]] const core::PlayerManager& playerManager() const override { return *m_playerManager; }

    [[nodiscard]] core::ConnectionManager& connectionManager() override { return *m_connectionManager; }
    [[nodiscard]] const core::ConnectionManager& connectionManager() const override { return *m_connectionManager; }

    [[nodiscard]] core::TimeManager& timeManager() override { return *m_timeManager; }
    [[nodiscard]] const core::TimeManager& timeManager() const override { return *m_timeManager; }

    [[nodiscard]] core::TeleportManager& teleportManager() override { return *m_teleportManager; }
    [[nodiscard]] const core::TeleportManager& teleportManager() const override { return *m_teleportManager; }

    [[nodiscard]] core::KeepAliveManager& keepAliveManager() override { return *m_keepAliveManager; }
    [[nodiscard]] const core::KeepAliveManager& keepAliveManager() const override { return *m_keepAliveManager; }

    [[nodiscard]] core::PositionTracker& positionTracker() override { return *m_positionTracker; }
    [[nodiscard]] const core::PositionTracker& positionTracker() const override { return *m_positionTracker; }

    [[nodiscard]] core::GameModeManager& gameModeManager() override { return *m_gameModeManager; }
    [[nodiscard]] const core::GameModeManager& gameModeManager() const override { return *m_gameModeManager; }

    // ========== 白名单管理器 ==========

    [[nodiscard]] core::WhitelistManager& whitelistManager() override { return *m_whitelistManager; }
    [[nodiscard]] const core::WhitelistManager& whitelistManager() const override { return *m_whitelistManager; }

    // ========== 封禁管理器 ==========

    [[nodiscard]] core::BannedPlayerList& bannedPlayerList() override { return *m_bannedPlayerList; }
    [[nodiscard]] const core::BannedPlayerList& bannedPlayerList() const override { return *m_bannedPlayerList; }

    [[nodiscard]] core::BannedIpList& bannedIpList() override { return *m_bannedIpList; }
    [[nodiscard]] const core::BannedIpList& bannedIpList() const override { return *m_bannedIpList; }

    // ========== OP 管理器 ==========

    [[nodiscard]] core::OpListManager& opListManager() override { return *m_opListManager; }
    [[nodiscard]] const core::OpListManager& opListManager() const override { return *m_opListManager; }

    // ========== 世界访问（维度感知）==========

    [[nodiscard]] ServerWorld* getPlayerWorld(PlayerId playerId) override;

    // ========== 维度管理器 ==========

    [[nodiscard]] ServerDimensionManager& dimensionManager() override { return *m_dimensionManager; }
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override { return *m_dimensionManager; }

    // ========== 天气同步服务 ==========
    [[nodiscard]] sync::WeatherSyncService& weatherSyncService() { return *m_weatherSyncService; }
    [[nodiscard]] const sync::WeatherSyncService& weatherSyncService() const { return *m_weatherSyncService; }

    // ========== 玩家广播门面 ==========
    [[nodiscard]] net::PlayerBroadcaster& broadcaster() { return *m_broadcaster; }
    [[nodiscard]] const net::PlayerBroadcaster& broadcaster() const { return *m_broadcaster; }

    // ========== 登录流程门面（批6 下沉） ==========
    [[nodiscard]] net::LoginFlow& loginFlow() { return *m_loginFlow; }
    [[nodiscard]] const net::LoginFlow& loginFlow() const { return *m_loginFlow; }

    // ========== Play 包处理门面（批7 下沉） ==========
    [[nodiscard]] net::ServerPlayHandler& playHandler() { return *m_playHandler; }
    [[nodiscard]] const net::ServerPlayHandler& playHandler() const { return *m_playHandler; }

    // ========== 玩家实体管理 ==========

    /**
     * @brief 获取玩家实体管理器
     *
     * 用于管理玩家的实体对象（Player实例）。批2b 已将 m_playerEntityManager 上提为
     * MinecraftServer 值成员，本方法不再纯虚；子类无需 override。
     */
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    // ========== 交互管理器 ==========

    [[nodiscard]] interaction::BlockInteractionManager& blockInteractionManager() override
    {
        return *m_blockInteractionManager;
    }
    [[nodiscard]] const interaction::BlockInteractionManager& blockInteractionManager() const override
    {
        return *m_blockInteractionManager;
    }

    [[nodiscard]] interaction::MiningManager& miningManager() override { return *m_miningManager; }
    [[nodiscard]] const interaction::MiningManager& miningManager() const override { return *m_miningManager; }

    [[nodiscard]] interaction::ContainerManager& containerManager() override { return *m_containerManager; }
    [[nodiscard]] const interaction::ContainerManager& containerManager() const override { return *m_containerManager; }

    [[nodiscard]] interaction::InventoryManager& inventoryManager() override { return *m_inventoryManager; }
    [[nodiscard]] const interaction::InventoryManager& inventoryManager() const override { return *m_inventoryManager; }

    // ========== 调试统计 ==========

    /**
     * @brief 获取服务端调试统计信息
     *
     * 原子变量，可从客户端线程安全读取。
     */
    [[nodiscard]] const ServerDebugStats& debugStats() const { return m_debugStats; }

    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override;
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override;

    // ========== 命令系统 ==========

    [[nodiscard]] mc::command::CommandRegistry& commandRegistry() override { return *m_commandRegistry; }
    [[nodiscard]] const mc::command::CommandRegistry& commandRegistry() const override { return *m_commandRegistry; }

    [[nodiscard]] mc::command::CommandStorage& commandStorage() override { return *m_commandStorage; }
    [[nodiscard]] const mc::command::CommandStorage& commandStorage() const override { return *m_commandStorage; }

    // ========== 记分板系统 ==========

    [[nodiscard]] ServerScoreboard& scoreboard() override { return *m_scoreboard; }
    [[nodiscard]] const ServerScoreboard& scoreboard() const override { return *m_scoreboard; }

    // ========== Boss 栏系统 ==========

    [[nodiscard]] CustomServerBossInfoManager& bossBarManager() override { return *m_bossBarManager; }
    [[nodiscard]] const CustomServerBossInfoManager& bossBarManager() const override { return *m_bossBarManager; }

    // ========== 脚本系统 ==========

    [[nodiscard]] ServerScriptManager* scriptManager() { return m_scriptManager.get(); }
    [[nodiscard]] const ServerScriptManager* scriptManager() const { return m_scriptManager.get(); }

    // ========== 配置 ==========

    [[nodiscard]] i32 viewDistance() const override { return m_settings.viewDistance.get(); }
    [[nodiscard]] i32 maxPlayers() const override { return m_settings.maxPlayers.get(); }
    [[nodiscard]] u64 seed() const override { return m_settings.parseSeed(); }
    [[nodiscard]] u64 currentTick() const override;
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty difficulty) override;
    [[nodiscard]] GameMode defaultGameMode() const override
    {
        return static_cast<GameMode>(m_settings.defaultGameMode.get());
    }
    void setDefaultGameMode(GameMode mode) override;
    [[nodiscard]] i32 playerIdleTimeoutMinutes() const override { return m_playerIdleTimeoutMinutes; }
    void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) override;
    void requestStop() override;

    // ========== 便捷方法 ==========

    /**
     * @brief 获取服务端设置
     */
    [[nodiscard]] ServerSettings& settings() { return m_settings; }
    [[nodiscard]] const ServerSettings& settings() const { return m_settings; }

    /**
     * @brief 获取计算型 Worker 池
     */
    [[nodiscard]] util::UniversalWorkerPool& computationWorkerPool() { return m_computationWorkerPool; }
    [[nodiscard]] const util::UniversalWorkerPool& computationWorkerPool() const { return m_computationWorkerPool; }

    /**
     * @brief 获取 IO Worker 池
     */
    [[nodiscard]] util::UniversalWorkerPool& ioWorkerPool() { return m_ioWorkerPool; }
    [[nodiscard]] const util::UniversalWorkerPool& ioWorkerPool() const { return m_ioWorkerPool; }

    /**
     * @brief 遍历所有玩家
     */
    template <typename Func>
    void forEachPlayer(Func&& func);

    template <typename Func>
    void forEachPlayer(Func&& func) const;

    /**
     * @brief 获取掉落表管理器
     */
    [[nodiscard]] mc::loot::LootTableManager& lootTableManager() override { return m_lootTableManager; }
    [[nodiscard]] const mc::loot::LootTableManager& lootTableManager() const override { return m_lootTableManager; }

    [[nodiscard]] mc::function::FunctionManager& functionManager() override { return m_functionManager; }
    [[nodiscard]] const mc::function::FunctionManager& functionManager() const override { return m_functionManager; }

    [[nodiscard]] mc::function::TimerQueue& functionTimerQueue() override { return m_functionTimerQueue; }
    [[nodiscard]] const mc::function::TimerQueue& functionTimerQueue() const override { return m_functionTimerQueue; }
    [[nodiscard]] mc::loot::LootPredicateManager& predicateManager() override { return m_predicateManager; }
    [[nodiscard]] const mc::loot::LootPredicateManager& predicateManager() const override { return m_predicateManager; }
    [[nodiscard]] world::storage::SingleLevelStorageManager* sharedStorage() override { return m_storage.get(); }
    [[nodiscard]] const world::storage::SingleLevelStorageManager* sharedStorage() const override
    {
        return m_storage.get();
    }
    [[nodiscard]] bool isSharedStorageReadonlyForeignWorld() const override;
    [[nodiscard]] PackRepository& resourcePackList() { return m_resourcePackList; }
    [[nodiscard]] const PackRepository& resourcePackList() const { return m_resourcePackList; }
    [[nodiscard]] mc::resource::DataPackRepository& dataPackList() override { return m_dataPackList; }
    [[nodiscard]] const mc::resource::DataPackRepository& dataPackList() const override { return m_dataPackList; }

    /**
     * @brief 序列化难度同步包
     * @return 难度同步 IR 包
     *
     * 提升为 public：LoginFlow 门面在 sendInitialDifficultyToPlayer 中调用
     * （批6 登录序列下沉）。
     */
    [[nodiscard]] mc::network::ir::IrPacket serializeDifficultyPacket();

    /**
     * @brief 解析玩家在命令分发时使用的权限等级。
     *
     * 默认实现直接返回 OP 列表中的等级；集成服务器可 override 在此之上叠加
     * 单机主机作弊提升（运行时判定，不写 ops.json）。
     *
     * 提升为 public：LoginFlow 门面在 createPlayerForConnection 中调用
     * （批6 登录序列下沉）。保留 virtual 维持子类多态。
     *
     * @param uuid 玩家 UUID。
     * @return 命令分发所用的权限等级 (0-4)。
     */
    [[nodiscard]] virtual i32 resolveOpLevel(const std::string& uuid) const noexcept;

protected:
    void attachWorldBindings(ServerWorld& world);
    void attachWorldCommandBindings(ServerWorld& world);
    [[nodiscard]] Result<void> initializeSharedStorage(const GameDirectory& gameDirectory, const std::string& levelId);
    [[nodiscard]] Result<size_t> saveAllWorldData();
    void shutdownSharedStorage();

    /**
     * @brief 回写所有在线玩家运行时状态到 PlayerDataManager 缓存
     *
     * 在 shutdownManagers() 中 saveAllWorldData() 之前调用。子类应遍历自己的
     * ServerPlayerEntityManager，对每个在线 Player 调用
     * PlayerDataManager::fromPlayer() + savePlayer()，把位置、生命、饥饿、经验、
     * 背包等运行时状态灌入缓存。后续 saveAllWorldData() 会通过 PlayerDataManager::saveAll()
     * 把缓存落盘到 RocksDB。
     *
     * 默认实现为空，因为基类无法访问 playerEntityManager()（纯虚）。子类必须 override
     * 此方法以提供具体的遍历逻辑。
     *
     * @note 调用时机：必须在维度管理器 shutdown 之前、玩家实体被 clearAll 之前调用。
     *       readonly foreign world 场景下不会被调用（shutdownManagers 已判断）。
     *
     * 批2b 已将本方法下沉为 MinecraftServer 基类实现（遍历已上提的 m_playerEntityManager），
     * 子类不再 override。保留 virtual 仅供测试桩（SaveStateSpyServer）spy 调用时机。
     */
    virtual void savePlayerRuntimeState();

    /**
     * @brief 初始化核心管理器
     */
    void initializeCoreManagers();

    /**
     * @brief 初始化世界
     */
    [[nodiscard]] Result<void> initializeWorld();

    /**
     * @brief 初始化交互管理器
     */
    void initializeInteractionManagers();

    /**
     * @brief 初始化同步管理器
     */
    void initializeSyncManagers();

    /**
     * @brief 初始化区块同步管理器（在 world 初始化后调用）
     */
    void initializeChunkSyncManagers();

    /**
     * @brief 初始化游戏注册表（方块、物品、附魔、配方）
     *
     * 薄化转调：实际装配职责已下沉到 RegistryBootstrap 门面（见
     * server/registry/RegistryBootstrap.hpp）。本方法仅构造门面并转调
     * initializeAll(registerEntities)，保持调用顺序与原行为逐字节一致。
     *
     * 加载所有 Vanilla 注册表：方块/物品/附魔/方块物品/物品标签/配方/战利品/
     * 函数/进度/worldgen 全链路/生物群系/实体类型（可选）/Java wire id 映射。
     *
     * @param registerEntities 是否注册实体类型（独立服 true，集成服 false）
     */
    void initializeRegistries(bool registerEntities = false);

    /**
     * @brief 设置世界回调
     *
     * 设置区块加载回调、实体生成回调、光照变化回调
     */
    void setupWorldCallbacks();

    /**
     * @brief 广播难度变更给所有玩家
     */
    void broadcastDifficultyChange();

    /**
     * @brief 关闭所有管理器
     */
    void shutdownManagers();

    /**
     * @brief 处理世界层的开容器请求
     */
    [[nodiscard]] virtual bool openContainerRequest(ContainerType type, const BlockPos& pos, Player& player);
    /**
     * @brief 执行实体 tick
     */
    void tickEntities();

    /**
     * @brief 执行心跳检查
     */
    void tickKeepAlive();

    /**
     * @brief 发送时间更新
     */
    void sendTimeUpdate();

    // 天气同步已下沉到 WeatherSyncService（server/sync/WeatherSyncService）。
    // tick 中调 m_weatherSyncService->tick()，登录时调其 sendInitialWeatherStateToPlayer。
    // 注：sendInitialDifficultyToPlayer 已于批6 迁入 LoginFlow。

    /**
     * @brief 发送心跳给所有玩家
     */
    void sendKeepAliveToAll();

    /**
     * @brief 填充创造模式初始物品栏
     */
    void initializeCreativeInventory(PlayerInventory& inventory);

    /**
     * @brief 设置区块发送/卸载回调
     */
    void setupChunkSendCallback();

    /**
     * @brief 设置袭击事件回调
     */
    void setupRaidManagerCallbacks();

    /**
     * @brief 为末地维度的 EndDragonFight 注入服务端 Boss 栏
     *
     * 在维度初始化完成后调用，从 dimensionManager 获取末地维度的 ServerWorld，
     * 为其 EndDragonFight 创建并注入 ServerDragonBossBar，使末影龙 Boss 栏
     * 能通过网络包同步到客户端。
     */
    void setupDragonFightBossBar();

    /**
     * @brief 发送区块数据给指定玩家
     *
     * IR 已由 ChunkSendManager 在 worker 线程构建（buildLevelChunkWithLightIR），此处仅按玩家
     * 拷贝进 IrPacket 发送。本地客户端经 LocalTransport 零拷贝直传 IR，远程 Java 客户端经
     * JavaBackend→levelChunkWithLightCodec 编码成 vanilla wire。
     */
    void sendChunkDataToPlayer(
        PlayerId playerId, ChunkCoord x, ChunkCoord z, const mc::network::ir::play::LevelChunkWithLight& ir);

    /**
     * @brief 发送卸载区块通知给指定玩家
     */
    void sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    /**
     * @brief 广播光照更新给所有在线玩家
     */
    void broadcastLightUpdate(ChunkCoord x,
        ChunkCoord z,
        i32 sectionY,
        const std::vector<u8>& skyLight,
        const std::vector<u8>& blockLight,
        bool trustEdges);

    /**
     * @brief 轮询网络事件
     *
     * 批2a 统一为基类默认实现：m_serverNetwork->tick()（drain Local + Wire 入站 + 派发
     * 延迟断开）+ _drainDisconnectedSessions()。两子类原实现完全一致，已删除 override。
     * IntegratedServer 的本地客户端（sessionId=0）入站经 m_serverNetwork 的 Local 通道
     * 一并 tick，无需子类特化。
     */
    virtual void pollNetwork();

    /**
     * @brief 广播 IR 包给所有连接的玩家
     *
     * 新网络层（1.21.11 IR）：游戏逻辑直接构造 ir::IrPacket 交由本方法，经各
     * ServerClientConnection::send 出站。作为发送原语公开：WeatherSyncService/
     * PlayerBroadcaster 等服务端子系统经此广播 IR 包，无需 friend。
     *
     * 批2a 统一为基类默认实现：若注入了本地客户端钩子（m_localClientSender），
     * 先经钩子发本地客户端，再 forEachPlayer 遍历远程玩家时跳过 localPlayerId
     * 避免双发；否则纯 forEachPlayer 广播。StandaloneServer 不注入钩子（纯远程），
     * IntegratedServer 在 initialize 注入（LocalTransport 零拷贝）。
     */
public:
    virtual void broadcastPacket(const mc::network::ir::IrPacket& packet);

protected:
    /**
     * @brief 将会话ID转换为玩家ID
     *
     * 批2a 统一为基类默认实现：sessionId == 0 且注入了本地客户端钩子
     * （m_localClientPlayerId）时返回本地客户端玩家ID；否则走 PlayerManager 查询。
     */
    [[nodiscard]] virtual PlayerId getPlayerIdForSession(u32 sessionId) const;

public:
    /**
     * @brief 向指定玩家发送 IR 包
     *
     * 作为发送原语公开：ServerDimensionManager 等服务端子系统经此向玩家下发
     * 维度切换/区块等 IR 包，无需 friend。
     *
     * 批2a 统一为基类默认实现：playerId == localPlayerId 且注入了本地客户端钩子
     * （m_localClientSender）时经钩子发本地客户端（LocalTransport 零拷贝）；
     * 否则走 ServerPlayerData::send → ServerClientConnection::send。
     */
    virtual void sendPacketToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet);

protected:
    /**
     * @brief 清理已断开的远程会话（批2a 提升为基类 virtual）
     *
     * 当前为空默认：断开已由 ServerNetwork::tick() 在主线程回调 _onRemoteClientDisconnect
     * 处理，session map / 玩家清理均在该回调内完成。保留 virtual 供批2c RemoteSessionManager
     * 注入实际清理逻辑。
     */
    virtual void _drainDisconnectedSessions() {}

public:
    // 注：updateEntityTrackingForPlayer/routeInboundPlayPacket 及 13 个非纯虚 handle*Packet
    // 已于批7 下沉至 net::ServerPlayHandler 门面（见 server/network/ServerPlayHandler.hpp）。
    // 经 playHandler() 访问门面：ServerPlayRouter::handle 调 route，登录/维度切换调
    // updateEntityTrackingForPlayer。

protected:
    // 注：sendCommandTreePacket/sendPermissionLevelChange 已于批6 迁入 LoginFlow
    // （登录流程整簇下沉）。/op、/deop 命令走 buildPermissionLevelChangeIr + connectionManager。

    // ========== 数据包处理方法 ==========
    //
    // 注：routeInboundPlayPacket/dispatchPacket 及 13 个非纯虚 handle*Packet（移动/传送确认/
    // 心跳/聊天/告示牌/骑乘输入/载具移动/玩家命令/船桨/实体交互/物品使用/方块交互/方块放置）
    // 已于批7 下沉至 net::ServerPlayHandler 门面。dispatchPacket 为死代码已删。
    //
    // 下列 4 个虚 handle 保留在 MinecraftServer：3 个纯虚（handleHotbarSelect/
    // handleContainerClick/handleCloseContainer）+ handleOpenPlayerInventoryPacket（虚，子类覆写）。
    // 它们读子类私有状态（m_clientInventory/m_openMenu/m_clientPlayerId），不可下沉；门面
    // route 内对应分支经 m_server.handleXxxPacket(...) 虚分发到子类 override，保留多态。

public:
    /**
     * @brief 处理快捷栏选择数据包（子类实现特定逻辑）
     *
     * 提升为 public：net::ServerPlayHandler::route 经 m_server 虚分发调用（批7）。
     */
    virtual void handleHotbarSelectPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) = 0;

    /**
     * @brief 处理容器点击数据包（子类实现特定逻辑）
     *
     * 提升为 public：net::ServerPlayHandler::route 经 m_server 虚分发调用（批7）。
     */
    virtual void handleContainerClickPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) = 0;

    /**
     * @brief 处理关闭容器数据包（子类实现特定逻辑）
     *
     * 提升为 public：net::ServerPlayHandler::route 经 m_server 虚分发调用（批7）。
     */
    virtual void handleCloseContainerPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) = 0;

    /**
     * @brief 处理请求打开玩家背包容器数据包（子类覆写打开菜单）
     *
     * 提升为 public：net::ServerPlayHandler::handlePlayerCommandPacket 的 OPEN_INVENTORY
     * 分支经 m_server 虚分发调用（批7），保留 IntegratedServer 开背包覆写。
     */
    virtual void handleOpenPlayerInventoryPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

protected:
    // 注：登录请求不再经 dispatchPacket 入站。新网络层登录全由 ServerHandshakeStateMachine
    // 驱动（ClientIntention→Hello→LoginFinished→LoginAcknowledged→Configuration→Play），
    // 玩家创建在各子类注册的 onPlayerReady 回调中完成（Configuration 结束后触发）。

    // ========== 数据包发送辅助方法 ==========

    /**
     * @brief 广播光照更新给相关玩家（子类实现）
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param sectionY 区块段Y坐标
     * @param skyLight 天空光照数据
     * @param blockLight 方块光照数据
     * @param trustEdges 是否信任边缘光照
     */
    /**
     * @brief 发送传送包给指定玩家
     */
    void sendTeleportPacket(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);

    /**
     * @brief 处理玩家创造模式切换后的额外同步
     */
    virtual void onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory);

    /**
     * @brief 返回玩家当前手持物品
     *
     * 提升为 public：net::ServerPlayHandler::handleBlockPlacementPacket 调用（批7）。
     */
public:
    [[nodiscard]] virtual ItemStack getHeldItemForPlacement(PlayerId playerId) = 0;

    /**
     * @brief 返回玩家当前选中槽位
     *
     * 提升为 public：net::ServerPlayHandler::handleBlockPlacementPacket 调用（批7）。
     */
    [[nodiscard]] virtual i32 getSelectedHotbarSlot(PlayerId playerId) = 0;

    /**
     * @brief 设置玩家指定槽位物品
     *
     * 提升为 public：net::ServerPlayHandler::handleBlockPlacementPacket 调用（批7）。
     */
    virtual void setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack) = 0;

    /**
     * @brief 同步玩家物品栏到客户端
     *
     * 提升为 public：net::ServerPlayHandler::handleBlockPlacementPacket 调用（批7）。
     */
    virtual void syncPlayerInventory(PlayerId playerId) = 0;

    /**
     * @brief 尝试打开工作台容器
     *
     * 提升为 public：net::ServerPlayHandler::handleBlockPlacementPacket 调用（批7）。
     */
    [[nodiscard]] virtual bool tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos) = 0;

protected:
    /**
     * @brief 发送方块更新包给指定玩家
     */
    void sendBlockUpdatePacket(PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId);

    // 注：登录流程整簇（setupInitialPlayerState/sendInitialGameState/
    // createPlayerForConnection/sendLoginResponseForConnection + 配套 PlayerCreationResult）
    // 已于批6 迁入 LoginFlow 门面。调用方经 m_loginFlow->createPlayerForConnection 进入。

protected:
    // ========== 声音广播方法 ==========

    /**
     * @brief 广播声音给所有玩家
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 声音位置
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    void broadcastSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume = 1.0f,
        f32 pitch = 1.0f);

    /**
     * @brief 广播声音给指定范围内的玩家
     *
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 声音位置
     * @param range 广播范围（格）
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    void broadcastSoundInRange(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 range,
        f32 volume = 1.0f,
        f32 pitch = 1.0f);

    // ========== 粒子广播方法 ==========

    /**
     * @brief 广播粒子给指定范围内的玩家
     *
     * @param type 粒子类型
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastParticleInRange(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count,
        f32 range = 256.0f);

    /**
     * @brief 广播实体状态事件给范围内玩家
     *
     * @param entityId 实体ID
     * @param status 状态码
     * @param pos 实体位置
     * @param range 广播范围（格），默认 64 格
     */
    void broadcastEntityStatusInRange(EntityInstanceId entityId, u8 status, const Vector3& pos, f32 range = 64.0f);

    /**
     * @brief 广播实体动画事件给范围内玩家
     *
     * @param entityId 实体ID
     * @param animation 动画类型
     * @param pos 实体位置
     * @param range 广播范围（格），默认 64 格
     */
    void broadcastEntityAnimationInRange(
        EntityInstanceId entityId, u8 animation, const Vector3& pos, f32 range = 64.0f);

    /**
     * @brief 向范围内玩家广播实体受伤动画（携带 hurtDir）
     *
     * 发送 TakeDamage 动画包并附带 hurtDir，客户端据此设置 damageTilt 屏幕倾斜方向
     * （对应 MC ClientboundHurtAnimationPacket）。
     */
    void broadcastHurtAnimationInRange(EntityInstanceId entityId, f32 hurtDir, const Vector3& pos, f32 range = 64.0f);

    /**
     * @brief 向范围内玩家广播实体拴绳链接变更
     *
     * 创建 ir::play::SetEntityLink 并发送给指定位置附近的玩家，
     * 用于客户端拴绳绳索的渲染同步。
     *
     * @param entityId 被拴实体的ID
     * @param linkedEntityId 拴绳持有者实体ID（0=解除拴绳）
     * @param pos 被拴实体位置（用于确定广播范围）
     * @param range 广播范围（格），默认 64 格
     */
    void broadcastSetEntityLinkInRange(
        EntityInstanceId entityId, EntityInstanceId linkedEntityId, const Vector3& pos, f32 range = 64.0f);

    // ========== 世界事件广播方法 ==========

    /**
     * @brief 广播世界事件给所有玩家
     *
     * 世界事件包括音效和粒子效果，如门开关声音、唱片播放、方块破坏等。
     *
     * @param eventId 事件ID，参见 WorldEvents 命名空间
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param data 事件数据（含义因事件而异）
     */
    void broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data);

    /**
     * @brief 广播世界事件给指定范围内的玩家
     *
     * @param eventId 事件ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param data 事件数据
     * @param range 广播范围（格），默认 64 格
     */
    void broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range = 64.0f);

    /**
     * @brief 广播方块破坏进度动画给指定范围内的玩家
     *
     * 对应 MC Java 中的 ServerLevel.destroyBlockProgress()。
     * 发送 BlockBreakAnimPacket 给 32 格范围内的玩家（排除破坏者自身）。
     *
     * @param breakerId 破坏者实体ID
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param progress 破坏进度 (0-9 表示阶段，-1 表示移除动画)
     * @param range 广播范围（格），默认 32 格（MC Java 使用 1024 = 32^2）
     */
    void broadcastBlockBreakProgressInRange(
        EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress, f32 range = 32.0f);

    /**
     * @brief 广播方块事件给指定范围内的玩家
     *
     * 对应 MC Java 中的 ServerPlayerList.broadcast(null, x, y, z, 64.0, dimension, ClientboundBlockEventPacket)。
     * 发送 BlockEventPacket 给 64 格范围内的玩家。
     *
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param paramA 事件参数A
     * @param paramB 事件参数B
     * @param blockStateId 方块状态ID
     * @param range 广播范围（格），默认 64 格
     */
    void broadcastBlockEventInRange(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId, f32 range = 64.0f);

    /**
     * @brief 广播方块实体数据更新给指定范围内的玩家
     *
     * 当方块实体数据变化（如告示牌文本更新）时，将最新的 NBT 快照发送给
     * 附近客户端。对应 MC Java: PlayerList.broadcast(null, x, y, z, 64.0,
     * dimension, new ClientboundBlockEntityDataPacket(pos, type, tag))。
     *
     * 1.21.11 线格式：blockPosPacked + blockEntityType + CompoundTag（无长度前缀）。
     *
     * @param pos 方块位置
     * @param type 方块实体类型
     * @param tag NBT 复合标签快照（Java 版大端二进制；nullptr 表示空 NBT）
     * @param range 广播范围（格），默认 64 格
     */
    void broadcastBlockEntityInRange(
        const BlockPos& pos, BlockEntityType type, std::shared_ptr<nbt::CompoundTag> tag, f32 range = 64.0f);

    /**
     * @brief 发送粒子给指定玩家
     *
     * @param playerId 玩家ID
     * @param type 粒子类型
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     */
    void sendParticleToPlayer(PlayerId playerId,
        particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);

    /**
     * @brief 广播振动粒子给指定范围内的玩家
     *
     * 振动粒子需要携带目标位置来源和到达时间信息，与普通粒子包不同。
     * 使用 LevelParticles 的 ParticleOptions(Vibration) 编码，按 PositionSource 类型序列化：
     * - BlockPositionSource: VarInt(0) + i64 packedBlockPos
     * - EntityPositionSource: VarInt(1) + VarInt entityId + f32 yOffset
     *
     * @param pos 粒子起始位置（振动源位置）
     * @param targetSource 粒子飞向的目标位置来源（监听器位置来源）
     * @param arrivalInTicks 到达目标的 tick 数
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastVibrationParticleInRange(
        const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks, f32 range = 256.0f);

    /**
     * @brief 广播轨迹粒子给指定范围内的玩家
     *
     * 轨迹粒子需要携带目标位置、ARGB 颜色和飞行持续时间。
     * 主要用于眼眸花状态切换的转换粒子效果。
     *
     * @param pos 粒子起始位置
     * @param targetPosition 粒子飞向的目标位置
     * @param color 粒子颜色（ARGB 格式）
     * @param durationInTicks 飞行持续时间（tick 数）
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastTrailParticleInRange(
        const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks, f32 range = 256.0f);

    /**
     * @brief 广播带颜色的 EntityEffect 粒子给范围内玩家
     *
     * 用于 BellBlockEntity 共振等需要携带 ARGB 颜色的场景。
     *
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     * @param color 粒子颜色（ARGB 格式）
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastEntityEffectParticleInRange(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color, f32 range = 256.0f);

    /**
     * @brief 广播方块粒子（携带方块状态 ID）给范围内玩家
     *
     * 用于旋风人地面粒子、长跳轨迹粒子等需要方块纹理的场景。
     *
     * @param type 粒子类型（必须为 requiresBlockState 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param blockStateId 方块状态 ID（BlockState::stateId()）
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastBlockParticleInRange(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        u32 blockStateId,
        f32 range = 256.0f);

    /**
     * @brief 广播物品粒子（携带物品堆）给范围内玩家
     *
     * 用于物品破碎、史莱姆弹跳、雪球击中等需要物品纹理的场景。
     *
     * @param type 粒子类型（必须为 requiresItemData 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param itemStack 物品堆（用于粒子纹理）
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastItemParticleInRange(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const ItemStack& itemStack,
        f32 range = 256.0f);

    // ========== 爆炸广播方法 ==========

    /**
     * @brief 广播爆炸事件给范围内玩家
     *
     * @param position 爆炸位置
     * @param strength 爆炸威力（半径）
     * @param affectedBlocks 受影响的方块列表
     * @param playerKnockback 玩家击退映射（玩家ID -> 击退向量）
     * @param range 广播范围（格），默认 64.0f
     */
    void broadcastExplosionInRange(const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback,
        f32 range = 64.0f);

    /**
     * @brief 发送爆炸包给指定玩家
     *
     * @param playerId 玩家ID
     * @param position 爆炸位置
     * @param strength 爆炸威力
     * @param affectedBlocks 受影响的方块列表
     * @param playerKnockback 玩家击退映射
     */
    void sendExplosionToPlayer(PlayerId playerId,
        const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback);

    /**
     * @brief 停止核心组件
     *
     * 停止 Worker 线程、断开所有连接、关闭管理器。
     * 子类的 stop() 方法调用此方法。
     */
    void stopCore();

protected:
    ServerSettings& m_settings;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};
    Difficulty m_difficulty = Difficulty::Normal;
    i32 m_playerIdleTimeoutMinutes = 0;

    // 核心管理器
    std::unique_ptr<core::PlayerManager> m_playerManager;
    std::unique_ptr<core::ConnectionManager> m_connectionManager;
    std::unique_ptr<core::TimeManager> m_timeManager;
    std::unique_ptr<core::TeleportManager> m_teleportManager;
    std::unique_ptr<core::KeepAliveManager> m_keepAliveManager;
    std::unique_ptr<core::PositionTracker> m_positionTracker;
    std::unique_ptr<core::GameModeManager> m_gameModeManager;
    std::unique_ptr<core::WhitelistManager> m_whitelistManager;
    std::unique_ptr<core::BannedPlayerList> m_bannedPlayerList;
    std::unique_ptr<core::BannedIpList> m_bannedIpList;
    std::unique_ptr<core::OpListManager> m_opListManager;

    // 世界级共享存储
    world::storage::GlobalStorageManager m_globalStorage;
    std::unique_ptr<world::storage::SingleLevelStorageManager> m_storage;

    // 本次会话主世界出生点是否已就绪。
    // 新世界首次启动（level.dat initialized=false）经 initializeWorldSpawn 计算后置 true；
    // 老存档（initialized=true）启动时直接置 true。shutdown 时由 saveAllWorldData 写回 level.dat 的
    // initialized 字段。注意：与 ServerWorld::m_initialized（子系统就绪标志）语义无关，不可复用。
    bool m_spawnInitializedThisSession = false;

    // 维度管理器
    std::unique_ptr<ServerDimensionManager> m_dimensionManager;

    // 服务端网络门面（管理所有 ServerClientConnection + LocalTransportPair + 协议表）。
    // 批2b 上提自两子类私有成员：StandaloneServer 在 initialize() 创建并 startAccept；
    // IntegratedServer 在 initialize() 创建并 createLocalClientSide，publishToLan() 时
    // startAccept。销毁顺序：子类 m_remoteSessions 须先于本成员 reset（session 持
    // ServerClientConnection& 非拥有），由子类 stop() 显式 clear 保证。
    std::unique_ptr<mc::server::net::ServerNetwork> m_serverNetwork;

    // 玩家实体管理器（PlayerId↔EntityInstanceId 映射 + 实体池接入）。批2b 上提自两子类
    // 私有值成员，ServerPlayerEntityManager 无参默认构造可作基类值成员。
    ServerPlayerEntityManager m_playerEntityManager;

    // 本地客户端发送钩子（批2a 四纯虚统一用）。IntegratedServer 在 initialize() 创建本地
    // 客户端连接后注入：m_localClientPlayerId 设本地客户端 playerId，m_localClientSender
    // 绑定 _sendToClientIr（LocalTransport 零拷贝）。StandaloneServer 不注入（nullopt /
    // 空 function），保持纯远程行为。sendPacketToPlayer/broadcastPacket/getPlayerIdForSession
    // 基类默认实现据此判本地/远程路径。
    std::optional<PlayerId> m_localClientPlayerId;
    std::function<void(const mc::network::ir::IrPacket&)> m_localClientSender;

    // 天气同步服务（影子状态 + 主世界天气广播，下沉自 sendWeatherUpdate）
    std::unique_ptr<sync::WeatherSyncService> m_weatherSyncService;

    // 玩家广播门面（声音/粒子/实体事件/世界事件/方块事件/爆炸/光照更新，下沉自 broadcast*/send*）
    std::unique_ptr<net::PlayerBroadcaster> m_broadcaster;

    // 登录流程门面（玩家创建 + 初始游戏状态推送整簇，下沉自 createPlayerForConnection 等）
    std::unique_ptr<net::LoginFlow> m_loginFlow;

    // Play 包处理门面（routeInboundPlayPacket + 13 个非纯虚 handle*Packet +
    // updateEntityTrackingForPlayer 整簇，批7 下沉）
    std::unique_ptr<net::ServerPlayHandler> m_playHandler;

    // 交互管理器
    std::unique_ptr<interaction::BlockInteractionManager> m_blockInteractionManager;
    std::unique_ptr<interaction::MiningManager> m_miningManager;
    std::unique_ptr<interaction::ContainerManager> m_containerManager;
    std::unique_ptr<interaction::InventoryManager> m_inventoryManager;
    util::UniversalWorkerPool m_computationWorkerPool;
    util::UniversalWorkerPool m_ioWorkerPool;

    // 命令
    std::unique_ptr<mc::command::CommandRegistry> m_commandRegistry;
    std::unique_ptr<mc::command::CommandStorage> m_commandStorage;

    // 记分板
    std::unique_ptr<ServerScoreboard> m_scoreboard;

    // Boss 栏管理器
    std::unique_ptr<CustomServerBossInfoManager> m_bossBarManager;

    // 掉落表
    mc::loot::LootTableManager m_lootTableManager;
    mc::loot::LootPredicateManager m_predicateManager;
    PackRepository m_resourcePackList;
    mc::resource::DataPackRepository m_dataPackList;

    // 函数系统
    mc::function::FunctionManager m_functionManager;
    mc::function::TimerQueue m_functionTimerQueue;

    // 脚本系统
    std::unique_ptr<ServerScriptManager> m_scriptManager;

    // 成就事件处理器
    advancement::AdvancementEventHandler m_advancementEventHandler;

    // Tick 计数器
    u64 m_tickCounter = 0;
    u64 m_lastKeepAliveTick = 0;

    // 调试统计（原子变量，供客户端线程读取）
    ServerDebugStats m_debugStats;

    // 心跳间隔（ticks）
    static constexpr u64 KEEPALIVE_INTERVAL = 300; // 15秒 @ 20 TPS
    static constexpr u64 CLEANUP_INTERVAL = 100;
};

// ============================================================================
// 模板实现
// ============================================================================

template <typename Func>
void MinecraftServer::forEachPlayer(Func&& func)
{
    m_playerManager->forEachPlayer(std::forward<Func>(func));
}

template <typename Func>
void MinecraftServer::forEachPlayer(Func&& func) const
{
    m_playerManager->forEachPlayer(std::forward<Func>(func));
}

} // namespace mc::server
