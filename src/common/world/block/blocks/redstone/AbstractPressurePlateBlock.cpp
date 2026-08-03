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

#include "AbstractPressurePlateBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

AbstractPressurePlateBlock::AbstractPressurePlateBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::POWERED(), false));
}

bool AbstractPressurePlateBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState AbstractPressurePlateBlock::withPowered(BlockState state, bool powered)
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

i32 AbstractPressurePlateBlock::getStoredSignal(const BlockState& state) const
{
    // 木/石压力板持久化 powered 布尔：按下=15、松开=0，归一为 0/15 供 tick 判变化。
    return isPowered(state) ? 15 : 0;
}

BlockState AbstractPressurePlateBlock::withStoredSignal(BlockState state, i32 signal) const
{
    // 木/石压力板只持久化布尔：有信号=true，无信号=false。
    return withPowered(std::move(state), signal > 0);
}

void AbstractPressurePlateBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 压力板放置时不触发信号
}

void AbstractPressurePlateBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 与 MC 1.21.11 BasePressurePlateBlock.canSurvive 一致：
    //   canSupportRigidBlock(world, pos.below()) || canSupportCenter(world, pos.below(), Direction.UP)
    // 下方支撑失效时移除压力板
    if (!_canSurvive(world, pos)) {
        world.setBlockState(pos, nullptr, 2);
    }
}

bool AbstractPressurePlateBlock::_canSurvive(IWorld& world, const BlockPos& pos) const
{
    // 与 MC 1.21.11 BasePressurePlateBlock.canSurvive 一致：
    //   canSupportRigidBlock(world, pos.below()) || canSupportCenter(world, pos.below(), Direction.UP)
    const BlockPos belowPos = pos.down();
    return Block::canSupportRigidBlock(world, belowPos) || Block::canSupportCenter(world, belowPos, Direction::Up);
}

void AbstractPressurePlateBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    const i32 oldSignal = getStoredSignal(state);
    const i32 newSignal = calculateSignalStrength(world, pos);

    if (newSignal != oldSignal) {
        // 信号变化，写入持久化状态
        BlockState newState = withStoredSignal(state, newSignal);
        world.setBlockState(pos, &newState, 2);

        // 播放音效（按下/弹起切换时）
        playClickSound(world, pos, newSignal > 0);

        // 通知相邻方块更新
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState && !neighborState->isAir()) {
                Block& neighborBlock = neighborState->getBlockMutable();
                neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
            }
        }
    } else if (newSignal > 0) {
        // 仍然有压力，继续检测
        world.tickManager().scheduleBlockTick(
            pos, *this, getTickDelay(oldSignal > 0, newSignal > 0), world::tick::TickPriority::High);
    }
}

i32 AbstractPressurePlateBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(side);

    // 压力板向所有方向输出信号；信号强度实时计算（不持久化到 block state）
    // 仅在按下（持久化信号>0）时输出，未按下返回 0
    if (getStoredSignal(state) <= 0) {
        return 0;
    }
    return calculateSignalStrength(world, pos);
}

i32 AbstractPressurePlateBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 压力板向上方输出强信号
    if (side == Direction::Up) {
        return getWeakPower(state, world, pos, side);
    }
    return 0;
}

const CollisionShape& AbstractPressurePlateBlock::getShape(const BlockState& state) const
{
    static const CollisionShape unpressedShape = CollisionShape::fromPixelBox(1.0f, 0.0f, 1.0f, 15.0f, 1.0f, 15.0f);
    static const CollisionShape pressedShape = CollisionShape::fromPixelBox(1.0f, 0.0f, 1.0f, 15.0f, 0.5f, 15.0f);
    return getStoredSignal(state) > 0 ? pressedShape : unpressedShape;
}

bool AbstractPressurePlateBlock::hasEntityOnPlate(IWorld& world, const BlockPos& pos) const
{
    // 创建压力板上方的碰撞箱
    // 压力板检测范围为方块上方的一个薄层
    AxisAlignedBB detectionBox(static_cast<f32>(pos.x) + 0.125f, // 略微收缩水平范围
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f, // 检测向上0.25格
        static_cast<f32>(pos.z) + 0.875f);

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 过滤：只检测可以触发压力板的实体
    for (Entity* entity : entities) {
        if (entity != nullptr && !entity->doesEntityNotTriggerPressurePlate()) {
            return true;
        }
    }

    return false;
}

void AbstractPressurePlateBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 当实体踩上压力板时，如果当前未被触发，则调度tick更新状态
    MC_UNUSED(entity);
    if (getStoredSignal(state) <= 0) {
        // 调度tick来更新状态
        // 需要const_cast因为scheduleBlockTick需要非const的Block引用
        world.tickManager().scheduleBlockTick(
            pos, const_cast<AbstractPressurePlateBlock&>(*this), 0, world::tick::TickPriority::High);
    }
}

void AbstractPressurePlateBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    const i32 oldSignal = getStoredSignal(state);
    const i32 newSignal = calculateSignalStrength(world, pos);

    if (newSignal != oldSignal) {
        // 信号变化，调度tick
        world.tickManager().scheduleBlockTick(
            pos, *this, getTickDelay(oldSignal > 0, newSignal > 0), world::tick::TickPriority::High);
    }
}

} // namespace blocks
} // namespace mc
