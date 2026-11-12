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

#include "common/world/block/blocks/sign/WallSignBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include <cstddef>

namespace mc {
namespace blocks {

// ========== WallSignBlock ==========

WallSignBlock::WallSignBlock(const BlockProperties& properties, const WoodType& woodType)
    : AbstractSignBlock(properties, woodType, woodType.soundType)
{

    // 各方向的碰撞形状（贴在墙面的薄板）
    m_shapesByDirection[Direction::North] = CollisionShape::box(0.0f, 0.28125f, 0.875f, 1.0f, 0.78125f, 1.0f);
    m_shapesByDirection[Direction::South] = CollisionShape::box(0.0f, 0.28125f, 0.0f, 1.0f, 0.78125f, 0.125f);
    m_shapesByDirection[Direction::East] = CollisionShape::box(0.0f, 0.28125f, 0.0f, 0.125f, 0.78125f, 1.0f);
    m_shapesByDirection[Direction::West] = CollisionShape::box(0.875f, 0.28125f, 0.0f, 1.0f, 0.78125f, 1.0f);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState WallSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = waterloggable::isWaterFluidState(fluidState);

    // 尝试找到可以附着的墙面
    for (Direction dir : context.getNearestLookingDirections()) {
        if (Directions::isHorizontal(dir)) {
            Direction facing = Directions::opposite(dir);
            BlockState state = defaultState()
                                   .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
                                   .with(BlockStateProperties::WATERLOGGED(), waterlogged);

            // 使用 IBlockReader 接口检查是否可放置
            IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
            if (isValidPosition(state, blockReader, pos)) {
                return state;
            }
        }
    }

    // 无法放置，返回空状态（会让放置失败）
    return defaultState();
}

bool WallSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction oppositeDir = Directions::opposite(facing);
    BlockPos adjPos = pos.offset(oppositeDir);
    const BlockState* adjState = world.getBlockState(adjPos);

    return adjState != nullptr && adjState->isSolidSide(world, adjPos, facing);
}

const BlockState& WallSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& WallSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& WallSignBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
