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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/Block.hpp"

namespace mc {

class Entity;

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
    /// 箭矢命中后信号持续 tick 数（对齐 vanilla ACTIVATION_TICKS_ARROWS = 20）
    static constexpr i32 ACTIVATION_TICKS_ARROWS = 20;
    /// 其他投射物命中后信号持续 tick 数（对齐 vanilla ACTIVATION_TICKS_OTHER = 8）
    static constexpr i32 ACTIVATION_TICKS_OTHER = 8;
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

    /**
     * @brief 投射物击中标靶时调用
     *
     * 对齐 vanilla TargetBlock.onProjectileHit（TargetBlock.java:42-49）：
     * 计算命中精度对应的红石信号强度，设置方块 state 并调度信号结束 tick。
     * 箭矢持续 ACTIVATION_TICKS_ARROWS(20) tick，其他投射物持续 ACTIVATION_TICKS_OTHER(8) tick。
     * 若该方块已有调度中的 tick，则不重复调度（保留首次命中设置的持续时间和强度）。
     *
     * 链路：ProjectileEntity::onBlockHit（ProjectileEntity.cpp:293）调 block.onProjectileHit，
     * AbstractArrowEntity::onBlockHit（AbstractArrowEntity.cpp:582）亦在末尾补发此通知。
     *
     * @param world 世界
     * @param state 当前方块状态
     * @param hitResult 击中结果（含命中点世界坐标、命中面方向、方块位置）
     * @param projectile 投掷物实体
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

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
     * @brief 计算投射物命中精度对应的红石信号强度
     *
     * 对齐 vanilla TargetBlock.getRedstoneStrength（TargetBlock.java:61-77）：
     * 取命中点在命中面平面内两个坐标轴的小数偏移最大值 d3，
     * 返回 ceil(15 * clamp((0.5 - d3) / 0.5, 0, 1))，至少为 1。
     * 越靠近面中心信号越强（正中=15，边缘=1）。
     *
     * @param hitPos 命中点世界坐标
     * @param hitFace 命中的面方向
     * @return i32 输出信号强度（1-15）
     */
    [[nodiscard]] static i32 getRedstoneStrength(const Vector3& hitPos, Direction hitFace);
};

} // namespace blocks
} // namespace mc
