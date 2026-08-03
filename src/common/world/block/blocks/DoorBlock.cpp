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

#include "DoorBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../IWorld.hpp"
#include "../../redstone/RedstoneSystem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

DoorBlock::DoorBlock(const BlockProperties& properties, bool isIron)
    : Block(properties)
    , m_isIron(isIron)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::OPEN())
            .add(BlockStateProperties::HINGE())
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
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::OPEN(), false)
            .with(BlockStateProperties::HINGE(), BlockStateProperties::DoorHinge::Left)
            .with(BlockStateProperties::POWERED(), false));

    m_shapes[0] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
    m_shapes[1] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f);
    m_shapes[2] = VoxelShapes::cube(13.0f, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    m_shapes[3] = VoxelShapes::cube(0.0f, 0.0f, 13.0f, 16.0f, 16.0f, 16.0f);
    m_shapes[4] = VoxelShapes::cube(0.0f, 0.0f, 13.0f, 16.0f, 16.0f, 16.0f);
    m_shapes[5] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
    m_shapes[6] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f);
    m_shapes[7] = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
}

BlockState DoorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockPos pos = context.placementPos();
    IWorld& world = const_cast<IWorld&>(context.getWorld());

    // 检查上方位置是否可替换（空气、花草、水等 canBeReplaced=true 的方块）
    // canBeReplaced() 已包含 isAir() 的语义（空气方块的 Material::AIR.isReplaceable()=true）
    const BlockState* upState = world.getBlockState(pos.up());
    if (pos.y >= world::MAX_BUILD_HEIGHT - 1 || upState == nullptr || !upState->canBeReplaced()) {
        return defaultState();
    }

    const bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos) ||
        world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos.up());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection())
        .with(BlockStateProperties::HINGE(), _calculateHingeSide(context))
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
}

void DoorBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(stack);

    BlockPos abovePos = pos.up();
    BlockState upperState =
        state.with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockState(abovePos, &upperState, 3);
}

void DoorBlock::neighborChanged(
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
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
    BlockStateProperties::DoubleBlockHalf otherHalf = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? BlockStateProperties::DoubleBlockHalf::Upper
        : BlockStateProperties::DoubleBlockHalf::Lower;
    BlockPos otherHalfPos = (half == BlockStateProperties::DoubleBlockHalf::Lower) ? BlockPos(pos.x, pos.y + 1, pos.z)
                                                                                   : BlockPos(pos.x, pos.y - 1, pos.z);

    const bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos) ||
        world::redstone::RedstoneSystem::instance().isBlockPowered(world, otherHalfPos);
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    if (powered != wasPowered) {
        bool wasOpen = state.get(BlockStateProperties::OPEN());
        BlockState newState =
            state.with(BlockStateProperties::POWERED(), powered).with(BlockStateProperties::OPEN(), powered);
        world.setBlockState(pos, &newState, 2);

        const BlockState* otherStatePtr = world.getBlockState(otherHalfPos);
        if (otherStatePtr != nullptr && &otherStatePtr->getBlock() == this &&
            otherStatePtr->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == otherHalf) {
            BlockState otherState = newState.with(BlockStateProperties::DOUBLE_BLOCK_HALF(), otherHalf);
            world.setBlockState(otherHalfPos, &otherState, 2);
        }
        if (wasOpen != powered) {
            _playSound(world, pos, powered);
        }
    }
}

