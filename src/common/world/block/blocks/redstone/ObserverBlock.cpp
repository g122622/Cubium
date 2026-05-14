#include "ObserverBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

ObserverBlock::ObserverBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::FACING())
                         .add(BlockStateProperties::POWERED())
                         .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), id);
                         });
    createBlockState(std::move(container));

    // 设置默认状态 - MC 1.16.5 默认朝向是 South
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::South)
            .with(BlockStateProperties::POWERED(), false));
}

BlockState ObserverBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // MC 1.16.5: 侦测器朝向玩家面向方向的反方向
    // context.horizontalDirection() 是玩家面向的方向
    // 侦测器的输出方向应该是玩家面向方向的反方向
    Direction facing = Directions::opposite(context.horizontalDirection());
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

Direction ObserverBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::FACING());
}

bool ObserverBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState ObserverBlock::withPowered(BlockState state, bool powered)
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

void ObserverBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // MC 1.16.5: 放置时如果状态是激活的，需要先设置为非激活状态
    // 这通常不应该发生，因为默认状态是非激活的
    if (isPowered(state)) {
        // 如果已经有tick调度，需要先取消
        BlockState unpoweredState = withPowered(state, false);
        world.setBlockState(pos, &unpoweredState, 18);
        updateNeighborsInFront(world, pos, unpoweredState);
    }
}

void ObserverBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // MC 1.16.5: 移除时如果正在输出且有tick调度，需要通知邻居
    if (isPowered(state)) {
        updateNeighborsInFront(world, pos, state.with(BlockStateProperties::POWERED(), false));
    }
}

void ObserverBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查变化是否来自侦测面（背面）
    Direction facing = getFacing(*state);
    Direction observeDir = Directions::opposite(facing);
    BlockPos observePos = pos.offset(observeDir);

    // 只有侦测面的变化才触发
    if (neighborPos == observePos) {
        // MC 1.16.5: 如果当前未激活，调度1 tick延迟后激活
        if (!isPowered(*state)) {
            world.tickManager().scheduleBlockTick(pos, *this, DETECT_DELAY, world::tick::TickPriority::High);
        }
    }
}

BlockState ObserverBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // MC 1.16.5: 当 updatePostPlacement 被调用时，检查更新是否来自观察面
    // 注意：MC 的 facing 参数是"邻居相对于当前方块的方向"
    // 所以如果观察面被更新，facing 应该是观察方向（输出的反方向）
    Direction outputDir = getFacing(state);
    Direction observeDir = Directions::opposite(outputDir);

    // 当观察面有方块变化时触发检测
    if (facing == observeDir && !isPowered(state)) {
        // MC 1.16.5: 调度 2 tick 延迟后激活
        world.tickManager().scheduleBlockTick(currentPos, *this, DETECT_DELAY, world::tick::TickPriority::High);
    }

    return state;
}

void ObserverBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // MC 1.16.5 逻辑：
    // 1. 如果未激活 -> 激活并调度 2 tick 后熄灭
    // 2. 如果已激活 -> 熄灭
    if (isPowered(state)) {
        // 脉冲结束，停止输出
        BlockState newState = withPowered(state, false);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 激活并调度熄灭
        BlockState newState = withPowered(state, true);
        world.setBlockState(pos, &newState, 2);
        world.tickManager().scheduleBlockTick(pos, *this, PULSE_DURATION, world::tick::TickPriority::High);
    }

    // 无论激活还是熄灭，都需要通知前方的邻居更新
    updateNeighborsInFront(world, pos, isPowered(state) ? state : withPowered(state, true));
}

void ObserverBlock::updateNeighborsInFront(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // MC Java: updateNeighborsInFront
    // 通知观察面背面的方块（即侦测器指向的反方向）更新
    Direction facing = getFacing(state);
    Direction observeDir = Directions::opposite(facing);
    BlockPos observePos = pos.offset(observeDir);

    // 先通知观察面的方块
    const BlockState* observeState = world.getBlockState(observePos);
    if (observeState && !observeState->isAir()) {
        Block& observeBlock = const_cast<Block&>(observeState->getBlock());
        observeBlock.neighborChanged(world, observePos, *this, pos, false);
    }

    // 然后通知观察面周围的其他邻居（除了侦测器本身）
    // MC Java: worldIn.notifyNeighborsOfStateExcept(blockpos, this, direction);
    for (Direction dir : Directions::all()) {
        if (dir == facing) continue; // 跳过侦测器输出方向

        BlockPos neighborPos = observePos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(world, neighborPos, *this, observePos, false);
        }
    }
}

i32 ObserverBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在输出方向输出信号
    if (side != getFacing(state)) {
        return 0;
    }

    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

i32 ObserverBlock::getStrongPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    // 侦测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

} // namespace blocks
} // namespace mc
