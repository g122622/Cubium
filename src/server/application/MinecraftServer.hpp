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
#include "common/entity/loot/LootTable.hpp"
#include "common/network/packet/ExplosionPacket.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ParticlePacket.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/resource/ResourcePackList.hpp"
#include "common/sound/network/SoundPackets.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/storage/GlobalStorageManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "server/advancement/AdvancementEventHandler.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/interaction/MiningManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/sync/BlockUpdateSyncManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include "server/sync/EntitySyncManager.hpp"
#include "server/sync/LightSyncManager.hpp"
#include <atomic>
#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
class BlockPos;
class Player;
enum class ContainerType : u8;
namespace server {
class EntityTracker;
class ItemPickupManager;
class WeatherManager;
} // namespace server
namespace world::spawn {
class NaturalSpawner;
class DespawnManager;
} // namespace world::spawn
namespace command {
class CommandRegistry;
}
} // namespace mc

namespace mc::server {

// 前向声明
class ServerWorld;
class ServerChunkManager;

/**
 * @brief Minecraft 服务器抽象基类
 *
 * 提供所有服务器类型的共享实现：
 * - 持有所有核心 Manager
 * - 实现 tick 主循环框架
 * - 提供对 Manager 的直接访问
 *
 * 子类只需实现：
 * - 网络层（LocalConnection 或 TcpServer）
 * - 特定的数据包处理
 */
class MinecraftServer : public IServer {
    // ServerDimensionManager 需要访问 sendPacketToPlayer
    friend class mc::ServerDimensionManager;

public:
    /**
     * @brief 构造函数
     * @param config 服务器配置
     */
    explicit MinecraftServer(const ServerCoreConfig& config);

    /**
     * @brief 析构函数
     */
    ~MinecraftServer() override;

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

    [[nodiscard]] core::PacketHandler& packetHandler() override { return *m_packetHandler; }
    [[nodiscard]] const core::PacketHandler& packetHandler() const override { return *m_packetHandler; }

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

    // ========== 世界管理器 ==========

    [[nodiscard]] ServerWorld& world() override { return *m_world; }
    [[nodiscard]] const ServerWorld& world() const override { return *m_world; }

    // ========== 维度管理器 ==========

    [[nodiscard]] ServerDimensionManager& dimensionManager() override { return *m_dimensionManager; }
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override { return *m_dimensionManager; }

    [[nodiscard]] ServerChunkManager& chunkManager() override;
    [[nodiscard]] const ServerChunkManager& chunkManager() const override;

    [[nodiscard]] WorldLightManager* lightManager() override;
    [[nodiscard]] const WorldLightManager* lightManager() const override;

    [[nodiscard]] mc::EntityManager& entityManager() override;
    [[nodiscard]] const mc::EntityManager& entityManager() const override;

    [[nodiscard]] EntityTracker& entityTracker() override;
    [[nodiscard]] const EntityTracker& entityTracker() const override;

    [[nodiscard]] PhysicsEngine* physicsEngine() override;
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override;

    [[nodiscard]] WeatherManager& weatherManager() override;
    [[nodiscard]] const WeatherManager& weatherManager() const override;

    [[nodiscard]] ItemPickupManager& itemPickupManager() override;
    [[nodiscard]] const ItemPickupManager& itemPickupManager() const override;

    // ========== 玩家实体管理 ==========

    /**
     * @brief 获取玩家实体管理器
     *
     * 用于管理玩家的实体对象（Player实例）。
     * 由子类（IntegratedServer、StandaloneServer）实现。
     */
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override = 0;
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override = 0;

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

    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override;
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override;

    // ========== 同步管理器 ==========

    [[nodiscard]] sync::EntitySyncManager& entitySyncManager() override { return *m_entitySyncManager; }
    [[nodiscard]] const sync::EntitySyncManager& entitySyncManager() const override { return *m_entitySyncManager; }

    [[nodiscard]] sync::BlockUpdateSyncManager& blockUpdateSyncManager() { return *m_blockUpdateSyncManager; }
    [[nodiscard]] const sync::BlockUpdateSyncManager& blockUpdateSyncManager() const
    {
        return *m_blockUpdateSyncManager;
    }

