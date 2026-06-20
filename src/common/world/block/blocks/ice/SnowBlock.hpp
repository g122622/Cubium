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

#include "../../../../util/property/Properties.hpp"
#include "../../../../util/property/StateContainer.hpp"
#include "../../Block.hpp"

namespace mc {

// 前向声明
class IWorld;
class IBlockReader;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 雪层方块
 *
 * 可堆叠的雪层方块（1-8层），在光照足够时会融化。
 * 每层高度为 2 像素（1/8 方块）。
 *
 * 放置规则（canSurvive）:
 * 1. 下方方块不在 SNOW_LAYER_CANNOT_SURVIVE_ON 标签中（冰、浮冰、屏障）
 * 2. 下方方块在 SNOW_LAYER_CAN_SURVIVE_ON 标签中时允许放置（蜂蜜块、灵魂沙、泥巴）
 * 3. 否则，下方方块的碰撞形状上表面必须完整，或下方为满层(8层)雪层
 */
class SnowBlock : public Block {
public:
    /**
     * @brief 获取 LAYERS 属性
     */
    [[nodiscard]] static const IntegerProperty& LAYERS() { return BlockStateProperties::LAYERS_1_8(); }

    /**
     * @brief 构造雪层方块
     */
    explicit SnowBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 在光照 > 11 时融化（掉落雪层物品并移除方块）。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 检查是否可以放置在指定位置
     *
     * 检查下方方块是否支持雪层存活：
     * - 下方不能是冰/浮冰/屏障（SNOW_LAYER_CANNOT_SURVIVE_ON）
     * - 下方是蜂蜜块/灵魂沙/泥巴时允许（SNOW_LAYER_CAN_SURVIVE_ON）
     * - 否则下方碰撞形状上表面必须完整，或下方为满层雪层
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居方块更新时检查支撑
     *
     * 当下方方块变化时，如果不再满足放置条件则变为空气。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 检查雪层是否可以在指定位置存活
     *
     * 静态工具方法，供 Biome::shouldSnow 等不持有非 const IWorld 引用的场景使用。
     * 逻辑与 isValidPosition 一致，但接受 const IWorld& 参数。
     *
     * @param world 世界（const 引用）
     * @param pos 雪层位置
     * @return 如果雪层可以存活返回 true
     */
    [[nodiscard]] static bool canSurviveAt(const IWorld& world, const BlockPos& pos);

private:
    /**
     * @brief 检查雪层是否可以在指定位置存活（IWorld版本）
     *
     * 供updatePostPlacement使用，避免IWorld到IBlockReader的向下转型。
     */
    [[nodiscard]] bool _canSurvive(IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
