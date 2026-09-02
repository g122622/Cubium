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
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/blocks/MultifaceBlock.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "sound/SoundCategory.hpp"
#include "sound/SoundEvents.hpp"
#include "util/Direction.hpp"
#include "util/math/MathUtils.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/SupportType.hpp"
#include "world/block/WaterLoggableHelpers.hpp"

#include "world/block/blocks/MultifaceSpreader.hpp"
#include "world/block/blocks/sculk/SculkSpreader.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/Fluids.hpp"
#include "world/gameevent/GameEvents.hpp"
#include "world/gameevent/VibrationSystem.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

/// MC SculkVeinBlock.SculkVeinSpreaderConfig：脉络扩散的特殊可替换判定。
class SculkVeinSpreaderConfig : public DefaultSpreaderConfig {
public:
    SculkVeinSpreaderConfig(const SculkVeinBlock& block, std::vector<MultifaceSpreadType> spreadTypes)
        : DefaultSpreaderConfig(block)
        , m_vein(block)
        , m_spreadTypes(std::move(spreadTypes))
    {}

    // 暴露基类 1 参 stateCanBeReplaced，避免 5 参重载将其隐藏（-Woverloaded-virtual）。
    using DefaultSpreaderConfig::stateCanBeReplaced;

    [[nodiscard]] const std::vector<MultifaceSpreadType>& getSpreadTypes() const override { return m_spreadTypes; }

    /// MC SculkVeinSpreaderConfig.isOtherBlockValidAsSource = !is(SCULK_VEIN)。
    [[nodiscard]] bool isOtherBlockValidAsSource(const BlockState& state) const override { return !state.is(&m_vein); }

    /// MC SculkVeinSpreaderConfig.stateCanBeReplaced（覆写为带 sourcePos/face 的版本）。
    [[nodiscard]] bool stateCanBeReplaced(IWorld& world,
        const BlockPos& sourcePos,
        const BlockPos& targetPos,
        Direction face,
        const BlockState& targetState) const
    {
        // MC: target.relative(face) 是 SCULK/SCULK_CATALYST/MOVING_PISTON → 拒绝。
        const BlockState* beyond = world.getBlockState(targetPos.offset(face));
        if (beyond != nullptr && (beyond->is(VanillaBlocks::SCULK) || beyond->is(VanillaBlocks::SCULK_CATALYST))) {
            return false;
        }
        // MC: WRAP_AROUND（sourcePos.distManhattan(targetPos)==2）且 sourcePos.relative(face.opposite)
        //     isFaceSturdy → 拒绝（防止两个 sturdy 面之间包绕）。
        if (distManhattan(sourcePos, targetPos) == 2) {
            const BlockPos back = sourcePos.offset(Directions::opposite(face));
            const BlockState* backState = world.getBlockState(back);
            if (backState != nullptr && backState->isFaceSturdy(world, back, face, SupportType::Full)) {
                return false;
            }
        }
        // MC: 流体非空且非水 → 拒绝；是火 → 拒绝。
        const fluid::FluidState* fluid = targetState.getFluidState();
        if (fluid != nullptr && !fluid->isEmpty() && &fluid->getFluid() != fluid::Fluids::WATER()) {
            return false;
        }
        if (BlockTags::FIRE().contains(targetState)) {
            return false;
        }
        return targetState.canBeReplaced() || DefaultSpreaderConfig::stateCanBeReplaced(targetState);
    }

    /// MC DefaultSpreaderConfig.canSpreadInto = stateCanBeReplaced && isValidStateForPlacement。
    [[nodiscard]] bool canSpreadInto(
        IWorld& world, const BlockPos& sourcePos, const MultifaceSpreadPos& spreadPos) const override
    {
        const BlockState* target = world.getBlockState(spreadPos.pos);
        if (target == nullptr) {
            // nullptr 视为空气：走 isValidStateForPlacement 的 canAttachTo 判定。
            return m_vein.canAttachTo(world, spreadPos.face, spreadPos.pos.offset(spreadPos.face));
        }
        if (!stateCanBeReplaced(world, sourcePos, spreadPos.pos, spreadPos.face, *target)) {
            return false;
        }
        return m_vein.isValidStateForPlacement(world, *target, spreadPos.pos, spreadPos.face);
    }

private:
    static i32 distManhattan(const BlockPos& a, const BlockPos& b) noexcept
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
    }

    const SculkVeinBlock& m_vein;
    std::vector<MultifaceSpreadType> m_spreadTypes;
};

} // namespace

