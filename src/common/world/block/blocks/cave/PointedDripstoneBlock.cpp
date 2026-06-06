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

#include "PointedDripstoneBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

PointedDripstoneBlock::PointedDripstoneBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建各厚度的碰撞形状
    // TipMerge: 最细 (5-11像素宽)
    m_shapes[BlockStateProperties::DripstoneThickness::TipMerge] = CollisionShape::fromPixelBox(5, 0, 5, 11, 16, 11);
    // Tip: 细 (4-12像素宽)
    m_shapes[BlockStateProperties::DripstoneThickness::Tip] = CollisionShape::fromPixelBox(4, 0, 4, 12, 16, 12);
    // Frustum: 中等 (3-13像素宽)
    m_shapes[BlockStateProperties::DripstoneThickness::Frustum] = CollisionShape::fromPixelBox(3, 0, 3, 13, 16, 13);
    // Middle: 较粗 (2-14像素宽)
    m_shapes[BlockStateProperties::DripstoneThickness::Middle] = CollisionShape::fromPixelBox(2, 0, 2, 14, 16, 14);
    // Base: 最粗 (1-15像素宽)
    m_shapes[BlockStateProperties::DripstoneThickness::Base] = CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::VERTICAL_DIRECTION())
            .add(BlockStateProperties::DRIPSTONE_THICKNESS())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void PointedDripstoneBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState PointedDripstoneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction clickedFace = context.getClickedFace();
    Direction verticalDirection = Direction::Up;

    // 根据点击面确定垂直方向
    if (clickedFace == Direction::Up) {
        verticalDirection = Direction::Up;
    } else if (clickedFace == Direction::Down) {
        verticalDirection = Direction::Down;
    }
    // 侧面的情况默认向上

    BlockState state =
        defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), verticalDirection)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState PointedDripstoneBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& PointedDripstoneBlock::getShape(const BlockState& state) const
{
    BlockStateProperties::DripstoneThickness thickness = state.get(BlockStateProperties::DRIPSTONE_THICKNESS());
    auto it = m_shapes.find(thickness);
    if (it != m_shapes.end()) {
        return it->second;
    }
    return m_shapes.at(BlockStateProperties::DripstoneThickness::Tip);
}

const fluid::FluidState* PointedDripstoneBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void PointedDripstoneBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // TODO: 实现滴石生长逻辑
}

const BlockState& PointedDripstoneBlock::rotate(const BlockState& state, Rotation rotation) const
{
    MC_UNUSED(rotation);
    // 垂直方向不受旋转影响
    return state;
}

const BlockState& PointedDripstoneBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 对于VERTICAL_DIRECTION，镜像时Up/Down互换
    Direction verticalDir = state.get(BlockStateProperties::VERTICAL_DIRECTION());
    if (mirror == Mirror::FrontBack || mirror == Mirror::LeftRight) {
        // 垂直方向在镜像时交换
        if (verticalDir == Direction::Up) {
            return state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
        } else if (verticalDir == Direction::Down) {
            return state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
        }
    }
    return state;
}

} // namespace blocks
} // namespace mc
