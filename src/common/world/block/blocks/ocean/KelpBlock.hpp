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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/PlantType.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 海带方块
 *
 * 水下生长的植物，可以堆叠到很高。
 * 从海带方块可以获得海带物品。
 *
 * ## 状态属性
 * - AGE_0_25: 年龄 (0-25)，控制生长阶段
 * - WATERLOGGED: 是否含水
 *
 * ## 生长机制 (MC 1.16.5)
 * - 通过随机 tick 生长
 * - 高度限制基于 AGE_0_25 (最大 25 格)
 * - 只能在水中生长
 * - 生长概率约 14%
 *
 * 参考: net.minecraft.block.KelpBlock
 */
class KelpBlock : public Block, public IPlantable {
public:
    explicit KelpBlock(const BlockProperties& properties);
    ~KelpBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取当前年龄
     * @param state 方块状态
     * @return i32 年龄值（0-25）
     */
    [[nodiscard]] i32 getAge(const BlockState& state) const;

    /**
     * @brief 设置年龄
     * @param age 年龄值（0-25）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] BlockState withAge(i32 age) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== IPlantable 接口 ==========

    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
