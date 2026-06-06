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
#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

/**
 * @brief 滴石尖锥方块
 *
 * 可上下放置的钟乳石/石笋方块，支持含水。
 * 根据厚度属性有不同的碰撞箱形状。
 *
 * 参考: net.minecraft.block.PointedDripstoneBlock
 */
class PointedDripstoneBlock : public Block, public IWaterLoggable {
public:
    explicit PointedDripstoneBlock(const BlockProperties& properties);

    ~PointedDripstoneBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    std::unordered_map<BlockStateProperties::DripstoneThickness, CollisionShape> m_shapes;
};

} // namespace blocks
} // namespace mc