BlockState DoorBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingPos);

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());

    if (Directions::getAxis(facing) == Axis::Y) {
        bool isLower = half == BlockStateProperties::DoubleBlockHalf::Lower;
        bool isUpDirection = facing == Direction::Up;

        if (isLower == isUpDirection) {
            if (&facingState.getBlock() == this && facingState.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) != half) {
                return state
                    .with(BlockStateProperties::HORIZONTAL_FACING(),
                        facingState.get(BlockStateProperties::HORIZONTAL_FACING()))
                    .with(BlockStateProperties::OPEN(), facingState.get(BlockStateProperties::OPEN()))
                    .with(BlockStateProperties::HINGE(), facingState.get(BlockStateProperties::HINGE()))
                    .with(BlockStateProperties::POWERED(), facingState.get(BlockStateProperties::POWERED()));
            }
            return VanillaBlocks::AIR->defaultState();
        }
    }

    if (half == BlockStateProperties::DoubleBlockHalf::Lower && facing == Direction::Down) {
        BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !belowState->isSolidSide(world, belowPos, Direction::Up)) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

bool DoorBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
    if (half == BlockStateProperties::DoubleBlockHalf::Lower) {
        const BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        return belowState != nullptr && belowState->isSolidSide(world, belowPos, Direction::Up);
    }

    const BlockState* belowState = world.getBlockState(pos.down());
    return belowState != nullptr && &belowState->getBlock() == this &&
        belowState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;
}

BlockActionResult DoorBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (m_isIron) {
        return ActionResultType::Pass;
    }

    toggleDoor(world, pos, !state.get(BlockStateProperties::OPEN()));
    return ActionResultType::Success;
}

void DoorBlock::toggleDoor(IWorld& world, const BlockPos& pos, bool open)
{
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    if (state.get(BlockStateProperties::OPEN()) == open) {
        return;
    }

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
    BlockStateProperties::DoubleBlockHalf otherHalf = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? BlockStateProperties::DoubleBlockHalf::Upper
        : BlockStateProperties::DoubleBlockHalf::Lower;
    BlockPos otherHalfPos = (half == BlockStateProperties::DoubleBlockHalf::Lower) ? pos.up() : pos.down();

    BlockState newState = state.with(BlockStateProperties::OPEN(), open);
    world.setBlockState(pos, &newState, 10);
    BlockState otherState = newState.with(BlockStateProperties::DOUBLE_BLOCK_HALF(), otherHalf);
    world.setBlockState(otherHalfPos, &otherState, 10);
    _playSound(world, pos, open);
}

const CollisionShape& DoorBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    bool hingeRight = state.get(BlockStateProperties::HINGE()) == BlockStateProperties::DoorHinge::Right;
    return m_shapes[_getShapeIndex(facing, open, hingeRight)];
}

const CollisionShape& DoorBlock::getCollisionShape(const BlockState& state) const
{
    return getShape(state);
}

const BlockState& DoorBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& DoorBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Rotation rotation = Directions::mirrorToRotation(mirror, state.get(BlockStateProperties::HORIZONTAL_FACING()));
    const BlockState& rotated = rotate(state, rotation);
    BlockStateProperties::DoorHinge hinge = rotated.get(BlockStateProperties::HINGE());
    BlockStateProperties::DoorHinge newHinge = (hinge == BlockStateProperties::DoorHinge::Left)
        ? BlockStateProperties::DoorHinge::Right
        : BlockStateProperties::DoorHinge::Left;
    return rotated.with(BlockStateProperties::HINGE(), newHinge);
}

bool DoorBlock::isOpen(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::OPEN());
}

bool DoorBlock::isWooden(const BlockState& state) noexcept
{
    const Block* block = &state.getBlock();
    if (auto* doorBlock = dynamic_cast<const DoorBlock*>(block)) {
        const Material& mat = doorBlock->material();
        return mat == Material::WOOD || mat == Material::NETHER_WOOD;
    }
    return false;
}

