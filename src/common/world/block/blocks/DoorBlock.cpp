#include "DoorBlock.hpp"
#include "../../IWorld.hpp"
#include "../../redstone/RedstoneSystem.hpp"
#include "../BlockRegistry.hpp"
#include "../VanillaBlocks.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

DoorBlock::DoorBlock(const BlockProperties& properties, bool isIron)
    : Block(properties)
    , m_isIron(isIron) {
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HALF())
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::OPEN())
        .add(BlockStateProperties::HINGE())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
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

BlockState DoorBlock::getStateForPlacement(BlockItemUseContext& context) {
    BlockPos pos = context.placementPos();
    IWorld& world = const_cast<IWorld&>(context.getWorld());

    const BlockState* upState = world.getBlockState(pos.up());
    if (pos.y >= 255 || upState == nullptr || !upState->isAir()) {
        const Material* mat = upState ? &upState->getMaterial() : nullptr;
        if (mat == nullptr || !mat->isReplaceable()) {
            return defaultState();
        }
    }

    const bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos) ||
                         world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos.up());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection())
        .with(BlockStateProperties::HINGE(), calculateHingeSide(context))
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
}

void DoorBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state) {
    BlockPos abovePos = pos.up();
    BlockState upperState = state.with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockState(abovePos, &upperState, 3);
}

void DoorBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                Block& neighborBlock, const BlockPos& neighborPos,
                                bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());
    BlockStateProperties::DoubleBlockHalf otherHalf = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? BlockStateProperties::DoubleBlockHalf::Upper
        : BlockStateProperties::DoubleBlockHalf::Lower;
    BlockPos otherHalfPos = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? BlockPos(pos.x, pos.y + 1, pos.z)
        : BlockPos(pos.x, pos.y - 1, pos.z);

    const bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos) ||
                         world::redstone::RedstoneSystem::instance().isBlockPowered(world, otherHalfPos);
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    if (powered != wasPowered) {
        bool wasOpen = state.get(BlockStateProperties::OPEN());
        BlockState newState = state
            .with(BlockStateProperties::POWERED(), powered)
            .with(BlockStateProperties::OPEN(), powered);
        world.setBlockState(pos, &newState, 2);

        const BlockState* otherStatePtr = world.getBlockState(otherHalfPos);
        if (otherStatePtr != nullptr &&
            &otherStatePtr->getBlock() == this &&
            otherStatePtr->get(BlockStateProperties::HALF()) == otherHalf) {
            BlockState otherState = newState.with(BlockStateProperties::HALF(), otherHalf);
            world.setBlockState(otherHalfPos, &otherState, 2);
        }
        if (wasOpen != powered) {
            playSound(world, pos, powered);
        }
    }
}

BlockState DoorBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingPos);

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());

    if (Directions::getAxis(facing) == Axis::Y) {
        bool isLower = half == BlockStateProperties::DoubleBlockHalf::Lower;
        bool isUpDirection = facing == Direction::Up;

        if (isLower == isUpDirection) {
            if (&facingState.getBlock() == this &&
                facingState.get(BlockStateProperties::HALF()) != half) {
                return state
                    .with(BlockStateProperties::HORIZONTAL_FACING(), facingState.get(BlockStateProperties::HORIZONTAL_FACING()))
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

bool DoorBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const {
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());
    if (half == BlockStateProperties::DoubleBlockHalf::Lower) {
        const BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        return belowState != nullptr && belowState->isSolidSide(world, belowPos, Direction::Up);
    }

    const BlockState* belowState = world.getBlockState(pos.down());
    return belowState != nullptr &&
           &belowState->getBlock() == this &&
           belowState->get(BlockStateProperties::HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;
}

ActionResultType DoorBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (m_isIron) {
        return ActionResultType::Pass;
    }

    toggleDoor(world, pos, !state.get(BlockStateProperties::OPEN()));
    return ActionResultType::Success;
}

void DoorBlock::toggleDoor(IWorld& world, const BlockPos& pos, bool open) {
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || &statePtr->getBlock() != this) {
        return;
    }

    BlockState state = *statePtr;
    if (state.get(BlockStateProperties::OPEN()) == open) {
        return;
    }

    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());
    BlockStateProperties::DoubleBlockHalf otherHalf = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? BlockStateProperties::DoubleBlockHalf::Upper
        : BlockStateProperties::DoubleBlockHalf::Lower;
    BlockPos otherHalfPos = (half == BlockStateProperties::DoubleBlockHalf::Lower)
        ? pos.up()
        : pos.down();

    BlockState newState = state.with(BlockStateProperties::OPEN(), open);
    world.setBlockState(pos, &newState, 10);
    BlockState otherState = newState.with(BlockStateProperties::HALF(), otherHalf);
    world.setBlockState(otherHalfPos, &otherState, 10);
    playSound(world, pos, open);
}

