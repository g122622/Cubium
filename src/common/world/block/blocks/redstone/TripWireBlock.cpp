#include "TripWireBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../util/AxisAlignedBB.hpp"
#include "../../../../util/property/Properties.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TripWireBlock::TripWireBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::ATTACHED())
        .add(BlockStateProperties::DISARMED())
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::WEST())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::ATTACHED(), false)
        .with(BlockStateProperties::DISARMED(), false)
        .with(BlockStateProperties::NORTH(), false)
        .with(BlockStateProperties::EAST(), false)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::WEST(), false));
}

bool TripWireBlock::isPowered(const BlockState& state) {
    return state.get(BlockStateProperties::POWERED());
}

bool TripWireBlock::isConnected(const BlockState& state, Direction direction) {
    switch (direction) {
        case Direction::North:
            return state.get(BlockStateProperties::NORTH());
        case Direction::East:
            return state.get(BlockStateProperties::EAST());
        case Direction::South:
            return state.get(BlockStateProperties::SOUTH());
        case Direction::West:
            return state.get(BlockStateProperties::WEST());
        default:
            return false;
    }
}

bool TripWireBlock::isActivated(const BlockState& state) {
    return state.get(BlockStateProperties::POWERED());
}

void TripWireBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 放置时不触发
}

void TripWireBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                     const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查支撑方块
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!belowState || !belowState->isSolid()) {
        // 没有支撑，掉落
        // TODO: 掉落线物品
        world.setBlockState(pos, nullptr, 3);
    }
}

void TripWireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);
    // 更新绊线状态
    updateState(world, pos);
}

void TripWireBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 移除时通知绊线钩
    notifyHooks(world, pos);
}

BlockState TripWireBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 更新连接状态
    // TODO: 检查相邻是否是绊线或绊线钩

    return state;
}

i32 TripWireBlock::getWeakPower(const BlockState& state, IWorld& world,
                                 const BlockPos& pos, Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return isPowered(state) ? 15 : 0;
}

i32 TripWireBlock::getStrongPower(const BlockState& state, IWorld& world,
                                   const BlockPos& pos, Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return isPowered(state) ? 15 : 0;
}

void TripWireBlock::updateState(IWorld& world, const BlockPos& pos) {
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检测实体碰撞
    bool hasEntity = checkEntityCollision(world, pos);
    bool isCurrentlyPowered = isPowered(*state);

    if (hasEntity != isCurrentlyPowered) {
        BlockState newState = *state;
        newState = newState.with(BlockStateProperties::POWERED(), hasEntity);
        world.setBlockState(pos, &newState, 3);

        // 通知绊线钩
        notifyHooks(world, pos);
    }
}

bool TripWireBlock::checkEntityCollision(IWorld& world, const BlockPos& pos) const {
    // 创建绊线的碰撞箱
    // 绊线是一个细线，检测范围为方块内的一小片区域
    AxisAlignedBB detectionBox(
        static_cast<f32>(pos.x) + 0.0f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.0f,
        static_cast<f32>(pos.x) + 1.0f,
        static_cast<f32>(pos.y) + 0.5f,  // 检测向上0.5格
        static_cast<f32>(pos.z) + 1.0f
    );

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 绊线被任何实体触发（玩家、生物、物品等）
    // 注意：潜行的玩家不会触发绊线，但需要实体系统支持 isSneaking()
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            // TODO: 检查玩家是否在潜行 (entity->isSneaking())
            // 目前所有实体都会触发
            return true;
        }
    }

    return false;
}

void TripWireBlock::notifyHooks(IWorld& world, const BlockPos& pos) {
    // 通知四个方向的绊线钩
    for (Direction dir : {Direction::North, Direction::East, Direction::South, Direction::West}) {
        BlockPos hookPos = pos.offset(dir);
        const BlockState* hookState = world.getBlockState(hookPos);
        if (hookState) {
            Block* hookBlock = const_cast<Block*>(&hookState->getBlock());
            if (hookBlock) {
                hookBlock->neighborChanged(world, hookPos, *this, pos, false);
            }
        }
    }
}

} // namespace blocks
} // namespace mc
