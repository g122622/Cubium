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

#include "common/core/Types.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <optional>
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
 * 挖掘速度计算公式：
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
    void startMining(PlayerId playerId, const BlockPos& pos, EntityInstanceId entityId = 0);

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
     * @brief 处理 StopDestroyBlock。
     *
     * 对齐原版 ServerPlayerGameMode.handleBlockBreakAction 的 STOP_DESTROY_BLOCK 分支
     *（:209-231）：计算绝对进度 f1 = 单tick进度 × (gameTicks - startTick + 1)。
     * - f1 >= 0.7F：清除破坏阶段动画，erase 状态并返回 true（由调用方触发 handleBlockBreak）。
     * - f1 < 0.7F：转入 delayed-destroy 状态机（hasDelayedDestroy=true），由 tick 续挖，
     *   返回 false。
     * - 状态不存在/位置不匹配：返回 false（容错，消除乱序/重复 STOP 的崩溃风险）。
     *
     * @param world 世界引用（用于查询方块状态计算挖掘速度）
     * @param pos 目标方块位置
     * @return true 表示进度已达 0.7 阈值并已在内部触发 onMiningComplete 完成破坏；
     *         false 表示转入延迟续挖、状态不存在或位置不匹配（正常语义，调用方无需额外处理）
     */
    bool tryCompleteMining(PlayerId playerId, const BlockPos& pos, ServerWorld& world);

    /**
     * @brief 设置破坏动画广播回调
     * @param callback 回调函数 (playerId, x, y, z, stage)
     */
    void setOnBreakAnimBroadcast(std::function<void(PlayerId, i32, i32, i32, i8)> callback);

    /**
     * @brief 设置 EntityInstanceId 解析器
     *
     * MiningManager 内部只有 PlayerId，但广播破坏动画需要 EntityInstanceId 作为 breakerId。
     * 通过此解析器将 PlayerId 转换为 EntityInstanceId。
     *
     * @param resolver 解析函数 (playerId) -> entityId，未找到返回 INVALID_ENTITY_ID
     */
    void setEntityIdResolver(std::function<EntityInstanceId(PlayerId)> resolver);

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
    [[nodiscard]] f32 _calculateMiningSpeed(ServerWorld& world, const BlockPos& pos, PlayerId playerId) const;

    /**
     * @brief 计算玩家的挖掘速度倍率
     * @param world 世界引用（用于检测眼睛是否在水中）
     * @param heldItem 手持物品
     * @param blockState 目标方块状态
     * @param playerData 玩家数据（用于获取效果和状态）
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 _calculateDigSpeedMultiplier(ServerWorld& world,
        const ItemStack& heldItem,
        const BlockState& blockState,
        const ServerPlayerData& playerData) const;

    /**
     * @brief 计算急迫效果乘数
     * @param playerData 玩家数据
     * @return 乘数（默认 1.0）
     */
    [[nodiscard]] f32 _calculateHasteMultiplier(const ServerPlayerData& playerData) const;

    /**
     * @brief 计算挖掘疲劳乘数
     * @param playerData 玩家数据
     * @return 乘数（默认 1.0）
     */
    [[nodiscard]] f32 _calculateMiningFatigueMultiplier(const ServerPlayerData& playerData) const;

    /**
     * @brief 检查玩家是否有水下速掘附魔
     * @param playerData 玩家数据
     * @return 如果头盔有水下速掘返回 true
     */
    [[nodiscard]] bool _hasAquaAffinity(const ServerPlayerData& playerData) const;

    /**
     * @brief 检查玩家眼睛是否在水中
     *
     * 眼睛检测点向下偏移约 0.11 格以避免边界精度问题
     *
     * @param world 世界引用（用于查询流体状态）
     * @param playerData 玩家数据（用于获取位置）
     * @return 如果眼睛在水中返回 true
     */
    [[nodiscard]] bool _areEyesInWater(ServerWorld& world, const ServerPlayerData& playerData) const;

    /**
     * @brief 广播破坏动画
     */
    void _broadcastBreakAnim(PlayerId playerId, const BlockPos& pos, i8 stage);

private:
    /**
     * @brief 挖掘状态
     *
     * 对齐原版 ServerPlayerGameMode 字段（:42-49）：
     * - progress：当前绝对进度 f = 单tick进度 × (已过tick数+1)
     * - lastStage：上次广播的阶段 lastSentState（255 表示未开始，对齐原版 -1）
     * - active：是否正在挖掘 isDestroyingBlock
     * - startTick：开始挖掘的 gameTick destroyProgressStart
     * - hasDelayedDestroy/delayedDestroyPos/delayedTickStart：延迟破坏状态机
     */
    struct MiningState {
        BlockPos position;
        f32 progress = 0.0f;            // 绝对进度：单tick进度 × (已过tick数+1)
        u8 lastStage = 255;             // 上次广播的阶段 (0-9)，255=未开始
        bool active = false;            // isDestroyingBlock
        u64 startTick = 0;              // destroyProgressStart
        EntityInstanceId breakerId = 0; // 用于广播动画
        // delayed-destroy（对齐原版 :46-48）
        bool hasDelayedDestroy = false;
        BlockPos delayedDestroyPos;
        u64 delayedTickStart = 0;
    };

    core::PlayerManager& m_playerManager;
    core::ConnectionManager& m_connectionManager;
    InventoryManager* m_inventoryManager = nullptr;
    std::unordered_map<PlayerId, MiningState> m_miningStates;

    /// 全局 gameTick 计数器（对齐原版 ServerPlayerGameMode.gameTicks，每 tick 递增）
    u64 m_gameTicks = 0;

    std::function<void(PlayerId, i32, i32, i32, i8)> m_onBreakAnimBroadcast;
    std::function<void(PlayerId, const BlockPos&)> m_onMiningComplete;
    std::function<EntityInstanceId(PlayerId)> m_entityIdResolver; ///< PlayerId -> EntityInstanceId 解析器
};

} // namespace mc::server::interaction
