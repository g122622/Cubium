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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc {
class AbstractContainerMenu;
class PlayerInventory;
class ServerDimensionManager;
namespace function {
class FunctionManager;
class TimerQueue;
} // namespace function
namespace loot {
class LootTableManager;
class LootPredicateManager;
} // namespace loot
namespace resource {
class DataPackRepository;
}
namespace server {
class ServerPlayerEntityManager;
class ServerWorld;
} // namespace server
namespace time {
class GameTime;
}
namespace world {
namespace tick {
class TickManager;
}
namespace storage {
class SingleLevelStorageManager;
}
} // namespace world
namespace command {
class CommandRegistry;
class CommandStorage;
} // namespace command
} // namespace mc

namespace mc::server {

// 前向声明
class ServerScoreboard;
class CustomServerBossInfoManager;

namespace core {
class PlayerManager;
class ConnectionManager;
class TimeManager;
class TeleportManager;
class KeepAliveManager;
class PositionTracker;
class GameModeManager;
class WhitelistManager;
class BannedPlayerList;
class BannedIpList;
class OpListManager;
} // namespace core

namespace interaction {
class BlockInteractionManager;
class MiningManager;
class ContainerManager;
class InventoryManager;
} // namespace interaction

/**
 * @brief 服务器接口
 *
 * 定义了所有服务器类型的统一接口。
 * 提供对各个 Manager 的直接访问，避免二次封装。
 */
class IServer {
public:
    virtual ~IServer() = default;

    // ========== 生命周期 ==========

    /**
     * @brief 初始化服务器
     */
    [[nodiscard]] virtual Result<void> initialize() = 0;

    /**
     * @brief 关闭服务器
     */
    virtual void shutdown() = 0;

    /**
     * @brief 执行一个 tick
     */
    virtual void tick() = 0;

    /**
     * @brief 检查服务器是否正在运行
     */
    [[nodiscard]] virtual bool isRunning() const = 0;

    // ========== 核心管理器 ==========

    [[nodiscard]] virtual core::PlayerManager& playerManager() = 0;
    [[nodiscard]] virtual const core::PlayerManager& playerManager() const = 0;

    [[nodiscard]] virtual core::ConnectionManager& connectionManager() = 0;
    [[nodiscard]] virtual const core::ConnectionManager& connectionManager() const = 0;

    [[nodiscard]] virtual core::TimeManager& timeManager() = 0;
    [[nodiscard]] virtual const core::TimeManager& timeManager() const = 0;

    [[nodiscard]] virtual core::TeleportManager& teleportManager() = 0;
    [[nodiscard]] virtual const core::TeleportManager& teleportManager() const = 0;

    [[nodiscard]] virtual core::KeepAliveManager& keepAliveManager() = 0;
    [[nodiscard]] virtual const core::KeepAliveManager& keepAliveManager() const = 0;

    [[nodiscard]] virtual core::PositionTracker& positionTracker() = 0;
    [[nodiscard]] virtual const core::PositionTracker& positionTracker() const = 0;

    [[nodiscard]] virtual core::GameModeManager& gameModeManager() = 0;
    [[nodiscard]] virtual const core::GameModeManager& gameModeManager() const = 0;

    // ========== 白名单管理器 ==========

    /**
     * @brief 获取白名单管理器
     * @return 白名单管理器引用
     */
    [[nodiscard]] virtual core::WhitelistManager& whitelistManager() = 0;
    [[nodiscard]] virtual const core::WhitelistManager& whitelistManager() const = 0;

    // ========== 封禁管理器 ==========

    /**
     * @brief 获取玩家封禁列表管理器
     * @return 玩家封禁列表管理器引用
     */
    [[nodiscard]] virtual core::BannedPlayerList& bannedPlayerList() = 0;
    [[nodiscard]] virtual const core::BannedPlayerList& bannedPlayerList() const = 0;

    /**
     * @brief 获取 IP 封禁列表管理器
     * @return IP 封禁列表管理器引用
     */
    [[nodiscard]] virtual core::BannedIpList& bannedIpList() = 0;
    [[nodiscard]] virtual const core::BannedIpList& bannedIpList() const = 0;

    // ========== OP 管理器 ==========

    /**
     * @brief 获取 OP 列表管理器
     * @return OP 列表管理器引用
     */
    [[nodiscard]] virtual core::OpListManager& opListManager() = 0;
    [[nodiscard]] virtual const core::OpListManager& opListManager() const = 0;

    // ========== 维度管理器 ==========

    /**
     * @brief 获取维度管理器
     * @return 维度管理器引用
     */
    [[nodiscard]] virtual ServerDimensionManager& dimensionManager() = 0;
    [[nodiscard]] virtual const ServerDimensionManager& dimensionManager() const = 0;

