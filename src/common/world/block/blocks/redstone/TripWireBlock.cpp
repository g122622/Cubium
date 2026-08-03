/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

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
#include "TripWireHookBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TripWireBlock::TripWireBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ATTACHED())
            .add(BlockStateProperties::DISARMED())
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::WEST())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
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
    const Block& neighborBlock = neighborState.getBlock();

    // 检查相邻方块是否是绊线钩
    if (&neighborBlock == VanillaBlocks::TRIPWIRE_HOOK) {
        // 绊线钩必须面向绊线才能连接
        Direction hookFacing = TripWireHookBlock::getFacing(neighborState);
        return hookFacing == Directions::opposite(direction);
    }

    // 检查相邻方块是否是绊线
    if (&neighborBlock == VanillaBlocks::TRIPWIRE) {
        return true;
    }

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
    MC_UNUSED(state);
    // 更新绊线状态
    updateState(world, pos);
}

void TripWireBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 移除时通知绊线钩
    _notifyHooks(world, pos);
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

    // 只处理水平方向的更新
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

i32 TripWireBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return isPowered(state) ? 15 : 0;
}

i32 TripWireBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 绊线只输出弱信号，不输出强信号
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);
    MC_UNUSED(state);
    return 0;
}

const CollisionShape& TripWireBlock::getShape(const BlockState& state) const
{
    // ATTACHED=true: AABB = (0, 1, 0) -> (16, 2.5, 16) - 绷紧的绊线
    // ATTACHED=false: TRIP_WRITE_ATTACHED_AABB = (0, 0, 0) -> (16, 8, 16) - 松弛的绊线
    static const CollisionShape attachedShape = CollisionShape::box(0.0f, 1.0f / 16.0f, 0.0f, 1.0f, 2.5f / 16.0f, 1.0f);
    static const CollisionShape detachedShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 8.0f / 16.0f, 1.0f);

    return state.get(BlockStateProperties::ATTACHED()) ? attachedShape : detachedShape;
}

void TripWireBlock::updateState(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检测实体碰撞
    bool hasEntity = _checkEntityCollision(world, pos);
    bool isCurrentlyPowered = isPowered(*state);

    if (hasEntity != isCurrentlyPowered) {
        BlockState newState = *state;
        newState = newState.with(BlockStateProperties::POWERED(), hasEntity);
        world.setBlockState(pos, &newState, 3);

        // 通知绊线钩
        _notifyHooks(world, pos);
    }
}

bool TripWireBlock::_checkEntityCollision(IWorld& world, const BlockPos& pos) const
{
    // 创建绊线的碰撞箱，使用方块的 shape 来获取碰撞箱
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    // 根据ATTACHED状态获取对应的碰撞箱
    // ATTACHED=true: (0, 1, 0) -> (16, 2.5, 16)
    // ATTACHED=false: (0, 0, 0) -> (16, 8, 16)
    AxisAlignedBB detectionBox;
    if (state->get(BlockStateProperties::ATTACHED())) {
        // 绷紧状态: Y范围 1/16 到 2.5/16
        detectionBox = AxisAlignedBB(static_cast<f32>(pos.x),
            static_cast<f32>(pos.y) + 1.0f / 16.0f,
            static_cast<f32>(pos.z),
            static_cast<f32>(pos.x) + 1.0f,
            static_cast<f32>(pos.y) + 2.5f / 16.0f,
            static_cast<f32>(pos.z) + 1.0f);
    } else {
        // 松弛状态: Y范围 0 到 8/16
        detectionBox = AxisAlignedBB(static_cast<f32>(pos.x),
            static_cast<f32>(pos.y),
            static_cast<f32>(pos.z),
            static_cast<f32>(pos.x) + 1.0f,
            static_cast<f32>(pos.y) + 8.0f / 16.0f,
            static_cast<f32>(pos.z) + 1.0f);
    }

    // 查询碰撞箱内的实体，检查实体是否触发绊线
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    for (Entity* entity : entities) {
        if (entity != nullptr && !entity->doesEntityNotTriggerPressurePlate()) {
            return true;
        }
    }

    return false;
}

void TripWireBlock::_notifyHooks(IWorld& world, const BlockPos& pos)
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