BlockStateProperties::DoorHinge DoorBlock::_calculateHingeSide(BlockItemUseContext& context)
{
    const IWorld& reader = context.getWorld();
    BlockPos pos = context.placementPos();
    Direction facing = context.horizontalDirection();

    Direction leftDir = Directions::rotateYCCW(facing);
    Direction rightDir = Directions::rotateY(facing);

    BlockPos leftPos(pos.x + Directions::xOffset(leftDir), pos.y, pos.z + Directions::zOffset(leftDir));
    BlockPos leftUpPos(leftPos.x, leftPos.y + 1, leftPos.z);
    BlockPos rightPos(pos.x + Directions::xOffset(rightDir), pos.y, pos.z + Directions::zOffset(rightDir));
    BlockPos rightUpPos(rightPos.x, rightPos.y + 1, rightPos.z);

    const BlockState* leftState = reader.getBlockState(leftPos);
    const BlockState* leftUpState = reader.getBlockState(leftUpPos);
    const BlockState* rightState = reader.getBlockState(rightPos);
    const BlockState* rightUpState = reader.getBlockState(rightUpPos);

    i32 score = 0;
    if (leftState != nullptr && leftState->hasOpaqueCollisionShape()) score--;
    if (leftUpState != nullptr && leftUpState->hasOpaqueCollisionShape()) score--;
    if (rightState != nullptr && rightState->hasOpaqueCollisionShape()) score++;
    if (rightUpState != nullptr && rightUpState->hasOpaqueCollisionShape()) score++;

    bool hasLeftDoor = leftState != nullptr && &leftState->getBlock() == this &&
        leftState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;
    bool hasRightDoor = rightState != nullptr && &rightState->getBlock() == this &&
        rightState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;

    if ((!hasLeftDoor || hasRightDoor) && score <= 0) {
        if ((!hasRightDoor || hasLeftDoor) && score >= 0) {
            i32 dx = Directions::xOffset(facing);
            i32 dz = Directions::zOffset(facing);
            f32 hitX = context.getHitX() - static_cast<f32>(pos.x);
            f32 hitZ = context.getHitZ() - static_cast<f32>(pos.z);

            if ((dx >= 0 || !(hitZ < 0.5f)) && (dx <= 0 || !(hitZ > 0.5f)) && (dz >= 0 || !(hitX > 0.5f)) &&
                (dz <= 0 || !(hitX < 0.5f))) {
                return BlockStateProperties::DoorHinge::Left;
            }
            return BlockStateProperties::DoorHinge::Right;
        }
        return BlockStateProperties::DoorHinge::Left;
    }
    return BlockStateProperties::DoorHinge::Right;
}

void DoorBlock::_playSound(IWorld& world, const BlockPos& pos, bool isOpening)
{
    const ResourceLocation& soundEvent = isOpening
        ? (m_isIron ? SoundEvents::BLOCK_IRON_DOOR_OPEN : SoundEvents::BLOCK_WOODEN_DOOR_OPEN)
        : (m_isIron ? SoundEvents::BLOCK_IRON_DOOR_CLOSE : SoundEvents::BLOCK_WOODEN_DOOR_CLOSE);
    world.playSound(soundEvent,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

i32 DoorBlock::_getOpenSound() const
{
    // 铁门开门音效 1005，木门开门音效 1006
    return m_isIron ? 1005 : 1006;
}

i32 DoorBlock::_getCloseSound() const
{
    // 铁门关门音效 1011，木门关门音效 1012
    return m_isIron ? 1011 : 1012;
}

size_t DoorBlock::_getShapeIndex(Direction facing, bool open, bool hingeRight) noexcept
{
    if (!open) {
        switch (facing) {
            case Direction::East:
                return 0;
            case Direction::South:
                return 1;
            case Direction::West:
                return 2;
            case Direction::North:
                return 3;
            default:
                return 0;
        }
    }

    if (hingeRight) {
        switch (facing) {
            case Direction::East:
                return 5;
            case Direction::South:
                return 6;
            case Direction::West:
                return 7;
            case Direction::North:
                return 4;
            default:
                return 4;
        }
    } else {
        switch (facing) {
            case Direction::East:
                return 4;
            case Direction::South:
                return 5;
            case Direction::West:
                return 6;
            case Direction::North:
                return 7;
            default:
                return 4;
        }
    }
}

} // namespace blocks
} // namespace mc
