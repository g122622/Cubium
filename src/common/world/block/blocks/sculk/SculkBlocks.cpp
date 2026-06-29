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

#include "SculkBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "sound/SoundCategory.hpp"
#include "sound/SoundEvents.hpp"
#include "util/Direction.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/WorldConstants.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/WaterLoggableHelpers.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "world/gameevent/GameEvents.hpp"
#include "world/gameevent/VibrationSystem.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "world/tick/base/TickPriority.hpp"
#include "world/tick/manager/TickManager.hpp"

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
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
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
    // 对齐 MC Java: SculkSensorBlock.getAnalogOutputSignal()
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
    world.setBlockState(pos, newState, 3);

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

void SculkSensorBlock::deactivate(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 设置方块状态：Phase -> Cooldown, Power -> 0
    const BlockState* newState =
        &state.with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Cooldown)
             .with(BlockStateProperties::POWER_0_15(), 0);
    world.setBlockState(pos, newState, 3);

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
        world.setBlockState(pos, newState, 3);

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

void SculkSensorBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 对齐 MC Java: SculkSensorBlock.affectNeighborsAfterRemoval()
    // 如果移除时处于 Active 状态，需要通知邻居更新红石信号
    if (getPhase(state) == BlockStateProperties::SculkSensorPhase::Active) {
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, state.getBlockMutable());
    }

    // 调用基类处理方块实体移除等
    Block::onBlockRemoved(world, pos, state);
}

// ============================================================================
// CalibratedSculkSensorBlock
// ============================================================================

CalibratedSculkSensorBlock::CalibratedSculkSensorBlock(const BlockProperties& properties)
    : SculkSensorBlock(properties)
{
    // 校准幽匿感测体额外添加 HORIZONTAL_FACING 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SCULK_SENSOR_PHASE())
            .add(BlockStateProperties::POWER_0_15())
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive)
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

void CalibratedSculkSensorBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState CalibratedSculkSensorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = SculkSensorBlock::getStateForPlacement(context);
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(context.horizontalDirection()));
    return state;
}

i32 CalibratedSculkSensorBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 对齐 MC Java: CalibratedSculkSensorBlock.getSignal()
    // FACING 方向是输入面（从该方向读取红石信号频率过滤），
    // 红石信号只在非 FACING 方向输出。
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (side == facing) {
        return 0;
    }
    return SculkSensorBlock::getWeakPower(state, world, pos, side);
}

const BlockState& CalibratedSculkSensorBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rotation));
}

const BlockState& CalibratedSculkSensorBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rot));
}

// ============================================================================
// SculkCatalystBlock
// ============================================================================

SculkCatalystBlock::SculkCatalystBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::BLOOM())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::BLOOM(), false));

    m_shape = CollisionShape::fullBlock();
}

void SculkCatalystBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

const CollisionShape& SculkCatalystBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

i32 SculkCatalystBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return state.get(BlockStateProperties::BLOOM()) ? 15 : 0;
}

// ============================================================================
// SculkShriekerBlock
// ============================================================================

SculkShriekerBlock::SculkShriekerBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SHRIEKING())
            .add(BlockStateProperties::CAN_SUMMON())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::SHRIEKING(), false)
            .with(BlockStateProperties::CAN_SUMMON(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 8, 16);
}

void SculkShriekerBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState SculkShriekerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState()
                           .with(BlockStateProperties::SHRIEKING(), false)
                           .with(BlockStateProperties::CAN_SUMMON(), false)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState SculkShriekerBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& SculkShriekerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* SculkShriekerBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

std::unique_ptr<BlockEntity> SculkShriekerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SculkShriekerBlockEntity>(pos);
}

BlockEntityType SculkShriekerBlock::getBlockEntityType() const
{
    return BlockEntityType::SculkShrieker;
}

// ========== SculkShriekerBlock 激活逻辑 ==========

void SculkShriekerBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 对齐 MC Java: SculkShriekerBlock.stepOn()
    // 只有非潜行实体踩上时才发出 SHRIEK 游戏事件
    // 潜行实体不会触发幽匿尖啸体
    if (entity.isSteppingCarefully()) {
        return;
    }

    // 当前处于 SHRIEKING 状态时不重复触发
    if (state.get(BlockStateProperties::SHRIEKING())) {
        return;
    }

    // 发出 SHRIEK 游戏事件，通知附近的幽匿尖啸体振动系统
    // 对齐 MC Java: SculkShriekerBlock.stepOn() 中调用 gameEvent(GameEvent.SHRIEK)
    world.gameEvent(gameevent::GameEvents::SHRIEK, pos, gameevent::GameEvent::Context(&entity, &state));
}

void SculkShriekerBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 对齐 MC Java: SculkShriekerBlock.tick()
    // SHRIEKING 状态到期后转回非 SHRIEKING 状态
    if (state.get(BlockStateProperties::SHRIEKING())) {
        // 将 SHRIEKING 设回 false
        const BlockState* newState = &state.with(BlockStateProperties::SHRIEKING(), false);
        world.setBlockState(pos, newState, 3);

        // 尖啸结束后，通知服务端执行响应逻辑（警告声音、黑暗效果、召唤检查）
        // 通过 gameEvent 机制通知，服务端 SculkShriekerHelper 会处理
        // 这里用一个 SHRIEK 完成事件标记（MC Java 中直接在 block entity 调用 tryRespond）
        // 由于 block tick 在 common 层，我们设置方块实体标记以让服务端处理
        BlockEntity* be = world.getBlockEntity(pos);
        if (be != nullptr && be->getType() == BlockEntityType::SculkShrieker) {
            auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);
            shrieker->setShriekingFinished(true);
            shrieker->setChanged();
        }
    }
}

void SculkShriekerBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 对齐 MC Java: SculkShriekerBlock.preRemoveSideEffects()
    // 如果方块正在 SHRIEKING 状态时被移除，仍需执行响应逻辑
    if (state.get(BlockStateProperties::SHRIEKING())) {
        BlockEntity* be = world.getBlockEntity(pos);
        if (be != nullptr && be->getType() == BlockEntityType::SculkShrieker) {
            auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);
            shrieker->setShriekingFinished(true);
            shrieker->setChanged();
        }
    }

    Block::onBlockRemoved(world, pos, state);
}

void SculkShriekerBlock::shriek(IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* sourceEntity)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.shriek()
    // 1. 设置 SHRIEKING 方块状态为 true
    const BlockState* newState = &state.with(BlockStateProperties::SHRIEKING(), true);
    world.setBlockState(pos, newState, 3);

    // 2. 调度 tick：SHRIEKING_TICKS 后触发状态转换回 false
    world.tickManager().scheduleBlockTick(pos, state.getBlock(), SHRIEKING_TICKS);

    // 3. 发出尖啸粒子效果（事件 ID 3007）
    // 对齐 MC Java: ServerLevel.levelEvent(3007, pos, 0)
    world.playEvent(mc::world::WorldEvents::SCULK_SHRIEK, pos, 0);

    // 4. 发出 SHRIEK 游戏事件（通知附近的幽匿感测体/尖啸体振动系统）
    // 对齐 MC Java: level.gameEvent(GameEvent.SHRIEK, pos, Context.of(sourceEntity))
    const BlockState* currentState = world.getBlockState(pos);
    world.gameEvent(gameevent::GameEvents::SHRIEK,
        pos,
        gameevent::GameEvent::Context(sourceEntity, currentState != nullptr ? currentState : &state));
}

} // namespace blocks
} // namespace mc
