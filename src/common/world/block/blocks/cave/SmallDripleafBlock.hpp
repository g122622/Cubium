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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IGrowable.hpp"
#include "../../IWaterLoggable.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/PlantType.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 小滴叶方块
 *
 * 小型装饰植物，可放置在水下，支持含水。
 * 有上下两部分组成（通过DOUBLE_BLOCK_HALF属性区分）。
 * 使用骨粉可以生长为大滴叶（1-5格茎+1格叶片）。
 *
 * 参考: net.minecraft.block.SmallDripleafBlock
 */
class SmallDripleafBlock : public Block, public IWaterLoggable, public IGrowable, public IPlantable {
public:
    explicit SmallDripleafBlock(const BlockProperties& properties);

    ~SmallDripleafBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== IGrowable 接口 ==========

    /**
     * @brief 小滴叶上方有足够空间时可以生长
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 骨粉对小滴叶总是有效
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长为大滴叶
     *
     * 移除小滴叶，在原位置放置1-5格大滴叶茎+1格大滴叶叶片
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== IPlantable 接口 ==========

    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    /**
     * @brief 检查此方块是否可以放置在指定方块上方
     *
     * 小滴叶可放置在以下方块上：
     * - #minecraft:small_dripleaf_placeable 标签中的方块（黏土、苔藓块）
     * - 下方位置有水源时，#minecraft:dirt 标签中的方块或耕地
     *
     * 参考: net.minecraft.block.SmallDripleafBlock.mayPlaceOn
     */
    [[nodiscard]] virtual bool mayPlaceOn(const BlockState& state, IBlockReader& world, const BlockPos& pos) const;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
