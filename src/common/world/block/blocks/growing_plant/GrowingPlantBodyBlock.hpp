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
 */

#pragma once

#include "GrowingPlantBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 生长植物身体方块（茎/干）
 *
 * 参考 MC 1.21.11: GrowingPlantBodyBlock
 * 生长植物的非尖端部分，不具备独立生长能力。
 *
 * 提供了 updatePostPlacement() 的通用逻辑：
 * - 当头部的支撑被移除时，身体方块会在断裂时变成头部
 */
class GrowingPlantBodyBlock : public GrowingPlantBlock {
public:
    /**
     * @brief 构造生长植物身体方块
     * @param properties 方块属性
     * @param growthDirection 生长方向（UP 或 DOWN）
     * @param shape 碰撞形状
     * @param scheduleFluidTicks 是否调度流体 tick
     */
    GrowingPlantBodyBlock(const BlockProperties& properties,
        Direction growthDirection,
        const CollisionShape& shape,
        bool scheduleFluidTicks = false);

    ~GrowingPlantBodyBlock() override = default;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 当头部从身体方块转换时调用
     *
     * 子类可以覆盖以将身体状态传递到头部方块
     * （如 CaveVines 传递 BERRIES 属性）。
     */
    [[nodiscard]] virtual BlockState updateHeadAfterConvertedFromBody(const BlockState& bodyState) const;
};

} // namespace blocks
} // namespace mc