    // ========== 世界访问（维度感知）==========

    /**
     * @brief 获取指定玩家所在维度的世界
     * @param playerId 玩家ID
     * @return 玩家所在维度的 ServerWorld 指针，玩家不在任何维度时返回 nullptr
     */
    [[nodiscard]] virtual ServerWorld* getPlayerWorld(PlayerId playerId) = 0;

    // ========== 玩家实体管理 ==========

    /**
     * @brief 获取玩家实体管理器
     *
     * 用于管理玩家的实体对象（Player实例）。
     */
    [[nodiscard]] virtual ServerPlayerEntityManager& playerEntityManager() = 0;
    [[nodiscard]] virtual const ServerPlayerEntityManager& playerEntityManager() const = 0;

    // ========== 交互管理器 ==========

    [[nodiscard]] virtual interaction::BlockInteractionManager& blockInteractionManager() = 0;
    [[nodiscard]] virtual const interaction::BlockInteractionManager& blockInteractionManager() const = 0;

    [[nodiscard]] virtual interaction::MiningManager& miningManager() = 0;
    [[nodiscard]] virtual const interaction::MiningManager& miningManager() const = 0;

    [[nodiscard]] virtual interaction::ContainerManager& containerManager() = 0;
    [[nodiscard]] virtual const interaction::ContainerManager& containerManager() const = 0;

    [[nodiscard]] virtual interaction::InventoryManager& inventoryManager() = 0;
    [[nodiscard]] virtual const interaction::InventoryManager& inventoryManager() const = 0;

    /**
     * @brief 获取指定玩家的物品栏。
     */
    [[nodiscard]] virtual PlayerInventory* playerInventory(PlayerId playerId) = 0;
    [[nodiscard]] virtual const PlayerInventory* playerInventory(PlayerId playerId) const = 0;

    // ========== 命令系统 ==========

    [[nodiscard]] virtual mc::command::CommandRegistry& commandRegistry() = 0;
    [[nodiscard]] virtual const mc::command::CommandRegistry& commandRegistry() const = 0;

    /**
     * @brief 获取命令存储
     *
     * /data storage 命令使用的持久化 NBT 存储。
     */
    [[nodiscard]] virtual mc::command::CommandStorage& commandStorage() = 0;
    [[nodiscard]] virtual const mc::command::CommandStorage& commandStorage() const = 0;

    // ========== 数据包系统 ==========

    /**
     * @brief 获取数据包列表
     * @return 数据包列表引用
     */
    [[nodiscard]] virtual resource::DataPackRepository& dataPackList() = 0;
    [[nodiscard]] virtual const resource::DataPackRepository& dataPackList() const = 0;

    /**
     * @brief 获取掉落表管理器
     * @return 掉落表管理器引用
     */
    [[nodiscard]] virtual loot::LootTableManager& lootTableManager() = 0;
    [[nodiscard]] virtual const loot::LootTableManager& lootTableManager() const = 0;

    /**
     * @brief 获取战利品谓词管理器
     * @return 战利品谓词管理器引用
     */
    [[nodiscard]] virtual loot::LootPredicateManager& predicateManager() = 0;
    [[nodiscard]] virtual const loot::LootPredicateManager& predicateManager() const = 0;

    /**
     * @brief 获取函数管理器
     *
     * 管理数据包函数（.mcfunction）的加载、注册、执行和调度。
     */
    [[nodiscard]] virtual function::FunctionManager& functionManager() = 0;
    [[nodiscard]] virtual const function::FunctionManager& functionManager() const = 0;

    /**
     * @brief 获取函数调度定时器队列
     *
     * 用于 /schedule 命令调度函数延迟执行。
     */
    [[nodiscard]] virtual function::TimerQueue& functionTimerQueue() = 0;
    [[nodiscard]] virtual const function::TimerQueue& functionTimerQueue() const = 0;

    /**
     * @brief 获取共享世界存储。
     *
     * 返回跨维度共享的单关卡存储入口，不再要求调用方通过主世界绕行。
     */
    [[nodiscard]] virtual world::storage::SingleLevelStorageManager* sharedStorage() = 0;
    [[nodiscard]] virtual const world::storage::SingleLevelStorageManager* sharedStorage() const = 0;
    [[nodiscard]] virtual bool isSharedStorageReadonlyForeignWorld() const = 0;

    // ========== 记分板系统 ==========

    /**
     * @brief 获取服务端记分板
     * @return 服务端记分板引用
     */
    [[nodiscard]] virtual ServerScoreboard& scoreboard() = 0;
    [[nodiscard]] virtual const ServerScoreboard& scoreboard() const = 0;