    [[nodiscard]] sync::ChunkSendManager& chunkSendManager() override { return *m_chunkSendManager; }
    [[nodiscard]] const sync::ChunkSendManager& chunkSendManager() const override { return *m_chunkSendManager; }

    [[nodiscard]] sync::LightSyncManager& lightSyncManager() override { return *m_lightSyncManager; }
    [[nodiscard]] const sync::LightSyncManager& lightSyncManager() const override { return *m_lightSyncManager; }

    // ========== 命令系统 ==========

    [[nodiscard]] mc::command::CommandRegistry& commandRegistry() override { return *m_commandRegistry; }
    [[nodiscard]] const mc::command::CommandRegistry& commandRegistry() const override { return *m_commandRegistry; }

    // ========== 记分板系统 ==========

    [[nodiscard]] ServerScoreboard& scoreboard() override { return *m_scoreboard; }
    [[nodiscard]] const ServerScoreboard& scoreboard() const override { return *m_scoreboard; }

    // ========== Boss 栏系统 ==========

    [[nodiscard]] CustomServerBossInfoManager& bossBarManager() override { return *m_bossBarManager; }
    [[nodiscard]] const CustomServerBossInfoManager& bossBarManager() const override { return *m_bossBarManager; }

    // ========== 配置 ==========

    [[nodiscard]] i32 viewDistance() const override { return m_config.viewDistance; }
    [[nodiscard]] i32 maxPlayers() const override { return m_config.maxPlayers; }
    [[nodiscard]] u64 seed() const override { return m_config.seed; }
    [[nodiscard]] u64 currentTick() const override;
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty difficulty) override;
    [[nodiscard]] GameMode defaultGameMode() const override { return m_config.defaultGameMode; }
    void setDefaultGameMode(GameMode mode) override;
    [[nodiscard]] i32 playerIdleTimeoutMinutes() const override { return m_playerIdleTimeoutMinutes; }
    void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) override;
    void broadcastServerMessage(std::string_view message) override;
    void requestStop() override;

    // ========== 便捷方法 ==========

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const ServerCoreConfig& config() const { return m_config; }

    /**
     * @brief 获取计算型 Worker 池
     */
    [[nodiscard]] util::ServerWorkerPool& computationWorkerPool() { return m_computationWorkerPool; }
    [[nodiscard]] const util::ServerWorkerPool& computationWorkerPool() const { return m_computationWorkerPool; }

    /**
     * @brief 获取 IO Worker 池
     */
    [[nodiscard]] util::ServerWorkerPool& ioWorkerPool() { return m_ioWorkerPool; }
    [[nodiscard]] const util::ServerWorkerPool& ioWorkerPool() const { return m_ioWorkerPool; }

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
    [[nodiscard]] mc::loot::LootTableManager& lootTableManager() { return m_lootTableManager; }
    [[nodiscard]] const mc::loot::LootTableManager& lootTableManager() const { return m_lootTableManager; }
    [[nodiscard]] ResourcePackList& resourcePackList() { return m_resourcePackList; }
    [[nodiscard]] const ResourcePackList& resourcePackList() const { return m_resourcePackList; }

