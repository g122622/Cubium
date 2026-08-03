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

#include "../../core/Types.hpp"
#include "../block/BlockPos.hpp"
#include "../tick/base/TickPriority.hpp"
#include "RedstoneContext.hpp"
#include "RedstonePower.hpp"
#include "common/util/Direction.hpp"
#include <cstddef>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class Block;

namespace world {
namespace redstone {

/**
 * @brief 红石火把烧毁记录
 *
 * 记录单个红石火把的翻转历史和烧毁状态。
 */
struct TorchBurnoutRecord {
    /// 翻转时间戳队列（存储最近的翻转时间）
    std::deque<u64> flipTimes;

    /// 是否已烧毁
    bool isBurnedOut = false;

    /// 烧毁时间（用于冷却检测）
    u64 burnoutTime = 0;
};

/**
 * @brief 红石系统管理器
 *
 * 协调红石信号的计算、更新和传播。
 * 使用单例模式，通过 IWorld 接口与世界交互。
 *
 * ## 核心职责
 * 1. 红石信号传播和更新调度
 * 2. 防止无限递归更新
 * 3. 更新批处理和优化
 * 4. 红石火把烧毁跟踪
 *
 * ## 使用示例
 * ```cpp
 * auto& redstone = RedstoneSystem::instance();
 *
 * // 更新相邻方块的红石状态
 * redstone.updateNeighbors(world, pos, block);
 *
 * // 调度延迟更新
 * redstone.scheduleUpdate(world, pos, block, 2, TickPriority::High);
 *
 * // 检查位置是否正在更新
 * if (!redstone.isUpdating(pos)) {
 *     redstone.beginUpdate(pos);
 *     // 执行红石计算...
 *     redstone.endUpdate(pos);
 * }
 *
 * // 红石火把翻转记录
 * if (redstone.checkAndRecordTorchFlip(pos, currentTick)) {
 *     // 火把烧毁
 * }
 * ```
 *
 * ## 线程安全
 * 内部使用 RedstoneContext 保护共享状态。
 */
class RedstoneSystem {
public:
    /**
     * @brief 获取单例实例
     */
    static RedstoneSystem& instance();

    // ========== 红石更新 ==========

    /**
     * @brief 更新相邻方块的红石状态
     *
     * 当红石信号变化时，通知相邻方块更新状态。
     * 这会触发 neighborChanged 回调。
     *
     * @param world 世界引用
     * @param pos 信号源位置
     * @param block 信号源方块（用于更新回调）
     */
    void updateNeighbors(IWorld& world, const BlockPos& pos, Block& block);

    /**
     * @brief 更新指定方向以外的相邻方块
     *
     * 用于红石线等需要跳过特定方向的更新。
     *
     * @param world 世界引用
     * @param pos 信号源位置
     * @param block 信号源方块
     * @param skipDirection 跳过的方向
     */
    void updateNeighborsExcept(IWorld& world, const BlockPos& pos, Block& block, Direction skipDirection);

    /**
     * @brief 更新水平和下方的方块
     *
     * 用于红石火把等不向上传播信号的情况。
     *
     * @param world 世界引用
     * @param pos 信号源位置
     * @param block 信号源方块
     */
    void updateNeighborsHorizontalAndDown(IWorld& world, const BlockPos& pos, Block& block);

    /**
     * @brief 更新指定方块周围的比较器
     *
     * 当容器内容变化时调用。
     *
     * @param world 世界引用
     * @param pos 容器位置
     */
    void updateComparators(IWorld& world, const BlockPos& pos);

    // ========== 更新调度 ==========

    /**
     * @brief 调度红石更新
     *
     * 安排延迟tick执行的红石更新。
     * 红石更新默认使用高优先级。
     *
     * @param world 世界引用
     * @param pos 更新位置
     * @param block 方块引用
     * @param delay 延迟tick数
     * @param priority tick优先级（默认High）
     */
    void scheduleUpdate(IWorld& world,
        const BlockPos& pos,
        Block& block,
        i32 delay,
        tick::TickPriority priority = tick::TickPriority::High);

    /**
     * @brief 调度极优先更新
     *
     * 用于红石二极管（中继器、比较器）面朝另一个二极管时。
     * 确保更新顺序正确。
     *
     * @param world 世界引用
     * @param pos 更新位置
     * @param block 方块引用
     * @param delay 延迟tick数
     */
    void scheduleExtremelyHighPriorityUpdate(IWorld& world, const BlockPos& pos, Block& block, i32 delay);

    // ========== 递归保护 ==========

    /**
     * @brief 检查位置是否正在被更新
     *
     * @param pos 检查位置
     * @return true 如果位置正在更新中
     */
    [[nodiscard]] bool isUpdating(const BlockPos& pos) const { return m_context.isUpdating(pos); }

