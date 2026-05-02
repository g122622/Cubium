#include "LeavesBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../BlockRegistry.hpp"
#include "../../BlockTags.hpp"
#include "../../../../util/property/StateContainer.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/IRandom.hpp"

namespace mc {
namespace blocks {

LeavesBlock::LeavesBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::DISTANCE_1_7())
        .add(BlockStateProperties::PERSISTENT())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态：距离7，非持久
    setDefaultState(defaultState()
        .with(BlockStateProperties::DISTANCE_1_7(), 7)
        .with(BlockStateProperties::PERSISTENT(), false));
}

BlockState LeavesBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 玩家放置的树叶标记为持久
    return updateDistance(
        defaultState().with(BlockStateProperties::PERSISTENT(), true),
        context.getWorld(),
        context.placementPos()
    );
}

BlockState LeavesBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingPos);

    // 检查邻居的距离
    i32 neighborDistance = getDistance(facingState) + 1;

    // 如果距离变化，调度更新
    i32 currentDistance = state.get(BlockStateProperties::DISTANCE_1_7());
    if (neighborDistance != 1 || currentDistance != neighborDistance) {
        world.tickManager().scheduleBlockTick(currentPos, *this, 1);
    }

    return state;
}

void LeavesBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);
    const BlockState* newState = world.getBlockState(pos);
    if (newState != nullptr) {
        BlockState updated = updateDistance(*newState, world, pos);
        if (updated != *newState) {
            world.setBlockState(pos, &updated, 3);
        }
    }
}

bool LeavesBlock::ticksRandomly() const {
    // 注意：这个方法检查的是方块本身的属性，而不是状态
    // 具体的随机tick检查在randomTick中做状态检查
    return true;
}

void LeavesBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);

    // 非持久且距离为7的树叶腐烂
    if (!state.get(BlockStateProperties::PERSISTENT()) && state.get(BlockStateProperties::DISTANCE_1_7()) == 7) {
        // 移除树叶方块
        // NOTE: 物品掉落需要在方块破坏系统中统一处理
        // 未来可调用 spawnDrops(state, world, pos) 或类似方法
        world.setBlockState(pos, BlockRegistry::instance().airState(), 3);
    }
}

const CollisionShape& LeavesBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static const CollisionShape EMPTY_SHAPE;
    return EMPTY_SHAPE;
}

BlockState LeavesBlock::updateDistance(const BlockState& state, IWorld& world, const BlockPos& pos) {
    i32 minDistance = 7;

    // 检查六个方向的邻居
    static const Direction directions[] = {
        Direction::Down, Direction::Up,
        Direction::North, Direction::South,
        Direction::West, Direction::East
    };

    for (Direction dir : directions) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState != nullptr) {
            minDistance = std::min(minDistance, getDistance(*neighborState) + 1);
            if (minDistance == 1) {
                break;  // 已经找到原木，不需要继续检查
            }
        }
    }

    return state.with(BlockStateProperties::DISTANCE_1_7(), minDistance);
}

i32 LeavesBlock::getDistance(const BlockState& neighborState) {
    // 检查是否是原木（LOGS标签）
    if (BlockTags::LOGS().contains(neighborState)) {
        return 0;
    }

    // 检查是否是树叶
    const Block& block = neighborState.owner();
    if (dynamic_cast<const LeavesBlock*>(&block) != nullptr) {
        return neighborState.get(BlockStateProperties::DISTANCE_1_7());
    }

    // 其他方块返回7
    return 7;
}

} // namespace blocks
} // namespace mc
