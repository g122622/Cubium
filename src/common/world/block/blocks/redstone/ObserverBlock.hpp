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
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 侦测器方块
 *
 * 侦测器可以检测方块变化并输出短脉冲。
 *
 * ## 特性
 * - 方块变化检测：检测前端方块的变化
 * - 2 tick脉冲输出
 * - 方向性：只能从背面输出
 * - 观察面和输出面分离
 *
 * ## 容易踩的坑
 * - 脉冲输出需要精确的tick控制
 * - 方块放置/破坏检测
 * - 方向性处理
 */
class ObserverBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ObserverBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 侦测器特有方法 ==========

    /**
     * @brief 获取侦测器朝向（输出方向）
     *
     * @param state 方块状态
     * @return Direction 输出方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 检查是否正在输出信号
     *
     * @param state 方块状态
     * @return true 如果正在输出
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 设置输出状态
     *
     * @param state 方块状态
     * @param powered 是否输出
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered);

    /// 脉冲持续时间（tick）- MC 1.16.5: 激活后持续 2 tick
    static constexpr i32 PULSE_DURATION = 2;

    /// 检测延迟（tick）- MC 1.16.5: 观察->输出需要 2 tick 延迟
    static constexpr i32 DETECT_DELAY = 2;

private:
    /**
     * @brief 通知侦测器前方的邻居方块更新
     *
     * 当侦测器状态改变时，需要通知前方（观察面背面的方块）更新。
     *
     * @param world 世界引用
     * @param pos 侦测器位置
     * @param state 侦测器状态
     */
    void _updateNeighborsInFront(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