    // ========== Boss 栏系统 ==========

    /**
     * @brief 获取自定义 Boss 栏管理器
     * @return Boss 栏管理器引用
     */
    [[nodiscard]] virtual CustomServerBossInfoManager& bossBarManager() = 0;
    [[nodiscard]] virtual const CustomServerBossInfoManager& bossBarManager() const = 0;

    // ========== 配置 ==========

    /**
     * @brief 检查是否为集成服务器（单机模式）
     * @return true 如果是 IntegratedServer
     */
    [[nodiscard]] virtual bool isIntegrated() const noexcept = 0;

    /**
     * @brief 检查是否为独立服务器（多人模式）
     * @return true 如果是 StandaloneServer
     */
    [[nodiscard]] virtual bool isDedicated() const noexcept = 0;

    [[nodiscard]] virtual i32 viewDistance() const = 0;
    [[nodiscard]] virtual i32 simulationDistance() const = 0;
    [[nodiscard]] virtual i32 maxPlayers() const = 0;
    [[nodiscard]] virtual u64 seed() const = 0;
    [[nodiscard]] virtual u64 currentTick() const = 0;

    /**
     * @brief 获取当前世界难度。
     */
    [[nodiscard]] virtual Difficulty difficulty() const = 0;

    /**
     * @brief 设置当前世界难度。
     *
     * 难度被锁定（isDifficultyLocked()=true）时本调用直接返回，不修改也不广播，
     * 对齐 Java MinecraftServer.setDifficulty(diff, false) 的 !isDifficultyLocked() 守卫。
     */
    virtual void setDifficulty(Difficulty difficulty) = 0;

    /**
     * @brief 难度是否被锁定（锁定后不可再切换难度，对齐 Java LevelData.isDifficultyLocked）。
     */
    [[nodiscard]] virtual bool isDifficultyLocked() const noexcept = 0;

    /**
     * @brief 设置难度锁定状态，并立即向所有在线玩家广播 cb:10 ChangeDifficulty（携带新 locked 值）。
     *
     * 对齐 Java MinecraftServer.setDifficultyLocked：仅置位并广播，不改变当前难度。
     */
    virtual void setDifficultyLocked(bool locked) = 0;

    /**
     * @brief 该玩家是否为单人/LAN 主机（集成服本地客户端玩家）。
     *
     * 对齐 Java IntegratedServer.isSingleplayerOwner / DedicatedServer.isSingleplayerOwner（恒 false）。
     * 用于难度切换/锁定等仅主机可执行操作的权限校验。
     */
    [[nodiscard]] virtual bool isSingleplayerOwner(PlayerId playerId) const noexcept = 0;

    /**
     * @brief 获取出生点保护半径（spawn-protection）。
     *
     * 对齐 Java MinecraftServer.getSpawnProtectionRadius，半径内（切比雪夫 XZ）非 op
     * 玩家禁止破坏/放置/交互方块。返回 0 表示关闭保护（原版语义）。
     */
    [[nodiscard]] virtual i32 spawnProtectionRadius() const = 0;

    /**
     * @brief 获取服务器默认游戏模式。
     */
    [[nodiscard]] virtual GameMode defaultGameMode() const = 0;

    /**
     * @brief 设置服务器默认游戏模式。
     */
    virtual void setDefaultGameMode(GameMode mode) = 0;

    /**
     * @brief 获取玩家挂机超时，单位为分钟。
     */
    [[nodiscard]] virtual i32 playerIdleTimeoutMinutes() const = 0;

    /**
     * @brief 设置玩家挂机超时，单位为分钟。
     */
    virtual void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) = 0;

    /**
     * @brief 请求服务器优雅停机。
     *
     * 只触发停机流程，不在调用线程里直接释放资源。
     */
    virtual void requestStop() = 0;

    // ========== 局域网发布 ==========

    /**
     * @brief 发布到局域网（仅集成服务器有效）
     *
     * 对应原版 MinecraftIntegratedServer 的 publishServer() 方法。
     * 集成服务器调用后，会启动一个 TCP 监听器，允许局域网内其他玩家通过
     * TCP 连接加入本局游戏；同时可切换作弊开关（运行时生效，不修改 level.dat）。
     *
     * @param port 监听端口（1-65535）
     * @param allowCheats 是否允许作弊
     * @return 成功返回 ok；已发布/端口占用/非集成服务器等情况返回错误
     */
    [[nodiscard]] virtual Result<void> publishToLan(i32 port, bool allowCheats) = 0;
};

} // namespace mc::server
