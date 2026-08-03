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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

// 前向声明
namespace entity {
class FallingBlockEntity;
}

namespace blocks {

/**
 * @brief 可下落方块基类
 *
 * 用于沙子、红沙、砾石等会受重力影响的方块。
 * 当下方方块无法支撑时，调度计划刻并生成下落方块实体。
 */
class FallingBlock : public Block {
public:
    explicit FallingBlock(const BlockProperties& properties);
    ~FallingBlock() override = default;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 方块更新后处理
     *
     * 当邻居方块更新时也调度 tick。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 虚方法（子类可覆盖） ==========

    /**
     * @brief 获取下落延迟
     *
     * 默认返回 2 tick。铁砧等子类可覆盖。
     *
     * @return 延迟 tick 数
     */
    [[nodiscard]] virtual i32 getFallDelay() const { return FALL_DELAY_TICKS; }

    /**
     * @brief 开始下落时的回调
     *
     * 子类可覆盖以执行特殊行为。
     *
     * @param world 世界
     * @param pos 位置
     * @param entity 下落方块实体
     */
    virtual void onStartFalling(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity)
    {
        (void)world;
        (void)pos;
        (void)entity;
    }

    /**
     * @brief 落地时的回调
     *
     * 当下落方块落到地面时调用。
     * 子类可覆盖（如混凝土粉末遇水固化）。
     *
     * @param world 世界
     * @param pos 落地位置
     * @param fallingState 下落时的方块状态
     * @param hitState 落地点的方块状态
     * @param entity 下落方块实体
     */
    virtual void onEndFalling(IWorld& world,
        const BlockPos& pos,
        const BlockState& fallingState,
        const BlockState& hitState,
        entity::FallingBlockEntity& entity)
    {
        (void)world;
        (void)pos;
        (void)fallingState;
        (void)hitState;
        (void)entity;
    }

    /**
     * @brief 破碎时的回调
     *
     * 当下落方块无法放置时调用（如落到不合适的方块上）。
     * 子类可覆盖（如铁砧损坏）。
     *
     * @param world 世界
     * @param pos 位置
     * @param entity 下落方块实体
     */
    virtual void onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity)
    {
        (void)world;
        (void)pos;
        (void)entity;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查方块状态是否可穿透
     *
     * 用于判断下落方块是否可以穿过指定方块。
     * 可穿透的方块包括：空气、液体、火焰、可替换材质。
     * 对齐 MC 1.21.11 FallingBlock.isFree()。
     *
     * @param state 方块状态
     * @return 如果可穿透返回 true
     */
    [[nodiscard]] static bool canFallThrough(const BlockState* state);

protected:
    static constexpr i32 FALL_DELAY_TICKS = 2;
};

} // namespace blocks
} // namespace mc
