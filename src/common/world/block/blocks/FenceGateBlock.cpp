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

#include "FenceGateBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/building/WallBlock.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

FenceGateBlock::FenceGateBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::OPEN())
            .add(BlockStateProperties::IN_WALL())
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::OPEN(), false)
            .with(BlockStateProperties::IN_WALL(), false)
            .with(BlockStateProperties::POWERED(), false));

    m_closedShape = VoxelShapes::cube(0.0f, 0.0f, 0.4375f, 1.0f, 1.0f, 0.5625f);
    m_openShape = VoxelShapes::empty();
    m_inWallClosedShape = VoxelShapes::cube(0.0f, 0.0f, 0.4375f, 1.0f, 0.8125f, 0.5625f);

    m_closedShapes[static_cast<size_t>(Direction::North)] = m_closedShape;
    m_openShapes[static_cast<size_t>(Direction::North)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::North)] = m_inWallClosedShape;

    m_closedShapes[static_cast<size_t>(Direction::South)] = m_closedShape;
    m_openShapes[static_cast<size_t>(Direction::South)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::South)] = m_inWallClosedShape;

    m_closedShapes[static_cast<size_t>(Direction::East)] = VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 1.0f);
    m_openShapes[static_cast<size_t>(Direction::East)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::East)] =
        VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 0.8125f, 1.0f);

    m_closedShapes[static_cast<size_t>(Direction::West)] = VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 1.0f);
    m_openShapes[static_cast<size_t>(Direction::West)] = m_openShape;
    m_inWallClosedShapes[static_cast<size_t>(Direction::West)] =
        VoxelShapes::cube(0.4375f, 0.0f, 0.0f, 0.5625f, 0.8125f, 1.0f);
}

BlockState FenceGateBlock::getStateForPlacement(BlockItemUseContext& context)
{
    IWorld& world = const_cast<IWorld&>(context.getWorld());
    Direction facing = context.horizontalDirection();
    bool inWall = _isWall(world, context.placementPos(), facing);
    bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, context.placementPos());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::IN_WALL(), inWall)
        .with(BlockStateProperties::POWERED(), powered);
}

void FenceGateBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);
    bool wasPowered = state.get(BlockStateProperties::POWERED());
    if (powered == wasPowered) {
        return;
    }

    bool wasOpen = state.get(BlockStateProperties::OPEN());
    BlockState newState =
        state.with(BlockStateProperties::POWERED(), powered).with(BlockStateProperties::OPEN(), powered);
    world.setBlockState(pos, &newState, 2);

    if (wasOpen != powered) {
        _playSound(world, pos, powered);
    }
}

BlockState FenceGateBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    Direction gateFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool inWall = _isWall(world, currentPos, gateFacing);
    if (inWall != state.get(BlockStateProperties::IN_WALL())) {
        return state.with(BlockStateProperties::IN_WALL(), inWall);
    }

    return state;
}

BlockActionResult FenceGateBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    bool wasOpen = state.get(BlockStateProperties::OPEN());
    BlockState newState = state.with(BlockStateProperties::OPEN(), !wasOpen);
    world.setBlockState(pos, &newState, 10);
    _playSound(world, pos, !wasOpen);
    return ActionResultType::Success;
}

const CollisionShape& FenceGateBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    bool inWall = state.get(BlockStateProperties::IN_WALL());

    size_t index = static_cast<size_t>(facing);
    if (index >= Directions::COUNT || !Directions::isHorizontal(facing)) {
        index = static_cast<size_t>(Direction::North);
    }

    if (open) {
        return m_openShapes[index];
    }
    if (inWall) {
        return m_inWallClosedShapes[index];
    }
    return m_closedShapes[index];
}

const CollisionShape& FenceGateBlock::getCollisionShape(const BlockState& state) const
{
    if (state.get(BlockStateProperties::OPEN())) {
        return VoxelShapes::empty();
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool inWall = state.get(BlockStateProperties::IN_WALL());

    size_t index = static_cast<size_t>(facing);
    if (index >= Directions::COUNT || !Directions::isHorizontal(facing)) {
        index = static_cast<size_t>(Direction::North);
    }

    return inWall ? m_inWallClosedShapes[index] : m_closedShapes[index];
}

const CollisionShape& FenceGateBlock::getOcclusionShape(const BlockState& state) const
{
    if (state.get(BlockStateProperties::OPEN())) {
        return VoxelShapes::empty();
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    if (index >= Directions::COUNT || !Directions::isHorizontal(facing)) {
        index = static_cast<size_t>(Direction::North);
    }

    return m_closedShapes[index];
}

const BlockState& FenceGateBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& FenceGateBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

bool FenceGateBlock::isOpen(const BlockState& state)
{
    return state.get(BlockStateProperties::OPEN());
}

bool FenceGateBlock::_isWall(const IWorld& world, const BlockPos& pos, Direction facing) const
{
    Direction leftDir;
    Direction rightDir;

    switch (facing) {
        case Direction::North:
            leftDir = Direction::West;
            rightDir = Direction::East;
            break;
        case Direction::South:
            leftDir = Direction::East;
            rightDir = Direction::West;
            break;
        case Direction::East:
            leftDir = Direction::North;
            rightDir = Direction::South;
            break;
        case Direction::West:
            leftDir = Direction::South;
            rightDir = Direction::North;
            break;
        default:
            return false;
    }

    const BlockState* leftState = world.getBlockState(pos.offset(leftDir));
    const BlockState* rightState = world.getBlockState(pos.offset(rightDir));
    return (leftState != nullptr && WallBlock::isWall(*leftState)) ||
        (rightState != nullptr && WallBlock::isWall(*rightState));
}

void FenceGateBlock::_playSound(IWorld& world, const BlockPos& pos, bool isOpening)
{
    const ResourceLocation& soundEvent =
        isOpening ? SoundEvents::BLOCK_FENCE_GATE_OPEN : SoundEvents::BLOCK_FENCE_GATE_CLOSE;
    world.playSound(soundEvent,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

} // namespace blocks
} // namespace mc
