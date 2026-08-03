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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 标靶方块
 *
 * 标靶是一种可以被箭矢命中的方块，根据命中精度输出不同强度的红石信号。
 *
 * ## 特性
 * - 箭矢命中触发
 * - 输出信号强度取决于命中精度
 * - 输出持续时间约1秒（20 ticks）
 * - 红石信号输出（0-15）
 *
 * 参考: net.minecraft.block.TargetBlock
 */
class TargetBlock : public Block {
public:
    /// 输出信号持续时间（ticks）
    static constexpr i32 SIGNAL_DURATION = 20;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TargetBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Normal;
    }

    // ========== 标靶特有方法 ==========

    /**
     * @brief 获取当前输出信号强度
     * @param state 方块状态
     * @return i32 信号强度（0-15）
     */
    [[nodiscard]] static i32 getPower(const BlockState& state);

    /**
     * @brief 设置输出信号强度
     * @param state 方块状态
     * @param power 信号强度（0-15）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPower(BlockState state, i32 power);

    /**
     * @brief 计算箭矢命中精度
     *
     * 根据命中点到方块中心的距离计算输出信号强度。
     * 距离越近，信号越强。
     *
     * @param hitX 命中点X坐标（相对方块）
     * @param hitY 命中点Y坐标（相对方块）
     * @param hitZ 命中点Z坐标（相对方块）
     * @return i32 输出信号强度（0-15）
     */
    [[nodiscard]] static i32 calculatePower(f32 hitX, f32 hitY, f32 hitZ);

    /**
     * @brief 被箭矢命中时触发
     *
     * @param world 世界引用
     * @param pos 标靶位置
     * @param state 当前方块状态
     * @param hitX 命中点X坐标
     * @param hitY 命中点Y坐标
     * @param hitZ 命中点Z坐标
     */
    void onHitByArrow(IWorld& world, const BlockPos& pos, const BlockState& state, f32 hitX, f32 hitY, f32 hitZ);
};

} // namespace blocks
} // namespace mc