    /**
     * @brief 开始更新某个位置
     *
     * @param pos 更新位置
     */
    void beginUpdate(const BlockPos& pos) { m_context.beginUpdate(pos); }

    /**
     * @brief 结束更新某个位置
     *
     * @param pos 更新位置
     */
    void endUpdate(const BlockPos& pos) { m_context.endUpdate(pos); }

    /**
     * @brief 检查是否可以增加更新深度
     */
    [[nodiscard]] bool canPushDepth() const { return m_context.canPushDepth(); }

    /**
     * @brief 增加更新深度
     */
    void pushDepth() { m_context.pushDepth(); }

    /**
     * @brief 减少更新深度
     */
    void popDepth() { m_context.popDepth(); }

    // ========== 信号查询 ==========

    /**
     * @brief 获取位置的间接红石信号强度
     *
     * 等同于调用 RedstonePower::getRedstonePowerFromNeighbors
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 最大信号强度 0-15
     */
    [[nodiscard]] i32 getIndirectPower(IWorld& world, const BlockPos& pos) const
    {
        return RedstonePower::getRedstonePowerFromNeighbors(world, pos);
    }

    /**
     * @brief 检查方块是否被红石充能
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return true 如果被充能
     */
    [[nodiscard]] bool isBlockPowered(IWorld& world, const BlockPos& pos) const
    {
        return RedstonePower::isPowered(world, pos);
    }

    // ========== 红石火把烧毁跟踪 ==========

    /**
     * @brief 烧毁检测窗口（tick）
     *
     * 在此时间窗口内翻转达到 BURNOUT_FLIPS 次会触发烧毁。
     */
    static constexpr i32 BURNOUT_WINDOW = 60;

    /**
     * @brief 触发烧毁的翻转次数
     */
    static constexpr i32 BURNOUT_FLIPS = 8;

    /**
     * @brief 烧毁后冷却时间（tick）
     */
    static constexpr i32 BURNOUT_COOLDOWN = 160;

    /**
     * @brief 记录红石火把翻转并检查是否应烧毁
     *
     * 此方法实现 MC 1.16.5 的红石火把烧毁机制：
     * - 在 BURNOUT_WINDOW (60) tick 内翻转 BURNOUT_FLIPS (8) 次会烧毁
     * - 烧毁后需要等待 BURNOUT_COOLDOWN (160) tick 冷却
     *
     * @param pos 火把位置
     * @param currentTick 当前游戏 tick
     * @return true 如果火把应该烧毁
     */
    bool checkAndRecordTorchFlip(const BlockPos& pos, u64 currentTick);

    /**
     * @brief 检查火把是否处于烧毁状态
     *
     * @param pos 火把位置
     * @param currentTick 当前游戏 tick
     * @return true 如果火把已烧毁且仍在冷却中
     */
    [[nodiscard]] bool isTorchBurnedOut(const BlockPos& pos, u64 currentTick) const;

    /**
     * @brief 清除火把的烧毁记录
     *
     * 当火把被移除时调用。
     *
     * @param pos 火把位置
     */
    void clearTorchRecord(const BlockPos& pos);

    /**
     * @brief 清理过期的烧毁记录
     *
     * 定期调用以避免内存泄漏。
     *
     * @param currentTick 当前游戏 tick
     */
    void cleanupBurnoutRecords(u64 currentTick);

    // ========== 重置 ==========

    /**
     * @brief 清空所有状态
     *
     * 在世界卸载时调用。
     */
    void clear()
    {
        m_context.clear();
        m_torchRecords.clear();
    }

private:
    RedstoneSystem() = default;

    /// 红石上下文（防止递归）
    RedstoneContext m_context;

    /// 红石火把烧毁记录（位置 -> 记录）
    std::unordered_map<BlockPos, TorchBurnoutRecord> m_torchRecords;

    /**
     * @brief 内部方法：通知单个邻居更新
     *
     * @param world 世界引用
     * @param neighborPos 邻居位置
     * @param neighborState 邻居方块状态
     * @param sourceBlock 触发更新的源方块
     * @param sourcePos 源方块位置
     */
    void _notifyNeighbor(IWorld& world,
        const BlockPos& neighborPos,
        const BlockState& neighborState,
        Block& sourceBlock,
        const BlockPos& sourcePos);

    /**
     * @brief 内部方法：更新指定方向列表的邻居
     *
     * @param world 世界引用
     * @param pos 源位置
     * @param block 源方块
     * @param directions 方向列表
     * @param directionCount 方向数量
     */
    void _updateNeighborsInDirections(
        IWorld& world, const BlockPos& pos, Block& block, const Direction* directions, size_t directionCount);
};

} // namespace redstone
} // namespace world
} // namespace mc