// ============================================================================
// SculkBlock
// ============================================================================

i32 SculkBlock::attemptUseCharge(ChargeCursor& cursor,
    IWorld& world,
    const BlockPos& origin,
    math::IRandom& random,
    SculkSpreader& spreader,
    bool /*shouldUpdateBlocks*/)
{
    // MC SculkBlock.attemptUseCharge:
    //   charge==0 → 直接返回；nextInt(chargeDecayRate)==0 才进入衰减/生长。
    //   距 origin 在 noGrowthRadius 内（flag=true）→ 不生长，按 additionalDecayRate 衰减。
    //   否则 canPlaceGrowth 且 nextInt(growthSpawnCost) < charge → 在上方放生长物，charge -= growthSpawnCost。
    //   否则按 additionalDecayRate 概率衰减（flag 时 -1，否则 -getDecayPenalty）。
    const i32 charge = cursor.charge();
    if (charge == 0 || random.nextInt(spreader.chargeDecayRate()) != 0) {
        return charge;
    }
    const BlockPos pos = cursor.pos();
    const i64 noGrowthSq = static_cast<i64>(spreader.noGrowthRadius()) * spreader.noGrowthRadius();
    const bool flag = pos.distanceSq(origin) < noGrowthSq;
    if (!flag && canPlaceGrowth(world, pos)) {
        const i32 cost = spreader.growthSpawnCost();
        if (random.nextInt(cost) < charge) {
            const BlockPos above = pos.up();
            const BlockState* growth = getRandomGrowthState(world, above, random, spreader.isWorldGeneration());
            if (growth != nullptr) {
                world.setBlockState(above, growth, 3);
            }
        }
        return std::max(0, charge - cost);
    }
    if (random.nextInt(spreader.additionalDecayRate()) != 0) {
        return charge;
    }
    return charge - (flag ? 1 : getDecayPenalty(spreader, pos, origin, charge));
}

const BlockState* SculkBlock::getRandomGrowthState(
    IWorld& world, const BlockPos& pos, math::IRandom& random, bool worldGen)
{
    // MC: nextInt(11)==0 → shrieker(CAN_SUMMON=worldGen)，否则 sensor。
    //     若方块有 WATERLOGGED 属性且 pos 处有流体 → 设 WATERLOGGED=true。
    const Block* block = random.nextInt(11) == 0 ? VanillaBlocks::SCULK_SHRIEKER : VanillaBlocks::SCULK_SENSOR;
    if (block == nullptr) {
        return nullptr;
    }
    const BlockState& state = block->defaultState();
    const BlockState* result = &state;
    if (block == VanillaBlocks::SCULK_SHRIEKER) {
        result = &result->with(BlockStateProperties::CAN_SUMMON(), worldGen);
    }
    // MC: hasProperty(WATERLOGGED) && !getFluidState(pos).isEmpty()
    const BlockState* posState = world.getBlockState(pos);
    const fluid::FluidState* fluid = posState != nullptr ? posState->getFluidState() : nullptr;
    if (fluid != nullptr && !fluid->isEmpty()) {
        result = &result->with(BlockStateProperties::WATERLOGGED(), true);
    }
    return result;
}

