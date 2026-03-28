#include "PistonBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

PistonBlock::PistonBlock(const BlockProperties& properties, bool sticky)
    : Block(properties)
    , m_sticky(sticky) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::EXTENDED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::EXTENDED(), false));
}

bool PistonBlock::isExtended(const BlockState& state) {
    return state.get(BlockStateProperties::EXTENDED());
}

BlockState PistonBlock::withExtended(BlockState state, bool extended) {
    return state.with(BlockStateProperties::EXTENDED(), extended);
}

Direction PistonBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

void PistonBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 活塞放置时不立即触发
}

void PistonBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                   const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state) {
        return;
    }

    // 检查是否应该改变状态
    bool shouldExtend = shouldBeExtended(world, pos, *state);
    bool isCurrentlyExtended = isExtended(*state);

    if (shouldExtend != isCurrentlyExtended) {
        if (shouldExtend) {
            extend(world, pos, *state);
        } else {
            retract(world, pos, *state);
        }
    }
}

BlockState PistonBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    return state;
}

void PistonBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 活塞动画由 PistonEntity 处理
}

bool PistonBlock::shouldBeExtended(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    Direction facing = getFacing(state);

    // MC Java 逻辑：检查除了朝向以外的所有方向是否被侧面充能
    // 注意：不检查活塞朝向方向的信号！

    // 检查活塞本体除前面外5个方向是否被充能
    for (Direction dir : Directions::all()) {
        if (dir == facing) {
            // 不检查活塞朝向方向（前面）
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);
        // MC Java: worldIn.isSidePowered(pos.offset(direction), direction)
        // 检查相邻方块在该方向是否被充能（从该方向接收强信号）
        if (world::redstone::RedstonePower::isSidePowered(world, neighborPos, dir)) {
            return true;
        }
    }

    return false;
}

bool PistonBlock::extend(IWorld& world, const BlockPos& pos, const BlockState& state) {
    Direction facing = getFacing(state);

    // 计算推动链
    std::vector<BlockPos> blocksToPush;
    if (!calculatePushChain(world, pos, facing, blocksToPush)) {
        return false;
    }

    // 如果有方块要推动
    if (!blocksToPush.empty()) {
        // 从最远端开始移动
        for (auto it = blocksToPush.rbegin(); it != blocksToPush.rend(); ++it) {
            const BlockPos& blockPos = *it;
            const BlockState* blockState = world.getBlockState(blockPos.x, blockPos.y, blockPos.z);
            if (!blockState) continue;

            BlockPos newPos = blockPos.offset(facing);

            // 移动方块
            world.setBlockState(newPos.x, newPos.y, newPos.z, blockState, 2);
            world.setBlockState(blockPos.x, blockPos.y, blockPos.z, nullptr, 2);
        }
    }

    // 更新活塞状态为伸出
    BlockState newState = withExtended(state, true);
    world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

    // TODO: 创建活塞头方块和活塞实体

    return true;
}

bool PistonBlock::retract(IWorld& world, const BlockPos& pos, const BlockState& state) {
    Direction facing = getFacing(state);

    // 更新活塞状态为收回
    BlockState newState = withExtended(state, false);
    world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

    if (m_sticky) {
        // 粘性活塞：尝试拉回方块
        BlockPos frontPos = pos.offset(facing);
        BlockPos pullPos = frontPos.offset(facing);  // 活塞头前面的方块

        const BlockState* pullState = world.getBlockState(pullPos.x, pullPos.y, pullPos.z);
        if (pullState && !pullState->isAir()) {
            // 检查方块是否可以被拉回
            Material::PushReaction reaction = getBlockPushReaction(*pullState);
            if (reaction == Material::PushReaction::Normal) {
                // 移动方块到活塞头位置
                world.setBlockState(frontPos.x, frontPos.y, frontPos.z, pullState, 2);
                world.setBlockState(pullPos.x, pullPos.y, pullPos.z, nullptr, 2);
            }
        }
    }

    // TODO: 移除活塞头方块和活塞实体

    return true;
}

bool PistonBlock::calculatePushChain(
    IWorld& world,
    const BlockPos& pos,
    Direction facing,
    std::vector<BlockPos>& blocks) const {

    BlockPos currentPos = pos.offset(facing);  // 从活塞头位置开始

    for (i32 i = 0; i < MAX_PUSH_DISTANCE; ++i) {
        const BlockState* state = world.getBlockState(currentPos.x, currentPos.y, currentPos.z);

        if (!state || state->isAir()) {
            // 空气，可以推动
            break;
        }

        Material::PushReaction reaction = getBlockPushReaction(*state);

        if (reaction == Material::PushReaction::Block) {
            // 不能推动的方块
            return false;
        }

        if (reaction == Material::PushReaction::Destroy) {
            // 会被破坏的方块
            // TODO: 掉落物品
            break;
        }

        if (reaction == Material::PushReaction::Normal) {
            // 可以推动的方块
            blocks.push_back(currentPos);
        }

        currentPos = currentPos.offset(facing);
    }

    return true;
}

Material::PushReaction PistonBlock::getBlockPushReaction(const BlockState& state) const {
    const Block& block = state.getBlock();
    return block.getPushReaction(state);
}

} // namespace blocks
} // namespace mc
