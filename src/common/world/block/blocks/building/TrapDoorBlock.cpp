#include "TrapDoorBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

TrapDoorBlock::TrapDoorBlock(const BlockProperties& properties, bool isIron)
    : Block(properties)
    , m_isIron(isIron) {
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::OPEN())
        .add(BlockStateProperties::HALF())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::OPEN(), false)
        .with(BlockStateProperties::HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::WATERLOGGED(), false));

    constexpr f32 P = 1.0f / 16.0f;

    CollisionShape closedBottom = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 3.0f * P, 16.0f);
    CollisionShape closedTop = CollisionShape::box(0.0f, 13.0f * P, 0.0f, 16.0f, 16.0f, 16.0f);

    CollisionShape openBottomNorth = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f * P);
    CollisionShape openTopNorth = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f * P);
    CollisionShape openBottomSouth = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 16.0f, 16.0f, 16.0f);
    CollisionShape openTopSouth = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 16.0f, 16.0f, 16.0f);
    CollisionShape openBottomEast = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    CollisionShape openTopEast = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    CollisionShape openBottomWest = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 16.0f, 16.0f);
    CollisionShape openTopWest = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 16.0f, 16.0f);

    m_shapes[getShapeIndex(Direction::North, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::North, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;
    m_shapes[getShapeIndex(Direction::South, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::South, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;
    m_shapes[getShapeIndex(Direction::East, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::East, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;
    m_shapes[getShapeIndex(Direction::West, false, BlockStateProperties::DoubleBlockHalf::Lower)] = closedBottom;
    m_shapes[getShapeIndex(Direction::West, false, BlockStateProperties::DoubleBlockHalf::Upper)] = closedTop;

    m_shapes[getShapeIndex(Direction::North, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomNorth;
    m_shapes[getShapeIndex(Direction::North, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopNorth;
    m_shapes[getShapeIndex(Direction::South, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomSouth;
    m_shapes[getShapeIndex(Direction::South, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopSouth;
    m_shapes[getShapeIndex(Direction::East, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomEast;
    m_shapes[getShapeIndex(Direction::East, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopEast;
    m_shapes[getShapeIndex(Direction::West, true, BlockStateProperties::DoubleBlockHalf::Lower)] = openBottomWest;
    m_shapes[getShapeIndex(Direction::West, true, BlockStateProperties::DoubleBlockHalf::Upper)] = openTopWest;
}

BlockState TrapDoorBlock::getStateForPlacement(BlockItemUseContext& context) {
    const BlockPos pos = context.placementPos();
    const Direction clickedFace = context.getClickedFace();
    IWorld& world = const_cast<IWorld&>(context.getWorld());

    Direction facing;
    BlockStateProperties::DoubleBlockHalf half;
    if (!context.replacingClickedBlock() && Directions::isHorizontal(clickedFace)) {
        facing = clickedFace;
        half = context.getHitY() > 0.5f
            ? BlockStateProperties::DoubleBlockHalf::Upper
            : BlockStateProperties::DoubleBlockHalf::Lower;
    } else {
        facing = Directions::opposite(context.horizontalDirection());
        half = clickedFace == Direction::Up
            ? BlockStateProperties::DoubleBlockHalf::Lower
            : BlockStateProperties::DoubleBlockHalf::Upper;
    }

    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = fluidState != nullptr && fluidState->getFluid().isIn(fluid::FluidTags::WATER());
    bool powered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::OPEN(), powered)
        .with(BlockStateProperties::HALF(), half)
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool TrapDoorBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    // 参考: net.minecraft.block.TrapDoorBlock
    // 活板门没有特殊的放置位置检查
    // 如果支撑丢失，由 updatePostPlacement 处理移除
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return true;
}

BlockState TrapDoorBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    // 参考: net.minecraft.block.TrapDoorBlock#updatePostPlacement
    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        MC_ASSERT(waterFluid != nullptr);
        world.scheduleFluidTick(currentPos, *waterFluid, waterFluid->getTickDelay(world));
    }

    // 调用父类处理
    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

void TrapDoorBlock::neighborChanged(IWorld& world, const BlockPos& pos,
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
    bool wasPowered = state.get(BlockStateProperties::POWERED());
    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);

    if (isPowered == wasPowered) {
        return;
    }

    bool wasOpen = state.get(BlockStateProperties::OPEN());
    BlockState newState = state
        .with(BlockStateProperties::POWERED(), isPowered)
        .with(BlockStateProperties::OPEN(), isPowered);
    world.setBlockState(pos, &newState, 2);

    if (newState.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        MC_ASSERT(waterFluid != nullptr);
        world.scheduleFluidTick(pos, *waterFluid, waterFluid->getTickDelay(world));
    }

    if (wasOpen != isPowered) {
        playSound(world, pos, isPowered);
    }
}

ActionResultType TrapDoorBlock::onBlockActivated(
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

    toggle(world, pos, state, !state.get(BlockStateProperties::OPEN()));
    return ActionResultType::Success;
}

const CollisionShape& TrapDoorBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    bool open = state.get(BlockStateProperties::OPEN());
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::HALF());

    size_t index = getShapeIndex(facing, open, half);
    MC_ASSERT(index < 16);
    return m_shapes[index];
}

const CollisionShape& TrapDoorBlock::getCollisionShape(const BlockState& state) const {
    if (state.get(BlockStateProperties::OPEN())) {
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }
    return getShape(state);
}

const BlockState& TrapDoorBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& TrapDoorBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

bool TrapDoorBlock::isOpen(const BlockState& state) {
    return state.get(BlockStateProperties::OPEN());
}

void TrapDoorBlock::toggle(IWorld& world, const BlockPos& pos, const BlockState& state, bool open) {
    if (state.get(BlockStateProperties::OPEN()) == open) {
        return;
    }

    BlockState newState = state.with(BlockStateProperties::OPEN(), open);
    world.setBlockState(pos, &newState, 10);

    if (newState.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        MC_ASSERT(waterFluid != nullptr);
        world.scheduleFluidTick(pos, *waterFluid, waterFluid->getTickDelay(world));
    }

    playSound(world, pos, open);
}

void TrapDoorBlock::playSound(IWorld& world, const BlockPos& pos, bool isOpening) {
    const BlockState* state = world.getBlockState(pos);
    const auto* trapDoor = state != nullptr ? dynamic_cast<const TrapDoorBlock*>(&state->getBlock()) : nullptr;
    const bool isIron = trapDoor != nullptr && trapDoor->isIronTrapdoor();

    const char* soundId = nullptr;
    if (isIron) {
        soundId = isOpening ? "minecraft:block.iron_trapdoor.open" : "minecraft:block.iron_trapdoor.close";
    } else {
        soundId = isOpening ? "minecraft:block.wooden_trapdoor.open" : "minecraft:block.wooden_trapdoor.close";
    }

    world.playSound(ResourceLocation(soundId), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

size_t TrapDoorBlock::getShapeIndex(Direction facing, bool open, BlockStateProperties::DoubleBlockHalf half) {
    size_t facingIdx = 0;
    switch (facing) {
        case Direction::North: facingIdx = 0; break;
        case Direction::South: facingIdx = 1; break;
        case Direction::East:  facingIdx = 2; break;
        case Direction::West:  facingIdx = 3; break;
        default: facingIdx = 0; break;
    }

    size_t halfIdx = (half == BlockStateProperties::DoubleBlockHalf::Upper) ? 1 : 0;
    size_t openIdx = open ? 2 : 0;
    return facingIdx * 4 + openIdx + halfIdx;
}

} // namespace blocks
} // namespace mc