protected:
    void attachWorldBindings(ServerWorld& world);
    void attachWorldCommandBindings(ServerWorld& world);
    [[nodiscard]] Result<void> initializeSharedStorage(const std::string& worldName);
    [[nodiscard]] Result<size_t> saveAllWorldData();
    void shutdownSharedStorage();

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
     * 加载所有 Vanilla 注册表：
     * - VanillaBlocks
     * - Items
     * - EnchantmentRegistry
     * - BlockItemRegistry
     * - 配方
     * - 实体类型（可选）
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
     * @brief 序列化难度同步包
     * @return 封装后的完整数据包，失败时返回空vector
     */
    [[nodiscard]] std::vector<u8> serializeDifficultyPacket();

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

    /**
     * @brief 发送天气更新
     */
    void sendWeatherUpdate();

    /**
     * @brief 发送初始天气状态给指定玩家
     */
    void sendInitialWeatherStateToPlayer(PlayerId playerId);

    /**
     * @brief 发送初始难度状态给指定玩家
     */
    void sendInitialDifficultyToPlayer(PlayerId playerId);

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
     * @brief 发送区块数据给指定玩家
     */
    void sendChunkDataToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);

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
     * @brief 轮询网络事件（子类实现）
     *
     * IntegratedServer 从 LocalEndpoint 接收数据包
     * StandaloneServer 从 TcpServer 接收数据包
     */
    virtual void pollNetwork() = 0;

    /**
     * @brief 广播数据包给所有连接的玩家（子类实现）
     */
    virtual void broadcastPacket(const u8* data, size_t size) = 0;

    /**
     * @brief 将会话ID转换为玩家ID（子类实现）
     * @param sessionId 会话ID（IntegratedServer 返回固定的客户端玩家ID）
     * @return 玩家ID，如果无效返回 0
     */
    [[nodiscard]] virtual PlayerId getPlayerIdForSession(u32 sessionId) const = 0;

    /**
     * @brief 向指定玩家发送数据包（子类实现）
     */
    virtual void sendPacketToPlayer(PlayerId playerId, const u8* data, size_t size) = 0;

    /**
     * @brief 向指定玩家同步命令树
     */
    void sendCommandTreePacket(PlayerId playerId);

    /**
     * @brief 刷新指定玩家的实体追踪范围
     */
    void updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z);

    // ========== 数据包处理方法 ==========

    /**
     * @brief 分派数据包到对应的处理方法
     * @param sessionId 会话ID（用于获取玩家ID）
     * @param data 数据包数据（包含包头）
     * @param size 数据大小
     */
    void dispatchPacket(u32 sessionId, const u8* data, size_t size);

    /**
     * @brief 处理玩家移动数据包
     */
    void handlePlayerMovePacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理传送确认数据包
     */
    void handleTeleportConfirmPacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理心跳响应数据包
     */
    void handleKeepAlivePacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理聊天消息数据包
     */
    void handleChatMessagePacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理方块交互数据包
     */
    void handleBlockInteractionPacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理方块放置数据包（子类实现特定逻辑）
     */
    virtual void handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理快捷栏选择数据包（子类实现特定逻辑）
     */
    virtual void handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size) = 0;

    /**
     * @brief 处理创造模式背包动作数据包（子类实现特定逻辑）
     */
    virtual void handleCreativeInventoryActionPacket(PlayerId playerId, const u8* data, size_t size);

    /**
     * @brief 处理容器点击数据包（子类实现特定逻辑）
     */
    virtual void handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size) = 0;

    /**
     * @brief 处理关闭容器数据包（子类实现特定逻辑）
     */
    virtual void handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size) = 0;

    /**
     * @brief 处理登录请求（子类实现特定逻辑）
     */
    virtual void handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size) = 0;

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
     */
    [[nodiscard]] virtual ItemStack getHeldItemForPlacement(PlayerId playerId) = 0;

    /**
     * @brief 返回玩家当前选中槽位
     */
    [[nodiscard]] virtual i32 getSelectedHotbarSlot(PlayerId playerId) = 0;

    /**
     * @brief 设置玩家指定槽位物品
     */
    virtual void setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack) = 0;

    /**
     * @brief 同步玩家物品栏到客户端
     */
    virtual void syncPlayerInventory(PlayerId playerId) = 0;

    /**
     * @brief 尝试打开工作台容器
     */
    [[nodiscard]] virtual bool tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos) = 0;

    /**
     * @brief 发送方块更新包给指定玩家
     */
    void sendBlockUpdatePacket(PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId);

    /**
     * @brief 设置玩家初始状态
     *
     * 设置玩家的初始位置、游戏模式等状态。
     * 子类在登录处理中调用此方法。
     */
    void setupInitialPlayerState(ServerPlayerData* player, GameMode gameMode);

    /**
     * @brief 发送初始游戏状态给玩家
     *
     * 发送传送包和天气状态给新登录的玩家。
     * 子类在登录处理中调用此方法。
     */
    void sendInitialGameState(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch);

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

    /**
     * @brief 发送声音给指定玩家
     *
     * @param playerId 玩家ID
     * @param soundEventId 声音事件ID
     * @param category 声音类别
     * @param position 声音位置
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    void sendSoundToPlayer(PlayerId playerId,
        const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume = 1.0f,
        f32 pitch = 1.0f) override;

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
    void broadcastParticleInRange(client::renderer::trident::particle::ParticleTypeId type,
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
    void broadcastEntityStatusInRange(EntityId entityId, u8 status, const Vector3& pos, f32 range = 64.0f);

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
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);

    /**
     * @brief 广播粒子给指定范围内的玩家（IServer 接口）
     *
     * @param type 粒子类型 ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param velocityX X速度
     * @param velocityY Y速度
     * @param velocityZ Z速度
     * @param offsetX X偏移范围
     * @param offsetY Y偏移范围
     * @param offsetZ Z偏移范围
     * @param count 粒子数量
     * @param range 广播范围（格），默认 256 格
     */
    void broadcastParticleInRange(u32 type,
        f64 x,
        f64 y,
        f64 z,
        f32 velocityX,
        f32 velocityY,
        f32 velocityZ,
        f32 offsetX,
        f32 offsetY,
        f32 offsetZ,
        u32 count,
        f32 range = 256.0f) override;

    // ========== 爆炸广播方法 ==========

    /**
     * @brief 广播爆炸事件给范围内玩家
     *
     * 参考 MC 1.16.5: 发送给爆炸点 64 格范围内的玩家
     *
     * @param position 爆炸位置
     * @param strength 爆炸威力（半径）
     * @param affectedBlocks 受影响的方块列表
     * @param playerKnockback 玩家击退映射（玩家ID -> 击退向量）
     * @param range 广播范围（格），默认 64.0f（与 MC 1.16.5 一致）
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
    ServerCoreConfig m_config;
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
    std::unique_ptr<core::PacketHandler> m_packetHandler;
    std::unique_ptr<core::GameModeManager> m_gameModeManager;
    std::unique_ptr<core::WhitelistManager> m_whitelistManager;
    std::unique_ptr<core::BannedPlayerList> m_bannedPlayerList;
    std::unique_ptr<core::BannedIpList> m_bannedIpList;
    std::unique_ptr<core::OpListManager> m_opListManager;

    // 主世界快捷引用；真实所有权由 ServerDimension 持有
    ServerWorld* m_world = nullptr;

    // 世界级共享存储
    world::storage::GlobalStorageManager m_globalStorage;
    std::unique_ptr<world::storage::SingleLevelStorageManager> m_storage;

    // 维度管理器
    std::unique_ptr<ServerDimensionManager> m_dimensionManager;

    // 交互管理器
    std::unique_ptr<interaction::BlockInteractionManager> m_blockInteractionManager;
    std::unique_ptr<interaction::MiningManager> m_miningManager;
    std::unique_ptr<interaction::ContainerManager> m_containerManager;
    std::unique_ptr<interaction::InventoryManager> m_inventoryManager;
    util::ServerWorkerPool m_computationWorkerPool;
    util::ServerWorkerPool m_ioWorkerPool;

    // 同步管理器
    std::unique_ptr<sync::EntitySyncManager> m_entitySyncManager;
    std::unique_ptr<sync::BlockUpdateSyncManager> m_blockUpdateSyncManager;
    std::unique_ptr<sync::ChunkSendManager> m_chunkSendManager;
    std::unique_ptr<sync::LightSyncManager> m_lightSyncManager;

    // 命令
    std::unique_ptr<mc::command::CommandRegistry> m_commandRegistry;

    // 记分板
    std::unique_ptr<ServerScoreboard> m_scoreboard;

    // Boss 栏管理器
    std::unique_ptr<CustomServerBossInfoManager> m_bossBarManager;

    // 掉落表
    mc::loot::LootTableManager m_lootTableManager;
    ResourcePackList m_resourcePackList;

    // 刷怪系统
    std::unique_ptr<::mc::world::spawn::NaturalSpawner> m_naturalSpawner;
    std::unique_ptr<::mc::world::spawn::DespawnManager> m_despawnManager;

    // 成就事件处理器
    advancement::AdvancementEventHandler m_advancementEventHandler;

    // Tick 计数器
    u64 m_tickCounter = 0;
    u64 m_lastKeepAliveTick = 0;

    // 心跳间隔（ticks）
    static constexpr u64 KEEPALIVE_INTERVAL = 300; // 15秒 @ 20 TPS
    static constexpr u64 CLEANUP_INTERVAL = 100;

    // 上次发送的天气强度（用于检测变化）
    f32 m_lastSentRainStrength = 0.0f;
    f32 m_lastSentThunderStrength = 0.0f;
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
