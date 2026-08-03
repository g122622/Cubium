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

#include "MultifaceBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

const BooleanProperty* MultifaceBlock::getFaceProperty(Direction direction)
{
    switch (direction) {
        case Direction::North:
            return &BlockStateProperties::NORTH();
        case Direction::South:
            return &BlockStateProperties::SOUTH();
        case Direction::East:
            return &BlockStateProperties::EAST();
        case Direction::West:
            return &BlockStateProperties::WEST();
        case Direction::Up:
            return &BlockStateProperties::UP();
        case Direction::Down:
            return &BlockStateProperties::DOWN();
        default:
            return nullptr;
    }
}

bool MultifaceBlock::hasFace(const BlockState& state, Direction direction)
{
    const BooleanProperty* prop = getFaceProperty(direction);
    if (prop == nullptr) {
        return false;
    }
    return state.get(*prop);
}

bool MultifaceBlock::hasAnyFace(const BlockState& state)
{
    static constexpr Direction kFaces[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    for (Direction dir : kFaces) {
        if (hasFace(state, dir)) {
            return true;
        }
    }
    return false;
}

std::vector<Direction> MultifaceBlock::availableFaces(const BlockState& state)
{
    // MC MultifaceBlock.availableFaces：非 MultifaceBlock 返回空集，否则收集所有 hasFace 的方向。
    std::vector<Direction> faces;
    if (dynamic_cast<const MultifaceBlock*>(&state.getBlock()) == nullptr) {
        return faces;
    }
    static constexpr Direction kFaces[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    for (Direction dir : kFaces) {
        if (hasFace(state, dir)) {
            faces.push_back(dir);
        }
    }
    return faces;
}

u8 MultifaceBlock::pack(const std::vector<Direction>& directions)
{
    // MC MultifaceBlock.pack：按 direction.ordinal() 置位。
    u8 mask = 0;
    for (Direction dir : directions) {
        mask |= static_cast<u8>(1u << static_cast<u8>(ordinal(dir)));
    }
    return mask;
}

bool MultifaceBlock::isValidStateForPlacement(
    IWorld& world, const BlockState& state, const BlockPos& pos, Direction direction) const
{
    // MC: isFaceSupported(direction) && (!state.is(this) || !hasFace(state, direction))
    //     && canAttachTo(reader, direction, pos.relative(direction), reader.getBlockState(pos.relative(direction)))
    if (!isFaceSupported(direction)) {
        return false;
    }
    if (state.is(this) && hasFace(state, direction)) {
        return false;
    }
    const BlockPos neighbor = pos.offset(direction);
    const BlockState* neighborState = world.getBlockState(neighbor);
    if (neighborState == nullptr) {
        return false;
    }
    return canAttachTo(world, direction, neighbor);
}

const BlockState* MultifaceBlock::getStateForPlacement(
    const BlockState* currentState, IWorld& world, const BlockPos& pos, Direction direction) const
{
    // MC: isValidStateForPlacement(reader, state, pos, direction)
    //   state 为空气时 is(this)=false、hasFace 无意义，仍可通过支撑判定。
    //   此处 nullptr 视为空气：直接进入 canAttachTo 判定。
    const BooleanProperty* faceProp = getFaceProperty(direction);
    if (faceProp == nullptr) {
        return nullptr;
    }
    if (!isFaceSupported(direction)) {
        return nullptr;
    }
    // 已是本方块且该朝向已有面 → 不可重复加面。
    if (currentState != nullptr && currentState->is(this) && hasFace(*currentState, direction)) {
        return nullptr;
    }
    // 相邻格需实心可附着。
    const BlockPos neighbor = pos.offset(direction);
    const BlockState* neighborState = world.getBlockState(neighbor);
    if (neighborState == nullptr || !canAttachTo(world, direction, neighbor)) {
        return nullptr;
    }

    const BlockState* result;
    if (currentState != nullptr && currentState->is(this)) {
        // 已是同种多面方块：保留现有面
        result = currentState;
    } else if (currentState != nullptr) {
        const fluid::FluidState* fluid = currentState->getFluidState();
        if (fluid != nullptr && fluid->isSource() && &fluid->getFluid() == fluid::Fluids::WATER()) {
            // 当前格是水源：水化
            result = &defaultState().with(BlockStateProperties::WATERLOGGED(), true);
        } else {
            result = &defaultState();
        }
    } else {
        // nullptr 视为空气
        result = &defaultState();
    }

    return &result->with(*faceProp, true);
}

bool MultifaceBlock::canAttachTo(IWorld& world, Direction direction, const BlockPos& neighborPos)
{
    // MC MultifaceBlock.canAttachTo:
    //   Block.isFaceFull(state.getBlockSupportShape(reader, pos), dir.getOpposite())
    //   || Block.isFaceFull(state.getCollisionShape(reader, pos), dir.getOpposite())
    const BlockState* state = world.getBlockState(neighborPos);
    if (state == nullptr || state->isAir()) {
        return false;
    }
    const Direction opposite = Directions::opposite(direction);
    if (Block::isFaceFull(state->getBlockSupportShape(), opposite)) {
        return true;
    }
    return Block::isFaceFull(state->getCollisionShape(), opposite);
}

void MultifaceBlock::buildMultifaceStateContainer()
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .add(BlockStateProperties::UP())
            .add(BlockStateProperties::DOWN())
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
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::UP(), false)
            .with(BlockStateProperties::DOWN(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

} // namespace blocks
} // namespace mc
