#include "RedstoneTorchBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"

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
    // 检查下方方块是否被充能
    BlockPos belowPos = pos.down();
    return world::redstone::RedstonePower::isSidePowered(world, belowPos, Direction::Up);
}

bool RedstoneTorchBlock::isLit(const BlockState& state) {
    return state.get(BlockStateProperties::LIT());
}

void RedstoneTorchBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查初始状态是否正确
    bool shouldBeLit = !shouldBeOff(world, pos);
    if (isLit(state) != shouldBeLit) {
        // 需要更新状态
        world.scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::ExtremelyHigh);
    }
}

void RedstoneTorchBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                          const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 更新火把状态
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state) {
        updateState(world, pos, *state);
    }
}

void RedstoneTorchBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 检查当前应该的状态
    bool shouldBeLit = !shouldBeOff(world, pos);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 检查烧毁
        if (checkForBurnout(world, pos, world.currentTick())) {
            // 烧毁，暂时不改变状态，调度冷却
            world.scheduleBlockTick(pos, *this, BURNOUT_COOLDOWN, world::tick::TickPriority::ExtremelyHigh);
            return;
        }

        // 改变状态
        BlockState newState = state.with(BlockStateProperties::LIT(), shouldBeLit);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);

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

    // 点亮时输出强度15
    return world::redstone::RedstonePower::MAX_POWER;
}

void RedstoneTorchBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 检查是否应该改变状态
    bool shouldBeLit = !shouldBeOff(world, pos);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 调度更新（延迟1 tick）
        world.scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::ExtremelyHigh);
    }
}

bool RedstoneTorchBlock::checkForBurnout(IWorld& world, const BlockPos& pos, u64 currentTick) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(currentTick);
    // TODO: 实现烧毁检测逻辑
    // 需要在世界中记录火把的翻转历史
    // 当前简化实现，不进行烧毁检测
    return false;
}

} // namespace blocks
} // namespace mc
