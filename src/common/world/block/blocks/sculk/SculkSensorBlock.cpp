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

#include "common/world/block/blocks/sculk/SculkSensorBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "sound/SoundCategory.hpp"
#include "sound/SoundEvents.hpp"
#include "util/Direction.hpp"
#include "world/IWorld.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/gameevent/GameEvents.hpp"
#include "world/gameevent/VibrationSystem.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ============================================================================
// SculkSensorBlock
// ============================================================================

SculkSensorBlock::SculkSensorBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SCULK_SENSOR_PHASE())
            .add(BlockStateProperties::POWER_0_15())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive)
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false));

    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 8, 16);
}

void SculkSensorBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 属性已在构造函数中通过 Builder 添加
    MC_UNUSED(container);
}

BlockState SculkSensorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state =
        defaultState()
            .with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive)
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState SculkSensorBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

i32 SculkSensorBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);
    return state.get(BlockStateProperties::POWER_0_15());
}

i32 SculkSensorBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    // 比较器输出的是振动频率（1-15），而非红石信号强度（基于距离）
    // 非 Active 状态时输出 0
    if (getPhase(state) != BlockStateProperties::SculkSensorPhase::Active) {
        return 0;
    }
    // 从方块实体获取最后振动频率
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::SculkSensor) {
        auto* sensorEntity = static_cast<blockentity::SculkSensorBlockEntity*>(entity);
        return sensorEntity->getLastVibrationFrequency();
    }
    return 0;
}

const CollisionShape& SculkSensorBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const BlockState& SculkSensorBlock::rotate(const BlockState& state, Rotation rotation) const
{
    MC_UNUSED(rotation);
    return state;
}

const BlockState& SculkSensorBlock::mirror(const BlockState& state, Mirror mirror) const
{
    MC_UNUSED(mirror);
    return state;
}

const fluid::FluidState* SculkSensorBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

std::unique_ptr<BlockEntity> SculkSensorBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SculkSensorBlockEntity>(pos);
}

BlockEntityType SculkSensorBlock::getBlockEntityType() const
{
    return BlockEntityType::SculkSensor;
}

// ========== 静态方法：激活/失活/状态查询 ==========

bool SculkSensorBlock::canActivate(const BlockState& state)
{
    return state.get(BlockStateProperties::SCULK_SENSOR_PHASE()) == BlockStateProperties::SculkSensorPhase::Inactive;
}

u8 SculkSensorBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 原版：幽匿感测体与校准幽匿感测体固有发光 1（learned 相位另加 POWER，不在默认状态里）。
    return 1;
}

BlockStateProperties::SculkSensorPhase SculkSensorBlock::getPhase(const BlockState& state)
{
    return state.get(BlockStateProperties::SCULK_SENSOR_PHASE());
}

void SculkSensorBlock::activate(const Entity* sourceEntity,
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    i32 redstoneStrength,
    i32 frequency)
{
    // 1. 设置方块状态：Phase -> Active, Power -> redstoneStrength
    const BlockState* newState =
        &state.with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Active)
             .with(BlockStateProperties::POWER_0_15(), redstoneStrength);
    world.setBlockState(pos, newState, world::BlockUpdateFlags::UPDATE_ALL);

    // 2. 调度 tick：ACTIVE_TICKS 后触发（通过虚方法获取，校准版为10tick）
    Block& block = state.getBlockMutable();
    i32 activeTicks = static_cast<const SculkSensorBlock&>(block).getActiveTicks();
    world.tickManager().scheduleBlockTick(pos, block, activeTicks);

    // 3. 通知邻居方块红石信号变化
    world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, block);

    // 4. 触发共振事件：对相邻的共振方块（如紫水晶块）发出对应频率的 RESONATE_X 事件
    const gameevent::GameEvent* resonateEvent = gameevent::VibrationSystem::getResonanceEventByFrequency(frequency);
    if (resonateEvent != nullptr) {
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState != nullptr && BlockTags::VIBRATION_RESONATORS().contains(*neighborState)) {
                world.gameEvent(
                    *resonateEvent, neighborPos, gameevent::GameEvent::Context(sourceEntity, neighborState));
            }
        }
    }

    // 5. 发出游戏事件（幽匿感测体触须点击声）
    world.gameEvent(gameevent::GameEvents::SCULK_SENSOR_TENDRILS_CLICKING,
        pos,
        gameevent::GameEvent::Context(sourceEntity, &state));

    // 6. 播放声音（非水浸状态下）
    if (!state.get(BlockStateProperties::WATERLOGGED())) {
        world.playSound(SoundEvents::BLOCK_SCULK_SENSOR_CLICKING,
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            world.getRandom().nextFloat() * 0.2f + 0.8f);
    }
}

void SculkSensorBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 感测体自己被踩也产生一次 STEP 振动（原版 stepOn）。
    // 三个前置条件缺一不可：
    // 1. 相位为 Inactive —— 活跃/冷却期间不重复接收
    // 2. 不是监守者 —— 监守者与感测体同源，踩上去不该自激
    // 3. 非潜行 —— 潜行不会产生振动（由 STEP 的 ignore_vibrations_sneaking 标签决定）
    //
    // 走发游戏事件的路线而不是直接访问振动系统：监听器本来就在附近监听，
    // 这样 common 层不必下转 ServerWorld，也和幽匿尖啸体的踩踏实现保持一致。
    if (world.isClientSide() || !canActivate(state) || entity.getTypeId() == "minecraft:warden" ||
        entity.isSteppingCarefully()) {
        Block::onEntityWalk(state, world, pos, entity);
        return;
    }

    world.gameEvent(gameevent::GameEvents::STEP, pos, gameevent::GameEvent::Context(&entity, &state));

    Block::onEntityWalk(state, world, pos, entity);
}

void SculkSensorBlock::deactivate(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 设置方块状态：Phase -> Cooldown, Power -> 0
    const BlockState* newState =
        &state.with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Cooldown)
             .with(BlockStateProperties::POWER_0_15(), 0);
    world.setBlockState(pos, newState, world::BlockUpdateFlags::UPDATE_ALL);

    // 调度 COOLDOWN_TICKS 后再 tick（Cooldown -> Inactive）
    world.tickManager().scheduleBlockTick(pos, state.getBlock(), COOLDOWN_TICKS);

    // 通知邻居红石信号变化（从有信号变为0）
    world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, state.getBlockMutable());
}

void SculkSensorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    auto phase = getPhase(state);

    if (phase == BlockStateProperties::SculkSensorPhase::Active) {
        // Active -> deactivate() -> Cooldown
        deactivate(world, pos, state);
    } else if (phase == BlockStateProperties::SculkSensorPhase::Cooldown) {
        // Cooldown -> Inactive
        const BlockState* newState =
            &state.with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive);
        world.setBlockState(pos, newState, world::BlockUpdateFlags::UPDATE_ALL);

        // 播放 SCULK_CLICKING_STOP 声音（非水浸状态下）
        if (!state.get(BlockStateProperties::WATERLOGGED())) {
            world.playSound(SoundEvents::BLOCK_SCULK_SENSOR_CLICKING_STOP,
                sound::SoundCategory::Blocks,
                pos.center(),
                1.0f,
                world.getRandom().nextFloat() * 0.2f + 0.8f);
        }
    }
    // Inactive 状态不应有 scheduled tick，忽略
}

void SculkSensorBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 如果移除时处于 Active 状态，需要通知邻居更新红石信号
    if (getPhase(state) == BlockStateProperties::SculkSensorPhase::Active) {
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, state.getBlockMutable());
    }

    // 调用基类处理方块实体移除等
    Block::onBlockRemoved(world, pos, state, movedByPiston);
}

bool SculkSensorBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 该方块族原版显式返回 false：形状不完整不等于可以穿过。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

} // namespace blocks
} // namespace mc