const CollisionShape& DoorBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    bool hingeRight = state.get(BlockStateProperties::HINGE()) == BlockStateProperties::DoorHinge::Right;
    return m_shapes[getShapeIndex(facing, open, hingeRight)];
}

const CollisionShape& DoorBlock::getCollisionShape(const BlockState& state) const {
    return getShape(state);
}

const BlockState& DoorBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& DoorBlock::mirror(const BlockState& state, Mirror mirror) const {
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

bool DoorBlock::isOpen(const BlockState& state) {
    return state.get(BlockStateProperties::OPEN());
}

bool DoorBlock::isWooden(const BlockState& state) {
    const Block* block = &state.getBlock();
    if (auto* doorBlock = dynamic_cast<const DoorBlock*>(block)) {
        const Material& mat = doorBlock->material();
        return mat == Material::WOOD || mat == Material::NETHER_WOOD;
    }
    return false;
}

BlockStateProperties::DoorHinge DoorBlock::calculateHingeSide(BlockItemUseContext& context) {
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

    bool hasLeftDoor = leftState != nullptr &&
                       &leftState->getBlock() == this &&
                       leftState->get(BlockStateProperties::HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;
    bool hasRightDoor = rightState != nullptr &&
                        &rightState->getBlock() == this &&
                        rightState->get(BlockStateProperties::HALF()) == BlockStateProperties::DoubleBlockHalf::Lower;

    if ((!hasLeftDoor || hasRightDoor) && score <= 0) {
        if ((!hasRightDoor || hasLeftDoor) && score >= 0) {
            i32 dx = Directions::xOffset(facing);
            i32 dz = Directions::zOffset(facing);
            f32 hitX = context.getHitX() - static_cast<f32>(pos.x);
            f32 hitZ = context.getHitZ() - static_cast<f32>(pos.z);

            if ((dx >= 0 || !(hitZ < 0.5f)) &&
                (dx <= 0 || !(hitZ > 0.5f)) &&
                (dz >= 0 || !(hitX > 0.5f)) &&
                (dz <= 0 || !(hitX < 0.5f))) {
                return BlockStateProperties::DoorHinge::Left;
            }
            return BlockStateProperties::DoorHinge::Right;
        }
        return BlockStateProperties::DoorHinge::Left;
    }
    return BlockStateProperties::DoorHinge::Right;
}

void DoorBlock::playSound(IWorld& world, const BlockPos& pos, bool isOpening) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isOpening);
}

i32 DoorBlock::getOpenSound() const {
    return m_isIron ? 1011 : 1005;
}

i32 DoorBlock::getCloseSound() const {
    return m_isIron ? 1012 : 1006;
}

size_t DoorBlock::getShapeIndex(Direction facing, bool open, bool hingeRight) {
    if (!open) {
        switch (facing) {
            case Direction::East:  return 0;
            case Direction::South: return 1;
            case Direction::West:  return 2;
            case Direction::North: return 3;
            default: return 0;
        }
    }

    if (hingeRight) {
        switch (facing) {
            case Direction::East:  return 5;
            case Direction::South: return 6;
            case Direction::West:  return 7;
            case Direction::North: return 4;
            default: return 4;
        }
    } else {
        switch (facing) {
            case Direction::East:  return 4;
            case Direction::South: return 5;
            case Direction::West:  return 6;
            case Direction::North: return 7;
            default: return 4;
        }
    }
}

} // namespace blocks
} // namespace mc
