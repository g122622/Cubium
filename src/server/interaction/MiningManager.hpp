#pragma once

#include "common/core/Types.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <unordered_map>

namespace mc {
class BlockState;
class ItemStack;
} // namespace mc

namespace mc::server {
class ServerWorld;
struct ServerPlayerData;
} // namespace mc::server

namespace mc::server::core {
class PlayerManager;
class ConnectionManager;
} // namespace mc::server::core

namespace mc::server::interaction {
class InventoryManager;
}

namespace mc::server::interaction {

/**
 * @brief 挖掘进度管理器
 *
 * 追踪玩家的挖掘进度：
 * - 计算挖掘速度（工具、方块硬度、附魔、效果等）
 * - 广播破坏动画阶段
 * - 处理挖掘中止
 *
 * 挖掘速度计算公式（MC 1.16.5）：
 * 最终挖掘速度 = (基础工具速度 + 效率附魔加成)
 *                × 急迫效果乘数
 *                × 挖掘疲劳乘数
 *                × 水下惩罚
 *                × 空中惩罚
 *
 * 方块相对硬度 = 挖掘速度 / 方块硬度 / 工具系数
 *               工具系数 = 正确工具 ? 30 : 100
 */
class MiningManager {
public:
    /**
     * @brief 构造函数
     */
    MiningManager(core::PlayerManager& playerManager, core::ConnectionManager& connectionManager);

    /**
     * @brief 设置物品栏管理器（用于获取手持物品）
     * @param inventoryManager 物品栏管理器指针
     */
    void setInventoryManager(InventoryManager* inventoryManager);

    /**
     * @brief 开始挖掘
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param entityId 用于广播动画的实体ID（破坏者ID）
     */
    void startMining(PlayerId playerId, const BlockPos& pos, EntityId entityId = 0);

    /**
     * @brief 中止挖掘
     * @param playerId 玩家ID
     */
    void abortMining(PlayerId playerId);

    /**
     * @brief 处理方块交互数据包
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param action 交互动作
     */
    void handleBlockInteraction(PlayerId playerId, const BlockPos& pos, network::BlockInteractionAction action);

    /**
     * @brief 每 tick 更新挖掘进度
     * @param world 世界引用（用于获取方块硬度）
     */
    void tick(ServerWorld& world);

    /**
     * @brief 获取挖掘进度 (0.0 - 1.0)
     * @param playerId 玩家ID
     * @return 挖掘进度，如果未在挖掘则返回 0
     */
    [[nodiscard]] f32 getMiningProgress(PlayerId playerId) const;

    /**
     * @brief 检查是否正在挖掘
     * @param playerId 玩家ID
     */
    [[nodiscard]] bool isMining(PlayerId playerId) const;

    /**
     * @brief 获取当前挖掘位置
     * @param playerId 玩家ID
     * @return 挖掘位置，如果未在挖掘则返回空
     */
    [[nodiscard]] std::optional<BlockPos> getMiningPosition(PlayerId playerId) const;

    /**
     * @brief 设置破坏动画广播回调
     * @param callback 回调函数 (playerId, x, y, z, stage)
     */
    void setOnBreakAnimBroadcast(std::function<void(PlayerId, i32, i32, i32, i8)> callback);

    /**
     * @brief 设置挖掘完成回调
     * @param callback 回调函数 (playerId, pos)
     */
    void setOnMiningComplete(std::function<void(PlayerId, const BlockPos&)> callback);

private:
    /**
     * @brief 计算挖掘速度
     * @param world 世界引用
     * @param pos 方块位置
     * @param playerId 玩家ID
     * @return 每tick的挖掘进度增量
     */
    [[nodiscard]] f32 calculateMiningSpeed(ServerWorld& world, const BlockPos& pos, PlayerId playerId) const;

    /**
     * @brief 计算玩家的挖掘速度倍率
     *
     * 参考 MC 1.16.5 PlayerEntity.getDigSpeed()
     *
     * @param world 世界引用（用于检测眼睛是否在水中）
     * @param heldItem 手持物品
     * @param blockState 目标方块状态
     * @param playerData 玩家数据（用于获取效果和状态）
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 calculateDigSpeedMultiplier(ServerWorld& world,
        const ItemStack& heldItem,
        const BlockState& blockState,
        const ServerPlayerData& playerData) const;

    /**
     * @brief 计算急迫效果乘数
     * @param playerData 玩家数据
     * @return 乘数（默认 1.0）
     */
    [[nodiscard]] f32 calculateHasteMultiplier(const ServerPlayerData& playerData) const;

    /**
     * @brief 计算挖掘疲劳乘数
     * @param playerData 玩家数据
     * @return 乘数（默认 1.0）
     */
    [[nodiscard]] f32 calculateMiningFatigueMultiplier(const ServerPlayerData& playerData) const;

    /**
     * @brief 检查玩家是否有水下速掘附魔
     * @param playerData 玩家数据
     * @return 如果头盔有水下速掘返回 true
     */
    [[nodiscard]] bool hasAquaAffinity(const ServerPlayerData& playerData) const;

    /**
     * @brief 检查玩家眼睛是否在水中
     *
     * 参考 MC 1.16.5 Entity.updateEyesInWater()
     * 眼睛检测点向下偏移约 0.11 格以避免边界精度问题
     *
     * @param world 世界引用（用于查询流体状态）
     * @param playerData 玩家数据（用于获取位置）
     * @return 如果眼睛在水中返回 true
     */
    [[nodiscard]] bool areEyesInWater(ServerWorld& world, const ServerPlayerData& playerData) const;

    /**
     * @brief 广播破坏动画
     */
    void broadcastBreakAnim(PlayerId playerId, const BlockPos& pos, i8 stage);

private:
    /**
     * @brief 挖掘状态
     */
    struct MiningState {
        BlockPos position;
        f32 progress = 0.0f;
        u8 lastStage = 255; // 上次广播的阶段 (0-9)
        bool active = false;
        u64 startTick = 0;
        EntityId breakerId = 0; // 用于广播动画
    };

    core::PlayerManager& m_playerManager;
    core::ConnectionManager& m_connectionManager;
    InventoryManager* m_inventoryManager = nullptr;
    std::unordered_map<PlayerId, MiningState> m_miningStates;

    std::function<void(PlayerId, i32, i32, i32, i8)> m_onBreakAnimBroadcast;
    std::function<void(PlayerId, const BlockPos&)> m_onMiningComplete;
};

} // namespace mc::server::interaction