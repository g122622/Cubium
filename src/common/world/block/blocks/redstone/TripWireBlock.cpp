#include "TripWireBlock.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/AxisAlignedBB.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../VanillaBlocks.hpp"
#include "TripWireHookBlock.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TripWireBlock::TripWireBlock(const BlockProperties& properties)
    : Block(properties)
{

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

bool TripWireBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

bool TripWireBlock::isConnected(const BlockState& state, Direction direction)
{
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

bool TripWireBlock::isActivated(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

bool TripWireBlock::shouldConnectTo(const BlockState& neighborState, Direction direction) const
{
    // MC 1.16.5: TripWireBlock.shouldConnectTo
    const Block& neighborBlock = neighborState.getBlock();

    // 检查相邻方块是否是绊线钩
    if (&neighborBlock == VanillaBlocks::TRIPWIRE_HOOK) {
        // 绊线钩必须面向绊线才能连接
        // 即钩的 FACING 必须与当前检测方向相反
        Direction hookFacing = TripWireHookBlock::getFacing(neighborState);
        return hookFacing == Directions::opposite(direction);
    }

    // 检查相邻方块是否是绊线
    if (&neighborBlock == VanillaBlocks::TRIPWIRE) {
        return true;
    }

    // 其他情况不连接
    return false;
}

void TripWireBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 放置时不触发
}

void TripWireBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
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
        // 没有支撑，掉落绊线物品
        // 参考 MC 1.16.5: TripWireBlock.neighborChanged
        const Block* block = &state->getBlock();
        if (block != nullptr) {
            const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
            if (blockItem != nullptr) {
                ItemStack dropStack(blockItem, 1);
                math::Random rng;
                ItemDropHelper::spawnItemEntity(&world,
                    dropStack,
                    static_cast<f64>(pos.x) + 0.5,
                    static_cast<f64>(pos.y) + 0.5,
                    static_cast<f64>(pos.z) + 0.5,
                    rng);
            }
        }
        world.setBlockState(pos, nullptr, 3);
    }
}

void TripWireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 更新绊线状态
    updateState(world, pos);
}

void TripWireBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 移除时通知绊线钩
    notifyHooks(world, pos);
}

BlockState TripWireBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // MC 1.16.5: 只处理水平方向的更新
    if (!Directions::isHorizontal(facing)) {
        return state;
    }

    // 检查是否应该连接到相邻方块
    bool shouldConnect = shouldConnectTo(facingState, facing);

    // 根据方向设置对应的连接属性
    switch (facing) {
        case Direction::North:
            return state.with(BlockStateProperties::NORTH(), shouldConnect);
        case Direction::East:
            return state.with(BlockStateProperties::EAST(), shouldConnect);
        case Direction::South:
            return state.with(BlockStateProperties::SOUTH(), shouldConnect);
        case Direction::West:
            return state.with(BlockStateProperties::WEST(), shouldConnect);
        default:
            return state;
    }
}

i32 TripWireBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return isPowered(state) ? 15 : 0;
}

i32 TripWireBlock::getStrongPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return isPowered(state) ? 15 : 0;
}

void TripWireBlock::updateState(IWorld& world, const BlockPos& pos)
{
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

bool TripWireBlock::checkEntityCollision(IWorld& world, const BlockPos& pos) const
{
    // 创建绊线的碰撞箱
    // 绊线是一个细线，检测范围为方块内的一小片区域
    AxisAlignedBB detectionBox(static_cast<f32>(pos.x) + 0.0f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.0f,
        static_cast<f32>(pos.x) + 1.0f,
        static_cast<f32>(pos.y) + 0.5f, // 检测向上0.5格
        static_cast<f32>(pos.z) + 1.0f);

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 绊线被任何实体触发（玩家、生物、物品等）
    // 参考 MC 1.16.5: 潜行的玩家不会触发绊线
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            // 潜行的玩家不会触发绊线
            if (entity->isSneaking()) {
                continue;
            }
            return true;
        }
    }

    return false;
}

void TripWireBlock::notifyHooks(IWorld& world, const BlockPos& pos)
{
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
