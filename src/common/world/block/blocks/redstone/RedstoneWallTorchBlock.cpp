#include "RedstoneWallTorchBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

RedstoneWallTorchBlock::RedstoneWallTorchBlock(const BlockProperties& properties)
    : RedstoneTorchBlock(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::LIT())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::LIT(), true));
}

Direction RedstoneWallTorchBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState RedstoneWallTorchBlock::withFacing(BlockState state, Direction facing) {
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

bool RedstoneWallTorchBlock::shouldBeOff(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    // MC Java: Direction direction = state.get(FACING).getOpposite();
    // return worldIn.isSidePowered(pos.offset(direction), direction);
    Direction facing = getFacing(state);
    Direction attachDir = Directions::opposite(facing);  // 附着面方向
    BlockPos attachPos = pos.offset(attachDir);
    return world::redstone::RedstonePower::isSidePowered(world, attachPos, attachDir);
}

bool RedstoneWallTorchBlock::canPlaceAt(IWorld& world, const BlockPos& pos, Direction facing) const {
    // 检查附着面是否可以支撑火把
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos.x, attachPos.y, attachPos.z);
    if (!attachState || attachState->isAir()) {
        return false;
    }

    // 检查附着面是否是固体面
    return attachState->getBlock().isSolidSide(*attachState, world, attachPos, facing);
}

void RedstoneWallTorchBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // MC Java: 放置时通知邻居
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 检查初始状态是否正确
    bool shouldBeLit = !shouldBeOff(world, pos, state);
    if (isLit(state) != shouldBeLit) {
        world.scheduleBlockTick(pos, *this, 2, world::tick::TickPriority::ExtremelyHigh);
    }
}

void RedstoneWallTorchBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                             const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return;
    }

    // 检查支撑是否还在
    Direction facing = getFacing(*state);
    if (!canPlaceAt(world, pos, facing)) {
        // 支撑丢失，火把掉落
        world.setBlockState(pos.x, pos.y, pos.z, nullptr, 2);
        return;
    }

    // 更新火把状态
    updateState(world, pos, *state);
}

BlockState RedstoneWallTorchBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查支撑是否有效
    Direction torchFacing = getFacing(state);
    if (facing == Directions::opposite(torchFacing)) {
        // 支撑面被更新
        if (!facingState.isAir() && facingState.getBlock().isSolidSide(facingState, world, facingPos, torchFacing)) {
            return state;
        }
        // 支撑丢失，移除火把
        return state.with(BlockStateProperties::LIT(), false);
    }

    return state;
}

BlockState RedstoneWallTorchBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 尝试击中的面
    Direction hitFace = context.face();
    if (Directions::isHorizontal(hitFace)) {
        // 检查是否可以附着
        BlockPos pos = context.placementPos();
        BlockPos attachPos = pos.offset(hitFace);
        const IWorld& world = context.getWorld();
        const BlockState* attachState = world.getBlockState(attachPos.x, attachPos.y, attachPos.z);
        if (attachState && attachState->getBlock().isSolidSide(*attachState, const_cast<IWorld&>(world), attachPos, Directions::opposite(hitFace))) {
            return defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(hitFace))
                .with(BlockStateProperties::LIT(), true);
        }
    }

    // 尝试其他水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::West, Direction::East}) {
        BlockPos pos = context.placementPos();
        BlockPos attachPos = pos.offset(dir);
        const IWorld& world = context.getWorld();
        const BlockState* attachState = world.getBlockState(attachPos.x, attachPos.y, attachPos.z);
        if (attachState && attachState->getBlock().isSolidSide(*attachState, const_cast<IWorld&>(world), attachPos, Directions::opposite(dir))) {
            return defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(dir))
                .with(BlockStateProperties::LIT(), true);
        }
    }

    return defaultState();
}

i32 RedstoneWallTorchBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 熄灭时不输出信号
    if (!isLit(state)) {
        return 0;
    }

    // MC Java: return blockState.get(REDSTONE_TORCH_LIT) && blockState.get(FACING) != side ? 15 : 0;
    // 不向附着面方向输出信号
    Direction facing = getFacing(state);
    if (side == facing) {
        return 0;
    }

    // 检查是否已烧毁
    if (world::redstone::RedstoneSystem::instance().isTorchBurnedOut(pos, world.currentTick())) {
        return 0;
    }

    // 点亮时输出强度15
    return world::redstone::RedstonePower::MAX_POWER;
}

void RedstoneWallTorchBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查是否应该改变状态
    bool shouldBeLit = !shouldBeOff(world, pos, state);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 调度更新（MC Java 使用 2 tick 延迟）
        world.scheduleBlockTick(pos, *this, 2, world::tick::TickPriority::ExtremelyHigh);
    }
}

} // namespace blocks
} // namespace mc
