#include "RedstoneTorchBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

RedstoneTorchBlock::RedstoneTorchBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::LIT())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::LIT(), true));
}

bool RedstoneTorchBlock::shouldBeOff(IWorld& world, const BlockPos& pos) const {
    // MC Java: worldIn.isSidePowered(pos.down(), Direction.DOWN)
    // 检查火把附着方块（下方）是否从下方方向接收到强信号
    // 即：检查附着方块是否有来自其下方的强信号输入
    BlockPos belowPos = pos.down();
    return world::redstone::RedstonePower::isSidePowered(world, belowPos, Direction::Down);
}

bool RedstoneTorchBlock::isLit(const BlockState& state) {
    return state.get(BlockStateProperties::LIT());
}

void RedstoneTorchBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // MC Java: 放置时通知六个方向的邻居
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 检查初始状态是否正确
    bool shouldBeLit = !shouldBeOff(world, pos);
    if (isLit(state) != shouldBeLit) {
        // 需要更新状态（MC Java 使用 2 tick 延迟）
        world.tickManager().scheduleBlockTick(pos, *this, 2, world::tick::TickPriority::ExtremelyHigh);
    }
}

void RedstoneTorchBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                          const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 更新火把状态
    const BlockState* state = world.getBlockState(pos);
    if (state) {
        updateState(world, pos, *state);
    }
}

void RedstoneTorchBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 检查当前应该的状态
    bool shouldBeLit = !shouldBeOff(world, pos);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 记录翻转并检查烧毁
        if (world::redstone::RedstoneSystem::instance().checkAndRecordTorchFlip(pos, world.currentTick())) {
            // 烧毁！保持当前状态，调度下一次检查
            world.tickManager().scheduleBlockTick(pos, *this, world::redstone::RedstoneSystem::BURNOUT_COOLDOWN,
                                   world::tick::TickPriority::ExtremelyHigh);
            return;
        }

        // 改变状态
        BlockState newState = state.with(BlockStateProperties::LIT(), shouldBeLit);
        world.setBlockState(pos, &newState, 3);

        // 更新相邻方块
        world::redstone::RedstoneSystem::instance().updateNeighborsExcept(
            world, pos, *this, Direction::Down);
    }
}

i32 RedstoneTorchBlock::getWeakPower(
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

    // 不向下输出信号
    if (side == Direction::Down) {
        return 0;
    }

    // 检查是否已烧毁
    if (world::redstone::RedstoneSystem::instance().isTorchBurnedOut(pos, world.currentTick())) {
        return 0;
    }

    // 点亮时输出强度15
    return world::redstone::RedstonePower::MAX_POWER;
}

i32 RedstoneTorchBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // MC Java: 只在向下方向输出强信号（充能下方方块）
    return side == Direction::Down ? getWeakPower(state, world, pos, side) : 0;
}

const CollisionShape& RedstoneTorchBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static const CollisionShape torchShape = CollisionShape::fromPixelBox(7.0f, 0.0f, 7.0f, 9.0f, 10.0f, 9.0f);
    return torchShape;
}

void RedstoneTorchBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查是否应该改变状态
    bool shouldBeLit = !shouldBeOff(world, pos);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 调度更新（MC Java 使用 2 tick 延迟）
        world.tickManager().scheduleBlockTick(pos, *this, 2, world::tick::TickPriority::ExtremelyHigh);
    }
}

} // namespace blocks
} // namespace mc
