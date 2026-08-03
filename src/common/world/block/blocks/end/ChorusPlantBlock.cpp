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

#include "ChorusPlantBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

ChorusPlantBlock::ChorusPlantBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .add(BlockStateProperties::DOWN())
            .add(BlockStateProperties::UP())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::DOWN(), false)
            .with(BlockStateProperties::UP(), false));

    // 形状计算：中心柱 + 各方向臂
    constexpr f32 apothem = 0.3125f;
    constexpr f32 f = 0.5f - apothem;
    constexpr f32 f1 = 0.5f + apothem;

    m_centerShape = CollisionShape::box(f, f, f, f1, f1, f1);

    for (i32 i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        f32 xOffset = static_cast<f32>(Directions::xOffset(dir));
        f32 yOffset = static_cast<f32>(Directions::yOffset(dir));
        f32 zOffset = static_cast<f32>(Directions::zOffset(dir));

        f32 minX = 0.5f + std::min(-apothem, xOffset * 0.5f);
        f32 minY = 0.5f + std::min(-apothem, yOffset * 0.5f);
        f32 minZ = 0.5f + std::min(-apothem, zOffset * 0.5f);
        f32 maxX = 0.5f + std::max(apothem, xOffset * 0.5f);
        f32 maxY = 0.5f + std::max(apothem, yOffset * 0.5f);
        f32 maxZ = 0.5f + std::max(apothem, zOffset * 0.5f);

        m_armShapes[i] = CollisionShape::box(minX, minY, minZ, maxX, maxY, maxZ);
    }

    // 预计算所有 64 种连接组合的形状
    for (size_t k = 0; k < 64; ++k) {
        CollisionShape shape = m_centerShape;

        for (i32 j = 0; j < 6; ++j) {
            if ((k & (1ULL << j)) != 0) {
                shape = CollisionShape::combine(shape, m_armShapes[j]);
            }
        }

        m_shapes[k] = shape;
    }
}

BlockState ChorusPlantBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    const IBlockReader& blockReader = static_cast<const IBlockReader&>(world);
    bool north = _canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::North);
    bool south = _canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::South);
    bool east = _canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::East);
    bool west = _canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::West);
    bool up = _canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::Up);
    bool down = _canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::Down);

    return defaultState()
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::WEST(), west)
        .with(BlockStateProperties::UP(), up)
        .with(BlockStateProperties::DOWN(), down);
}

bool ChorusPlantBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    for (i32 i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        if (_canConnect(world, pos, dir)) {
            return true;
        }
    }

    return false;
}

BlockState ChorusPlantBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    bool connected = _canConnect(blockReader, currentPos, facing);

    switch (facing) {
        case Direction::North:
            return state.with(BlockStateProperties::NORTH(), connected);
        case Direction::South:
            return state.with(BlockStateProperties::SOUTH(), connected);
        case Direction::East:
            return state.with(BlockStateProperties::EAST(), connected);
        case Direction::West:
            return state.with(BlockStateProperties::WEST(), connected);
        case Direction::Up:
            return state.with(BlockStateProperties::UP(), connected);
        case Direction::Down:
            return state.with(BlockStateProperties::DOWN(), connected);
        default:
            return state;
    }
}

const CollisionShape& ChorusPlantBlock::getShape(const BlockState& state) const
{
    const size_t index = getShapeIndex(state);
    MC_ASSERT_RELEASE(index < m_shapes.size());
    return m_shapes[index];
}

size_t ChorusPlantBlock::getShapeIndex(const BlockState& state)
{
    size_t index = 0;

    if (state.get(BlockStateProperties::DOWN())) index |= 1ULL << 0;
    if (state.get(BlockStateProperties::UP())) index |= 1ULL << 1;
    if (state.get(BlockStateProperties::NORTH())) index |= 1ULL << 2;
    if (state.get(BlockStateProperties::SOUTH())) index |= 1ULL << 3;
    if (state.get(BlockStateProperties::WEST())) index |= 1ULL << 4;
    if (state.get(BlockStateProperties::EAST())) index |= 1ULL << 5;

    return index;
}

bool ChorusPlantBlock::_canConnect(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    const Block& adjBlock = adjState->getBlock();

    if (adjState->is(this)) {
        return true;
    }

    if (&adjBlock == VanillaBlocks::CHORUS_FLOWER) {
        return true;
    }

    if (direction == Direction::Down && &adjBlock == VanillaBlocks::END_STONE) {
        return true;
    }

    return false;
}

BlockState ChorusPlantBlock::getStateWithConnections(IWorld& world, const BlockPos& pos, const BlockState& defaultState)
{
    const ChorusPlantBlock& block = static_cast<const ChorusPlantBlock&>(defaultState.getBlock());
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);

    bool north = block._canConnect(blockReader, pos, Direction::North);
    bool south = block._canConnect(blockReader, pos, Direction::South);
    bool east = block._canConnect(blockReader, pos, Direction::East);
    bool west = block._canConnect(blockReader, pos, Direction::West);
    bool up = block._canConnect(blockReader, pos, Direction::Up);
    bool down = block._canConnect(blockReader, pos, Direction::Down);

    return defaultState.with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::WEST(), west)
        .with(BlockStateProperties::UP(), up)
        .with(BlockStateProperties::DOWN(), down);
}

} // namespace blocks
} // namespace mc