bool SculkBlock::canPlaceGrowth(IWorld& world, const BlockPos& pos)
{
    // MC: 上方为空气 或 (WATER 且 水源) → 继续；否则 false。
    //     4×3×4（offset(-4,0,-4)→(4,2,4)）范围内 SCULK_SENSOR+SCULK_SHRIEKER 计数 ≤ 2。
    const BlockState* above = world.getBlockState(pos.up());
    const bool air = (above == nullptr) || above->isAir();
    const bool waterSource = above != nullptr && above->is(VanillaBlocks::WATER) && above->getFluidState() != nullptr &&
        above->getFluidState()->isSource();
    if (!air && !waterSource) {
        return false;
    }
    i32 count = 0;
    for (i32 dx = -4; dx <= 4; ++dx) {
        for (i32 dy = 0; dy <= 2; ++dy) {
            for (i32 dz = -4; dz <= 4; ++dz) {
                const BlockState* s = world.getBlockState(pos + BlockPos{dx, dy, dz});
                if (s != nullptr && (s->is(VanillaBlocks::SCULK_SENSOR) || s->is(VanillaBlocks::SCULK_SHRIEKER))) {
                    ++count;
                    if (count > 2) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

i32 SculkBlock::getDecayPenalty(const SculkSpreader& spreader, const BlockPos& pos, const BlockPos& origin, i32 charge)
{
    // MC: f = (sqrt(distSqr) - noGrowthRadius)²; j = (24 - noGrowthRadius)²;
    //     f1 = min(1, f/j); penalty = max(1, (int)(charge * f1 * 0.5))
    const i32 noGrowth = spreader.noGrowthRadius();
    const f32 dist = std::sqrt(static_cast<f64>(pos.distanceSq(origin)));
    const f32 f = math::square(dist - static_cast<f32>(noGrowth));
    const f32 j = static_cast<f32>(math::square(24 - noGrowth));
    const f32 f1 = std::min(1.0f, f / j);
    return std::max(1, static_cast<i32>(static_cast<f32>(charge) * f1 * 0.5f));
}

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
// SculkVeinBlock
// ============================================================================

SculkVeinBlock::SculkVeinBlock(const BlockProperties& properties)
    : MultifaceBlock(properties)
    , m_veinSpreader(std::make_unique<SculkVeinSpreaderConfig>(*this, defaultSpreadOrder()))
    , m_sameSpaceSpreader(std::make_unique<SculkVeinSpreaderConfig>(
          *this, std::vector<MultifaceSpreadType>{MultifaceSpreadType::SamePosition}))
{
    buildMultifaceStateContainer();

    // 预计算 64 种面组合形状（2^6，与 GlowLichenBlock 一致：每面 1 像素薄板）。
    const CollisionShape northShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 1);
    const CollisionShape southShape = CollisionShape::fromPixelBox(0, 0, 15, 16, 16, 16);
    const CollisionShape eastShape = CollisionShape::fromPixelBox(15, 0, 0, 16, 16, 16);
    const CollisionShape westShape = CollisionShape::fromPixelBox(0, 0, 0, 1, 16, 16);
    const CollisionShape upShape = CollisionShape::fromPixelBox(0, 15, 0, 16, 16, 16);
    const CollisionShape downShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 1, 16);

    for (int down = 0; down <= 1; ++down) {
        for (int up = 0; up <= 1; ++up) {
            for (int north = 0; north <= 1; ++north) {
                for (int south = 0; south <= 1; ++south) {
                    for (int east = 0; east <= 1; ++east) {
                        for (int west = 0; west <= 1; ++west) {
                            const size_t idx = static_cast<size_t>(
                                (down) | (up << 1) | (north << 2) | (south << 3) | (east << 4) | (west << 5));
                            CollisionShape shape = CollisionShape::empty();
                            if (north) shape = CollisionShape::combine(shape, northShape);
                            if (south) shape = CollisionShape::combine(shape, southShape);
                            if (east) shape = CollisionShape::combine(shape, eastShape);
                            if (west) shape = CollisionShape::combine(shape, westShape);
                            if (up) shape = CollisionShape::combine(shape, upShape);
                            if (down) shape = CollisionShape::combine(shape, downShape);
                            if (!north && !south && !east && !west && !up && !down) {
                                shape = CollisionShape::fullBlock();
                            }
                            m_shapes[idx] = shape;
                        }
                    }
                }
            }
        }
    }
}

size_t SculkVeinBlock::shapeIndex(const BlockState& state)
{
    size_t index = 0;
    if (state.get(BlockStateProperties::DOWN())) index |= 1;
    if (state.get(BlockStateProperties::UP())) index |= 2;
    if (state.get(BlockStateProperties::NORTH())) index |= 4;
    if (state.get(BlockStateProperties::SOUTH())) index |= 8;
    if (state.get(BlockStateProperties::EAST())) index |= 16;
    if (state.get(BlockStateProperties::WEST())) index |= 32;
    return index;
}

BlockState SculkVeinBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const Direction clickedFace = context.getClickedFace();
    const BlockState* current = context.getWorld().getBlockState(context.placementPos());
    const BlockState* placed =
        MultifaceBlock::getStateForPlacement(current, context.getWorld(), context.placementPos(), clickedFace);
    if (placed == nullptr) {
        return defaultState();
    }
    return *placed;
}

BlockState SculkVeinBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& SculkVeinBlock::getShape(const BlockState& state) const
{
    return m_shapes[shapeIndex(state)];
}

const fluid::FluidState* SculkVeinBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

// ========== SculkVeinBlock 的 SculkBehaviour 实现 ==========

i32 SculkVeinBlock::attemptUseCharge(ChargeCursor& cursor,
    IWorld& world,
    const BlockPos& /*origin*/,
    math::IRandom& random,
    SculkSpreader& spreader,
    bool shouldUpdateBlocks)
{
    // MC SculkVeinBlock.attemptUseCharge:
    //   shouldUpdateBlocks && attemptPlaceSculk → charge - 1；
    //   否则 nextInt(chargeDecayRate)==0 ? floor(charge*0.5) : charge。
    if (shouldUpdateBlocks && attemptPlaceSculk(spreader, world, cursor.pos(), random)) {
        return cursor.charge() - 1;
    }
    if (random.nextInt(spreader.chargeDecayRate()) == 0) {
        return static_cast<i32>(std::floor(static_cast<f32>(cursor.charge()) * 0.5f));
    }
    return cursor.charge();
}

bool SculkVeinBlock::attemptSpreadVein(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    std::optional<std::vector<Direction>> facings,
    bool worldGen)
{
    // MC SculkBehaviour.DEFAULT.attemptSpreadVein（SculkVeinBlock 未覆写，走接口默认）：
    //   facings==null → sameSpaceSpreader.spreadAll(currentState, world, pos, worldGen) > 0；
    //   facings 非空且非 empty → (!isAir && !fluid.WATER) ? false : regrow(world, pos, state, facings)；
    //   facings empty → super（即 false）。
    if (!facings.has_value()) {
        const BlockState* current = world.getBlockState(pos);
        if (current == nullptr) {
            return m_sameSpaceSpreader.spreadAll(defaultState(), world, pos, worldGen) > 0;
        }
        return m_sameSpaceSpreader.spreadAll(*current, world, pos, worldGen) > 0;
    }
    if (facings.value().empty()) {
        return false;
    }
    if (!state.isAir()) {
        const fluid::FluidState* fluid = state.getFluidState();
        if (fluid == nullptr || &fluid->getFluid() != fluid::Fluids::WATER()) {
            return false;
        }
    }
    return regrow(world, pos, state, facings.value());
}

void SculkVeinBlock::onDischarged(IWorld& world, const BlockState& state, const BlockPos& pos, math::IRandom& random)
{
    // MC SculkVeinBlock.onDischarged: 移除贴在 SCULK 上的面；无面则变 AIR/WATER。
    MC_UNUSED(random);
    if (!state.is(this)) {
        return;
    }
    const BlockState* result = &state;
    for (Direction dir : Directions::all()) {
        if (MultifaceBlock::hasFace(*result, dir)) {
            const BlockState* neighbor = world.getBlockState(pos.offset(dir));
            if (neighbor != nullptr && neighbor->is(VanillaBlocks::SCULK)) {
                result = &result->with(*MultifaceBlock::getFaceProperty(dir), false);
            }
        }
    }
    if (!MultifaceBlock::hasAnyFace(*result)) {
        const fluid::FluidState* fluid =
            world.getBlockState(pos) != nullptr ? world.getBlockState(pos)->getFluidState() : nullptr;
        result = (fluid != nullptr && !fluid->isEmpty()) ? &VanillaBlocks::WATER->defaultState()
                                                         : &VanillaBlocks::AIR->defaultState();
    }
    world.setBlockState(pos, result, 3);
}

bool SculkVeinBlock::hasSubstrateAccess(IWorld& world, const BlockState& state, const BlockPos& pos)
{
    // MC SculkVeinBlock.hasSubstrateAccess: is(SCULK_VEIN) 且某面朝向 SCULK_REPLACEABLE。
    if (!state.is(VanillaBlocks::SCULK_VEIN)) {
        return false;
    }
    for (Direction dir : Directions::all()) {
        if (MultifaceBlock::hasFace(state, dir)) {
            const BlockState* neighbor = world.getBlockState(pos.offset(dir));
            if (neighbor != nullptr && BlockTags::SCULK_REPLACEABLE().contains(*neighbor)) {
                return true;
            }
        }
    }
    return false;
}

bool SculkVeinBlock::regrow(
    IWorld& world, const BlockPos& pos, const BlockState& current, const std::vector<Direction>& directions)
{
    // MC SculkVeinBlock.regrow: 默认 vein 状态逐方向加面（仅 canAttachTo 的方向）；
    //   无任何可加面 → false；有流体则 WATERLOGGED。
    BlockState vein = VanillaBlocks::SCULK_VEIN->defaultState();
    bool any = false;
    for (Direction dir : directions) {
        if (MultifaceBlock::canAttachTo(world, dir, pos.offset(dir))) {
            vein = vein.with(*MultifaceBlock::getFaceProperty(dir), true);
            any = true;
        }
    }
    if (!any) {
        return false;
    }
    const fluid::FluidState* fluid = current.getFluidState();
    if (fluid != nullptr && !fluid->isEmpty()) {
        vein = vein.with(BlockStateProperties::WATERLOGGED(), true);
    }
    world.setBlockState(pos, &vein, 3);
    return true;
}

bool SculkVeinBlock::attemptPlaceSculk(
    SculkSpreader& spreader, IWorld& world, const BlockPos& pos, math::IRandom& random)
{
    // MC SculkVeinBlock.attemptPlaceSculk:
    //   打乱方向遍历，对 vein 已有面的方向，取相邻格；若属 replaceableBlocks → 放 SCULK +
    //   pushEntitiesUp + veinSpreader.spreadAll(sculk, world, neighbor, worldGen) +
    //   对新 sculk 除来源反方向外的相邻 vein 调 onDischarged。
    const BlockState* veinState = world.getBlockState(pos);
    if (veinState == nullptr) {
        return false;
    }
    const BlockTag& replaceable = spreader.replaceableBlocks();

    std::vector<Direction> dirs = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    random.shuffle(dirs);

    for (Direction dir : dirs) {
        if (!MultifaceBlock::hasFace(*veinState, dir)) {
            continue;
        }
        const BlockPos neighbor = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighbor);
        if (neighborState == nullptr || !replaceable.contains(*neighborState)) {
            continue;
        }
        const BlockState* sculkState = &VanillaBlocks::SCULK->defaultState();
        world.setBlockState(neighbor, sculkState, 3);
        Block::pushEntitiesUp(*neighborState, *sculkState, world, neighbor);
        // MC: veinSpreader.spreadAll 返回成功扩散数，此处仅用副作用（蔓延脉络），忽略计数。
        (void)m_veinSpreader.spreadAll(*sculkState, world, neighbor, spreader.isWorldGeneration());

        const Direction sourceOpposite = Directions::opposite(dir);
        for (Direction d2 : Directions::all()) {
            if (d2 == sourceOpposite) {
                continue;
            }
            const BlockPos n2 = neighbor.offset(d2);
            const BlockState* n2State = world.getBlockState(n2);
            if (n2State != nullptr && n2State->is(this)) {
                onDischarged(world, *n2State, n2, random);
            }
        }
        return true;
    }
    return false;
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
    world.gameEvent(gameevent::GameEvents::SHRIEK, pos, gameevent::GameEvent::Context(&entity, &state));
}

void SculkShriekerBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // SHRIEKING 状态到期后转回非 SHRIEKING 状态
    if (state.get(BlockStateProperties::SHRIEKING())) {
        // 将 SHRIEKING 设回 false
        const BlockState* newState = &state.with(BlockStateProperties::SHRIEKING(), false);
        world.setBlockState(pos, newState, 3);

        // 尖啸结束后，通知服务端执行响应逻辑（警告声音、黑暗效果、召唤检查）
        // 由于 block tick 在 common 层，设置方块实体标记以让服务端处理
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
    // 1. 设置 SHRIEKING 方块状态为 true
    const BlockState* newState = &state.with(BlockStateProperties::SHRIEKING(), true);
    world.setBlockState(pos, newState, 3);

    // 2. 调度 tick：SHRIEKING_TICKS 后触发状态转换回 false
    world.tickManager().scheduleBlockTick(pos, state.getBlock(), SHRIEKING_TICKS);

    // 3. 发出尖啸粒子效果（事件 ID 3007）
    world.playEvent(mc::world::WorldEvents::SCULK_SHRIEK, pos, 0);

    // 4. 发出 SHRIEK 游戏事件（通知附近的幽匿感测体/尖啸体振动系统）
    const BlockState* currentState = world.getBlockState(pos);
    world.gameEvent(gameevent::GameEvents::SHRIEK,
        pos,
        gameevent::GameEvent::Context(sourceEntity, currentState != nullptr ? currentState : &state));
}

} // namespace blocks
} // namespace mc
