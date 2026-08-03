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

#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <cmath>

namespace mc {
namespace blocks {

/**
 * @brief 生长植物方块基类
 *
 * 参考 MC 1.21.11: GrowingPlantBlock
 * 向上或向下生长的植物方块的共同基类。
 * 子类必须实现 getHeadBlock() 和 getBodyBlock()。
 *
 * 提供了 isValidPosition() 和 updatePostPlacement() 的通用逻辑：
 * - 在生长方向的反方向必须有同类方块或坚固支撑面
 * - 支撑丢失时自动断裂
 */
class GrowingPlantBlock : public Block {
public:
    /**
     * @brief 构造生长植物方块
     * @param properties 方块属性
     * @param growthDirection 生长方向（UP 或 DOWN）
     * @param shape 碰撞形状
     * @param scheduleFluidTicks 是否调度流体 tick（含水植物需要）
     */
    GrowingPlantBlock(const BlockProperties& properties,
        Direction growthDirection,
        const CollisionShape& shape,
        bool scheduleFluidTicks = false);

    ~GrowingPlantBlock() override = default;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取头部方块（生长尖端）
     */
    [[nodiscard]] virtual const Block* getHeadBlock() const = 0;

    /**
     * @brief 获取身体方块（茎/干）
     */
    [[nodiscard]] virtual const Block* getBodyBlock() const = 0;

    /**
     * @brief 获取生长方向
     */
    [[nodiscard]] Direction getGrowthDirection() const noexcept { return m_growthDirection; }

protected:
    Direction m_growthDirection;
    CollisionShape m_shape;
    bool m_scheduleFluidTicks;
};

} // namespace blocks
} // namespace mc
