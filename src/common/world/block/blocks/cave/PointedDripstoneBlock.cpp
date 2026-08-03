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
 */

#include "PointedDripstoneBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/CauldronBlock.hpp"
#include "common/world/block/blocks/LavaCauldronBlock.hpp"
#include "common/world/block/blocks/LayeredCauldronBlock.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include "common/world/block/registry/MudBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // 将 BlockStateProperties 引入作用域

// ============================================================================
// 构造函数
// ============================================================================

PointedDripstoneBlock::PointedDripstoneBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建各厚度的碰撞形状
    // TipMerge: 半径6像素，全高
    m_shapes[BlockStateProperties::DripstoneThickness::TipMerge] = CollisionShape::fromPixelBox(5, 0, 5, 11, 16, 11);
    // Tip 朝上: 半径6像素，底部到11像素高（尖端在上方）
    m_shapes[BlockStateProperties::DripstoneThickness::Tip] = CollisionShape::fromPixelBox(4, 0, 4, 12, 16, 12);
    // Frustum: 半径8像素，全高
    m_shapes[BlockStateProperties::DripstoneThickness::Frustum] = CollisionShape::fromPixelBox(3, 0, 3, 13, 16, 13);
    // Middle: 半径10像素，全高
    m_shapes[BlockStateProperties::DripstoneThickness::Middle] = CollisionShape::fromPixelBox(2, 0, 2, 14, 16, 14);
    // Base: 半径12像素，全高
    m_shapes[BlockStateProperties::DripstoneThickness::Base] = CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15);

    // 朝下尖端的特殊形状：尖端在下方（5像素到顶部）
    m_tipDownShape = CollisionShape::fromPixelBox(4, 0, 4, 12, 11, 12);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::VERTICAL_DIRECTION())
            .add(BlockStateProperties::DRIPSTONE_THICKNESS())
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
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

// ============================================================================
// 方块状态容器
// ============================================================================

void PointedDripstoneBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

// ============================================================================
// 放置逻辑
// ============================================================================

BlockState PointedDripstoneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据点击面确定首选方向：点击顶面→朝下(钟乳石)，点击底面→朝上(石笋)
    Direction clickedFace = context.getClickedFace();
    Direction preferredDir;
    if (clickedFace == Direction::Up) {
        preferredDir = Direction::Down; // 点击顶面 → 从上方悬挂的钟乳石
    } else if (clickedFace == Direction::Down) {
        preferredDir = Direction::Up; // 点击底面 → 从下方生长的石笋
    } else {
        // 侧面的情况：默认朝下
        preferredDir = Direction::Down;
    }

    // 计算实际放置方向
    Direction tipDirection = calculateTipDirection(world, pos, preferredDir);
    if (tipDirection == Direction::None) {
        // 两边都不能放置，返回默认状态（放置可能失败）
        return defaultState();
    }

    // 潜行时不合并尖端（isTipMerge = false），非潜行时允许合并
    Player* player = context.getPlayer();
    bool isSecondaryUseActive = player != nullptr && player->isSneaking();
    bool isTipMerge = !isSecondaryUseActive;
    auto thickness = calculateDripstoneThickness(world, pos, tipDirection, isTipMerge);

    // 检查含水
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::VERTICAL_DIRECTION(), tipDirection)
        .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool PointedDripstoneBlock::isValidPosition(
    const BlockState& state, IBlockReader& blockReader, const BlockPos& pos) const
{
    Direction tipDirection = state.get(BlockStateProperties::VERTICAL_DIRECTION());
    // 获取支撑方向的反方向
    Direction supportDir = Directions::opposite(tipDirection);
    BlockPos supportPos = pos.offset(supportDir);

    // 检查支撑方块是否有坚固面，或是否为同方向滴石
    // IBlockReader 继承自 IWorld，使用 static_cast 安全
    IWorld& world = static_cast<IWorld&>(blockReader);
    const BlockState* supportState = world.getBlockState(supportPos);
    if (supportState == nullptr) {
        return false;
    }

    if (supportState->isSolidSide(world, supportPos, tipDirection)) {
        return true;
    }
    return isPointedDripstoneWithDirection(supportState, tipDirection);
}

// ============================================================================
// 邻居更新
// ============================================================================

BlockState PointedDripstoneBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 处理含水调度
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 只关心上下方向的变化
    if (facing != Direction::Up && facing != Direction::Down) {
        return state;
    }

    Direction tipDirection = state.get(BlockStateProperties::VERTICAL_DIRECTION());

    // 如果支撑方向发生变化且不能存活，调度破坏 tick
    if (facing == Directions::opposite(tipDirection)) {
        if (!isValidPointedDripstonePlacement(world, currentPos, tipDirection)) {
            if (tipDirection == Direction::Down) {
                // 钟乳石：延迟2tick掉落
                world.tickManager().scheduleBlockTick(currentPos, *this, DELAY_BEFORE_FALLING);
            } else {
                // 石笋：延迟1tick破坏
                world.tickManager().scheduleBlockTick(currentPos, *this, 1);
            }
            return state;
        }
    }

    // 重新计算厚度
    bool isTipMerge =
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()) == BlockStateProperties::DripstoneThickness::TipMerge;
    auto newThickness = calculateDripstoneThickness(world, currentPos, tipDirection, isTipMerge);

    return state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), newThickness);
}

// ============================================================================
// 碰撞形状
// ============================================================================

const CollisionShape& PointedDripstoneBlock::getShape(const BlockState& state) const
{
    auto thickness = state.get(BlockStateProperties::DRIPSTONE_THICKNESS());

    // Tip 根据方向有不同形状
    if (thickness == BlockStateProperties::DripstoneThickness::Tip) {
        Direction dir = state.get(BlockStateProperties::VERTICAL_DIRECTION());
        if (dir == Direction::Down) {
            return m_tipDownShape;
        }
    }

    auto it = m_shapes.find(thickness);
    if (it != m_shapes.end()) {
        return it->second;
    }
    return m_shapes.at(BlockStateProperties::DripstoneThickness::Tip);
}

// ============================================================================
// 流体状态
// ============================================================================

const fluid::FluidState* PointedDripstoneBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

// ============================================================================
// 随机刻 - 核心生长逻辑
// ============================================================================

void PointedDripstoneBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 1. 尝试流体传输（水/岩浆到炼药锅，泥巴转粘土）
    maybeTransferFluid(state, world, pos, random.nextFloat());

    // 2. 以 GROWTH_PROBABILITY_PER_RANDOM_TICK 概率尝试生长
    if (random.nextFloat() >= GROWTH_PROBABILITY_PER_RANDOM_TICK) {
        return;
    }

    // 3. 必须是钟乳石起点位置
    if (!isStalactiteStartPos(state, world, pos)) {
        return;
    }

    // 4. 尝试生长
    growStalactiteOrStalagmiteIfPossible(state, world, pos, random);
}

// ============================================================================
// Tick - 处理支撑失效
// ============================================================================

void PointedDripstoneBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    if (isStalagmite(state) &&
        !isValidPointedDripstonePlacement(world, pos, state.get(BlockStateProperties::VERTICAL_DIRECTION()))) {
        // 石笋失去支撑：直接破坏并掉落物品
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            spawnAfterBreak(world, pos, state, nullptr, false);
            world.setBlockState(pos, airState, 3);
        }
    } else if (isStalactite(state)) {
        // 钟乳石失去支撑：从最高处开始逐个生成掉落方块实体
        // 找到钟乳石柱的起始位置（最高处），然后逐个掉落
        // 从触发位置开始向下遍历
        _spawnFallingStalactite(world, pos, state);
    }
}

// ============================================================================
// 摔落伤害（踩在石笋尖端上增加伤害）
// ============================================================================

void PointedDripstoneBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只有朝上的TIP厚度才增加摔落伤害
    // 石笋替代普通摔落伤害
    if (state.get(BlockStateProperties::VERTICAL_DIRECTION()) == Direction::Up &&
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()) == BlockStateProperties::DripstoneThickness::Tip) {
        // 石笋伤害：摔落距离 + 2.5，伤害倍率 2.0
        // 不调用父类 onFallenUpon，替代普通摔落伤害
        entity.causeFallDamage(fallDistance + STALAGMITE_FALL_DISTANCE_OFFSET,
            static_cast<f32>(STALAGMITE_FALL_DAMAGE_MODIFIER),
            DamageSources::stalagmite());
        return;
    }

    // 非尖端滴石：使用父类默认行为（施加普通摔落伤害）
    Block::onFallenUpon(world, pos, state, entity, fallDistance);
}

// ============================================================================
// 旋转和镜像
// ============================================================================

const BlockState& PointedDripstoneBlock::rotate(const BlockState& state, Rotation rotation) const
{
    MC_UNUSED(rotation);
    // 垂直方向不受旋转影响
    return state;
}

const BlockState& PointedDripstoneBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 对于VERTICAL_DIRECTION，镜像时Up/Down互换
    Direction verticalDir = state.get(BlockStateProperties::VERTICAL_DIRECTION());
    if (mirror == Mirror::FrontBack || mirror == Mirror::LeftRight) {
        if (verticalDir == Direction::Up) {
            return state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
        }
        if (verticalDir == Direction::Down) {
            return state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
        }
    }
    return state;
}

// ============================================================================
// 静态辅助方法
// ============================================================================

bool PointedDripstoneBlock::isPointedDripstoneWithDirection(const BlockState* state, Direction direction)
{
    return state != nullptr && state->is(VanillaBlocks::POINTED_DRIPSTONE) &&
        state->get(BlockStateProperties::VERTICAL_DIRECTION()) == direction;
}

bool PointedDripstoneBlock::isStalactite(const BlockState& state)
{
    return isPointedDripstoneWithDirection(&state, Direction::Down);
}

bool PointedDripstoneBlock::isStalagmite(const BlockState& state)
{
    return isPointedDripstoneWithDirection(&state, Direction::Up);
}

bool PointedDripstoneBlock::isTip(const BlockState* state, bool allowMerge)
{
    if (state == nullptr || !state->is(VanillaBlocks::POINTED_DRIPSTONE)) {
        return false;
    }
    auto thickness = state->get(BlockStateProperties::DRIPSTONE_THICKNESS());
    return thickness == BlockStateProperties::DripstoneThickness::Tip ||
        (allowMerge && thickness == BlockStateProperties::DripstoneThickness::TipMerge);
}

bool PointedDripstoneBlock::isUnmergedTipWithDirection(const BlockState* state, Direction direction)
{
    return isTip(state, false) && state->get(BlockStateProperties::VERTICAL_DIRECTION()) == direction;
}

bool PointedDripstoneBlock::isStalactiteStartPos(const BlockState& state, IWorld& world, const BlockPos& pos)
{
    if (!isStalactite(state)) {
        return false;
    }
    // 上方不是滴石 → 这是钟乳石柱的根部
    const BlockState* above = world.getBlockState(pos.up());
    return above == nullptr || !above->is(VanillaBlocks::POINTED_DRIPSTONE);
}

bool PointedDripstoneBlock::canDrip(const BlockState& state)
{
    return isStalactite(state) &&
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()) == BlockStateProperties::DripstoneThickness::Tip &&
        !state.get(BlockStateProperties::WATERLOGGED());
}

bool PointedDripstoneBlock::canTipGrow(const BlockState& state, IWorld& world, const BlockPos& pos)
{
    Direction tipDir = state.get(BlockStateProperties::VERTICAL_DIRECTION());
    BlockPos growPos = pos.offset(tipDir);
    const BlockState* growState = world.getBlockState(growPos);

    // 生长方向前方有流体，不能生长
    if (growState != nullptr) {
        const fluid::FluidState* fluid = world.getFluidState(growPos);
        if (fluid != nullptr && !fluid->isEmpty()) {
            return false;
        }
    }

    // 前方是空气，或是对向的未合并TIP
    if (growState == nullptr || growState->isAir()) {
        return true;
    }
    return isUnmergedTipWithDirection(growState, Directions::opposite(tipDir));
}

bool PointedDripstoneBlock::canGrow(IWorld& world, const BlockPos& pos)
{
    // 上方1格是滴水石块 + 上方2格是水源
    const BlockState* aboveState = world.getBlockState(pos.up());
    if (aboveState == nullptr || !aboveState->is(VanillaBlocks::DRIPSTONE_BLOCK)) {
        return false;
    }

    // 检查上方2格是否为水源
    const fluid::FluidState* aboveFluid = world.getFluidState(pos.up(2));
    return aboveFluid != nullptr && aboveFluid->isSource() && aboveFluid->getFluid().isIn(fluid::FluidTags::WATER());
}

bool PointedDripstoneBlock::isValidPointedDripstonePlacement(IWorld& world, const BlockPos& pos, Direction direction)
{
    // 检查支撑方向（反方向）的方块
    BlockPos supportPos = pos.offset(Directions::opposite(direction));
    const BlockState* supportState = world.getBlockState(supportPos);

    if (supportState == nullptr) {
        return false;
    }

    // 支撑方块在该方向有坚固面，或者支撑方块是同方向的滴石
    if (supportState->isSolidSide(world, supportPos, direction)) {
        return true;
    }
    return isPointedDripstoneWithDirection(supportState, direction);
}

Direction PointedDripstoneBlock::calculateTipDirection(IWorld& world, const BlockPos& pos, Direction preferredDir)
{
    if (isValidPointedDripstonePlacement(world, pos, preferredDir)) {
        return preferredDir;
    }
    Direction oppositeDir = Directions::opposite(preferredDir);
    if (isValidPointedDripstonePlacement(world, pos, oppositeDir)) {
        return oppositeDir;
    }
    // 两边都不能放置
    return Direction::None;
}

BlockStateProperties::DripstoneThickness PointedDripstoneBlock::calculateDripstoneThickness(
    IWorld& world, const BlockPos& pos, Direction tipDirection, bool isTipMerge)
{
    Direction supportDir = Directions::opposite(tipDirection);

    // 检查生长方向上的邻居
    BlockPos growPos = pos.offset(tipDirection);
    const BlockState* growState = world.getBlockState(growPos);

    // 情况1：生长方向上有反方向的滴石 → 两个尖端对接
    if (isPointedDripstoneWithDirection(growState, supportDir)) {
        // 非合并模式 且 对方不是TIP_MERGE → TIP
        // 合并模式 或 对方是TIP_MERGE → TIP_MERGE
        if (!isTipMerge && growState != nullptr &&
            growState->get(BlockStateProperties::DRIPSTONE_THICKNESS()) !=
                BlockStateProperties::DripstoneThickness::TipMerge) {
            return BlockStateProperties::DripstoneThickness::Tip;
        }
        return BlockStateProperties::DripstoneThickness::TipMerge;
    }

    // 情况2：生长方向上没有同方向滴石 → 这是最尖端
    if (!isPointedDripstoneWithDirection(growState, tipDirection)) {
        return BlockStateProperties::DripstoneThickness::Tip;
    }

    // 情况3：生长方向上有同方向滴石 → 根据上方厚度推断
    auto growThickness = growState->get(BlockStateProperties::DRIPSTONE_THICKNESS());

    if (growThickness != BlockStateProperties::DripstoneThickness::Tip &&
        growThickness != BlockStateProperties::DripstoneThickness::TipMerge) {
        // 上方是 FRUSTUM/MIDDLE/BASE → 检查支撑方向是否有同向滴石
        BlockPos supportPos2 = pos.offset(supportDir);
        const BlockState* supportState = world.getBlockState(supportPos2);
        if (!isPointedDripstoneWithDirection(supportState, tipDirection)) {
            // 支撑方向没有同向滴石 → 这是 BASE
            return BlockStateProperties::DripstoneThickness::Base;
        }
        // 支撑方向也有同向滴石 → 这是 MIDDLE
        return BlockStateProperties::DripstoneThickness::Middle;
    }

    // 上方是 TIP 或 TIP_MERGE → 这是 FRUSTUM
    return BlockStateProperties::DripstoneThickness::Frustum;
}

std::optional<BlockPos> PointedDripstoneBlock::findTip(
    const BlockState& state, IWorld& world, const BlockPos& pos, i32 maxDistance, bool allowMerge)
{
    // 如果自身就是尖端，直接返回
    if (isTip(&state, allowMerge)) {
        return pos;
    }

    Direction tipDirection = state.get(BlockStateProperties::VERTICAL_DIRECTION());
    BlockPosMutable mutablePos(pos.x, pos.y, pos.z);

    for (i32 i = 1; i < maxDistance; i++) {
        mutablePos.move(tipDirection);
        if (!world.isWithinWorldBounds(mutablePos.toImmutable())) {
            break;
        }
        const BlockState* currentState = world.getBlockState(mutablePos.toImmutable());

        if (currentState == nullptr) {
            break;
        }

        if (isTip(currentState, allowMerge)) {
            return mutablePos.toImmutable();
        }

        // 不是同方向的滴石，停止搜索
        if (!isPointedDripstoneWithDirection(currentState, tipDirection)) {
            break;
        }
    }

    return std::nullopt;
}

std::optional<BlockPos> PointedDripstoneBlock::findFillableCauldronBelow(
    IWorld& world, const BlockPos& tipPos, const fluid::Fluid& fluid)
{
    BlockPosMutable mutablePos(tipPos.x, tipPos.y, tipPos.z);

    for (i32 i = 0; i < MAX_SEARCH_LENGTH_BETWEEN_TIP_AND_CAULDRON; i++) {
        mutablePos.move(Direction::Down);
        if (!world.isWithinWorldBounds(mutablePos.toImmutable())) {
            break;
        }
        const BlockState* state = world.getBlockState(mutablePos.toImmutable());

        if (state == nullptr) {
            break;
        }

        // 检查是否为可接收滴水的炼药锅
        // 参考 MC 原版：使用 instanceof AbstractCauldronBlock && canReceiveStalactiteDrip(fluid)
        const Block& block = state->getBlock();
        bool isCauldron = (&block == block_registry::BuildingBlocks::CAULDRON);
        bool isWaterCauldron = (&block == block_registry::BuildingBlocks::WATER_CAULDRON);
        bool isLavaCauldron = (&block == block_registry::BuildingBlocks::LAVA_CAULDRON);

        if (isCauldron || isWaterCauldron || isLavaCauldron) {
            // 检查该炼药锅是否可以接收指定流体的滴水
            if (isCauldron && CauldronBlock::canReceiveStalactiteDrip(fluid)) {
                // 空炼药锅可以接收任何流体（水和岩浆）
                return mutablePos.toImmutable();
            }
            if (isWaterCauldron) {
                // 水炼药锅：检查 canReceiveStalactiteDrip（仅水炼药锅可接收水滴）
                auto* waterCauldron = static_cast<const blocks::LayeredCauldronBlock*>(&block);
                if (waterCauldron->canReceiveStalactiteDrip(fluid) && !blocks::LayeredCauldronBlock::isFull(*state)) {
                    return mutablePos.toImmutable();
                }
            }
            // LavaCauldronBlock::canReceiveStalactiteDrip 始终返回 false，不可接收任何滴水
            // 不匹配的炼药锅不返回，继续检查 canDripThrough
        }

        // 检查是否可以穿过
        if (!canDripThrough(world, mutablePos.toImmutable(), state)) {
            break;
        }
    }

    return std::nullopt;
}

std::optional<BlockPos> PointedDripstoneBlock::findStalactiteTipAboveCauldron(IWorld& world, const BlockPos& pos)
{
    // 从炼药锅位置向上搜索可滴水的钟乳石尖端
    // 使用与 findFillableCauldronBelow 相同的 canDripThrough 检查来穿透方块
    BlockPosMutable mutablePos(pos.x, pos.y, pos.z);

    for (i32 i = 0; i < MAX_SEARCH_LENGTH_BETWEEN_TIP_AND_CAULDRON; i++) {
        mutablePos.move(Direction::Up);
        if (!world.isWithinWorldBounds(mutablePos.toImmutable())) {
            break;
        }
        const BlockState* state = world.getBlockState(mutablePos.toImmutable());

        if (state == nullptr) {
            break;
        }

        // 找到可滴水的钟乳石尖端（朝下的 TIP，不含水）
        // 注意：canDrip 只接受 TIP 厚度，不接受 TIP_MERGE
        if (canDrip(*state)) {
            return mutablePos.toImmutable();
        }

        // 不可穿透的方块停止搜索
        if (!canDripThrough(world, mutablePos.toImmutable(), state)) {
            break;
        }
    }

    return std::nullopt;
}

const fluid::Fluid* PointedDripstoneBlock::getCauldronFillFluidType(IWorld& world, const BlockPos& tipPos)
{
    // 从钟乳石尖端向上查找根方块，然后检查根方块上方的流体
    const BlockState* tipState = world.getBlockState(tipPos);
    if (tipState == nullptr || !isStalactite(*tipState)) {
        return fluid::Fluids::EMPTY();
    }

    std::optional<BlockPos> rootPosOpt =
        findRootBlock(world, tipPos, *tipState, MAX_SEARCH_LENGTH_WHEN_CHECKING_DRIP_TYPE);
    if (!rootPosOpt.has_value()) {
        return fluid::Fluids::EMPTY();
    }

    // 根方块上方一格是流体位置
    BlockPos fluidPos = rootPosOpt.value().up();
    if (!world.isWithinWorldBounds(fluidPos)) {
        return fluid::Fluids::EMPTY();
    }

    const BlockState* blockAboveRoot = world.getBlockState(fluidPos);
    if (blockAboveRoot == nullptr) {
        return fluid::Fluids::EMPTY();
    }

    // 特殊情况：泥巴上方产生水（在下界水蒸发，泥巴不产生水滴）
    if (blockAboveRoot->is(block_registry::MudBlocks::MUD) && !world.isUltraWarm()) {
        return fluid::Fluids::WATER();
    }

    // 检查流体状态
    const fluid::FluidState* fluidState = world.getFluidState(fluidPos);
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        const fluid::Fluid& fluid = fluidState->getFluid();
        // 只接受水和岩浆
        if (fluid.isIn(fluid::FluidTags::WATER()) || fluid.isIn(fluid::FluidTags::LAVA())) {
            return &fluid;
        }
    }

    return fluid::Fluids::EMPTY();
}

bool PointedDripstoneBlock::canDripThrough(IWorld& world, const BlockPos& pos, const BlockState* state)
{
    if (state == nullptr || state->isAir()) {
        return true;
    }
    // 实心方块不可穿透
    if (state->isOpaque()) {
        return false;
    }
    // 有流体不可穿透
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        return false;
    }
    // 检查碰撞箱是否与 4x16x4 中心柱相交
    // 即方块本地坐标 (0.375, 0, 0.375)-(0.625, 1, 0.625)
    static const AxisAlignedBB DRIP_SPACE_LOCAL(0.375f, 0.0f, 0.375f, 0.625f, 1.0f, 0.625f);
    const CollisionShape& shape = state->getCollisionShape();
    if (shape.isFullBlock()) {
        return false;
    }
    if (shape.isEmpty()) {
        return true;
    }
    // 检查方块碰撞箱是否与滴水通道区域重叠
    for (const auto& box : shape.boxes()) {
        if (DRIP_SPACE_LOCAL.intersects(box)) {
            return false;
        }
    }
    return true;
}

std::optional<BlockPos> PointedDripstoneBlock::findRootBlock(
    IWorld& world, const BlockPos& pos, const BlockState& state, i32 maxDistance)
{
    if (!isStalactite(state)) {
        return std::nullopt;
    }

    // 沿反方向（向上）搜索，找到不是滴石的方块
    BlockPosMutable mutablePos(pos.x, pos.y, pos.z);
    const BlockState* currentState = &state;

    for (i32 i = 0; i < maxDistance; i++) {
        if (!isPointedDripstoneWithDirection(currentState, Direction::Down)) {
            // 找到根方块
            return mutablePos.toImmutable();
        }
        mutablePos.move(Direction::Up);
        if (!world.isWithinWorldBounds(mutablePos.toImmutable())) {
            break;
        }
        currentState = world.getBlockState(mutablePos.toImmutable());
        if (currentState == nullptr) {
            break;
        }
    }

    return std::nullopt;
}

void PointedDripstoneBlock::createDripstone(
    IWorld& world, const BlockPos& pos, Direction direction, BlockStateProperties::DripstoneThickness thickness)
{
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);
    const BlockState* newState = &VanillaBlocks::POINTED_DRIPSTONE->defaultState()
                                      .with(BlockStateProperties::VERTICAL_DIRECTION(), direction)
                                      .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness)
                                      .with(BlockStateProperties::WATERLOGGED(), waterlogged);
    world.setBlockState(pos, newState, 3);
}

void PointedDripstoneBlock::createMergedTips(IWorld& world, const BlockPos& pos, const BlockState& upState)
{
    // 当两个TIP对接时：上方设为 TIP_MERGE + DOWN，下方设为 TIP_MERGE + UP
    BlockPos downPos = pos;
    BlockPos upPos = pos.up();

    // 如果传入的是朝上的TIP，则上方在 pos 之上，下方在 pos
    // 如果传入的是朝下的TIP，则下方在 pos，上方在 pos 之上
    // 如果 upState 朝上，则 upPos=pos, downPos=pos.below()
    if (upState.get(BlockStateProperties::VERTICAL_DIRECTION()) == Direction::Up) {
        upPos = pos;
        downPos = pos.down();
    }

    createDripstone(world, downPos, Direction::Down, BlockStateProperties::DripstoneThickness::TipMerge);
    createDripstone(world, upPos, Direction::Up, BlockStateProperties::DripstoneThickness::TipMerge);
}

void PointedDripstoneBlock::grow(IWorld& world, const BlockPos& tipPos, Direction direction)
{
    BlockPos growPos = tipPos.offset(direction);
    const BlockState* growState = world.getBlockState(growPos);

    // 如果生长方向遇到对向的未合并TIP → 合并
    if (isUnmergedTipWithDirection(growState, Directions::opposite(direction))) {
        createMergedTips(world, growPos, *growState);
    } else if (growState == nullptr || growState->isAir()) {
        // 空气位置 → 创建新的TIP
        createDripstone(world, growPos, direction, BlockStateProperties::DripstoneThickness::Tip);
    }
    // 如果是含水空气位置也可以创建
    else {
        const fluid::FluidState* fluidState = world.getFluidState(growPos);
        if (fluidState != nullptr && fluidState->isSource() && fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
            // 水源位置 → 创建含水的TIP
            const BlockState* newState =
                &VanillaBlocks::POINTED_DRIPSTONE->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), direction)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
                     .with(BlockStateProperties::WATERLOGGED(), true);
            world.setBlockState(growPos, newState, 3);
        }
    }
}

void PointedDripstoneBlock::growStalagmiteBelow(IWorld& world, const BlockPos& tipPos)
{
    BlockPosMutable mutablePos(tipPos.x, tipPos.y, tipPos.z);

    for (i32 i = 0; i < MAX_STALAGMITE_SEARCH_RANGE_WHEN_GROWING; i++) {
        mutablePos.move(Direction::Down);
        const BlockState* state = world.getBlockState(mutablePos.toImmutable());

        // 遇到流体，停止
        if (state != nullptr) {
            const fluid::FluidState* fluidState = world.getFluidState(mutablePos.toImmutable());
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return;
            }
        }

        // 遇到朝上的未合并TIP且可生长 → 继续生长该石笋
        if (isUnmergedTipWithDirection(state, Direction::Up) && canTipGrow(*state, world, mutablePos.toImmutable())) {
            grow(world, mutablePos.toImmutable(), Direction::Up);
            return;
        }

        // 检查该位置是否可以放置朝上的滴石，且下方不是水
        BlockPos belowPos = mutablePos.toImmutable().down();
        if (isValidPointedDripstonePlacement(world, mutablePos.toImmutable(), Direction::Up) &&
            !world.isWaterAt(belowPos)) {
            // 在该位置的下方一格创建新石笋
            grow(world, belowPos, Direction::Up);
            return;
        }

        // 不能穿过，停止搜索
        if (!canDripThrough(world, mutablePos.toImmutable(), state)) {
            return;
        }
    }
}

void PointedDripstoneBlock::growStalactiteOrStalagmiteIfPossible(
    const BlockState& state, IWorld& world, const BlockPos& pos, math::IRandom& random)
{
    // 检查生长条件：上方1格是滴水石块，上方2格是水源
    if (!canGrow(world, pos)) {
        return;
    }

    // 沿钟乳石方向寻找尖端
    std::optional<BlockPos> tipPosOpt = findTip(state, world, pos, MAX_GROWTH_LENGTH, false);
    if (!tipPosOpt.has_value()) {
        return; // 未找到有效尖端
    }

    BlockPos tipPos = tipPosOpt.value();
    const BlockState* tipState = world.getBlockState(tipPos);
    if (tipState == nullptr || !canDrip(*tipState) || !canTipGrow(*tipState, world, tipPos)) {
        return;
    }

    // 50%概率向下生长钟乳石，50%概率尝试生成石笋
    if (random.nextBoolean()) {
        grow(world, tipPos, Direction::Down);
    } else {
        growStalagmiteBelow(world, tipPos);
    }
}

void PointedDripstoneBlock::maybeTransferFluid(const BlockState& state, IWorld& world, const BlockPos& pos, f32 chance)
{
    // 必须是钟乳石起点位置
    if (!isStalactiteStartPos(state, world, pos)) {
        return;
    }

    // 寻找上方流体
    std::optional<BlockPos> rootPosOpt = findRootBlock(world, pos, state, MAX_SEARCH_LENGTH_WHEN_CHECKING_DRIP_TYPE);
    if (!rootPosOpt.has_value()) {
        return; // 未找到根方块
    }

    BlockPos rootPos = rootPosOpt.value();
    BlockPos fluidPos = rootPos.up();
    const BlockState* fluidBlockState = world.getBlockState(fluidPos);
    if (fluidBlockState == nullptr) {
        return;
    }

    // 确定流体类型
    // 特殊情况：泥巴上方产生水（在下界水蒸发，泥巴不产生水滴）
    bool isMudWithWater = fluidBlockState->is(VanillaBlocks::MUD) && !world.isUltraWarm();
    const fluid::Fluid* fluid = nullptr;

    if (isMudWithWater) {
        fluid = fluid::Fluids::WATER();
    } else {
        const fluid::FluidState* fluidState = world.getFluidState(fluidPos);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            fluid = &fluidState->getFluid();
        }
    }

    if (fluid == nullptr) {
        return;
    }

    bool isWater = fluid->isIn(fluid::FluidTags::WATER());
    bool isLava = fluid->isIn(fluid::FluidTags::LAVA());

    if (!isWater && !isLava) {
        return;
    }

    // 概率检查
    f32 threshold = isWater ? WATER_TRANSFER_PROBABILITY_PER_RANDOM_TICK : LAVA_TRANSFER_PROBABILITY_PER_RANDOM_TICK;
    if (chance >= threshold) {
        return;
    }

    // 寻找尖端
    std::optional<BlockPos> tipPosOpt2 = findTip(state, world, pos, MAX_SEARCH_LENGTH_WHEN_CHECKING_DRIP_TYPE, false);
    if (!tipPosOpt2.has_value()) {
        return; // 未找到有效尖端
    }
    BlockPos tipPos = tipPosOpt2.value();

    // 泥巴变粘土逻辑
    // 如果根方块上方是泥巴且流体为水，将泥巴替换为粘土
    if (isMudWithWater) {
        const BlockState* clayState = &VanillaBlocks::CLAY->defaultState();
        world.setBlockState(fluidPos, clayState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, fluidPos, clayState);
        world.playEvent(world::WorldEvents::DRIPSTONE_DRIP, tipPos, 0);
        return;
    }

    // 寻找下方可接收流体的炼药锅
    std::optional<BlockPos> cauldronPosOpt = findFillableCauldronBelow(world, tipPos, *fluid);
    if (cauldronPosOpt.has_value()) {
        BlockPos cauldronPos = cauldronPosOpt.value();
        world.playEvent(world::WorldEvents::DRIPSTONE_DRIP, tipPos, 0);
        // 计算延迟：50 + (尖端Y - 炼药锅Y) tick
        i32 delay = 50 + (tipPos.y - cauldronPos.y);
        const BlockState* cauldronState = world.getBlockState(cauldronPos);
        if (cauldronState != nullptr) {
            world.tickManager().scheduleBlockTick(cauldronPos, cauldronState->getBlockMutable(), delay);
        }
    }
}

void PointedDripstoneBlock::_spawnFallingStalactite(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 从当前位置开始向下遍历，逐个生成掉落方块实体
    // 只有最底部的尖端（TIP 或 TIP_MERGE）会设置伤害参数
    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState == nullptr) {
        return;
    }

    BlockPosMutable mutablePos(pos.x, pos.y, pos.z);
    const BlockState* currentState = &state;

    while (currentState != nullptr && isStalactite(*currentState)) {
        BlockPos currentPos = mutablePos.toImmutable();

        // 将当前位置替换为空气（含水则替换为流体方块）
        const fluid::FluidState* fluidState = world.getFluidState(currentPos);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            // 含水滴石掉落时替换为流体对应的方块状态
            const BlockState* fluidBlockState = fluidState->getBlockState();
            if (fluidBlockState != nullptr) {
                world.setBlockState(currentPos, fluidBlockState, 3);
            } else {
                world.setBlockState(currentPos, airState, 3);
            }
        } else {
            world.setBlockState(currentPos, airState, 3);
        }

        // 创建掉落方块实体
        auto fallingEntity = std::make_unique<entity::FallingBlockEntity>();
        fallingEntity->setTypeId(entity::EntityTypeKeys::FALLING_BLOCK);
        fallingEntity->setPosition(static_cast<f32>(currentPos.x) + 0.5f,
            static_cast<f32>(currentPos.y),
            static_cast<f32>(currentPos.z) + 0.5f);
        fallingEntity->setVelocity(0.0f, 0.0f, 0.0f);
        fallingEntity->setBlockId(currentState->blockId());
        fallingEntity->setFallingState(currentState);
        fallingEntity->setFallStartPos(static_cast<f64>(currentPos.y));

        // 只有尖端（TIP 或 TIP_MERGE）设置伤害
        // 伤害 = max(高度差, 6) * 每格伤害系数，上限 MAX_DAMAGE
        if (isTip(currentState, true)) {
            i32 fallHeight = std::max(pos.y - currentPos.y + 1, 6);
            f32 damagePerDist = FALLING_STALACTITE_FALL_DAMAGE_PER_DISTANCE * static_cast<f32>(fallHeight);
            fallingEntity->setHurtEntities(true);
            fallingEntity->setFallDamagePerDistance(damagePerDist);
            fallingEntity->setFallDamageMax(FALLING_STALACTITE_MAX_DAMAGE);
            fallingEntity->setFallDamageType(DamageType::FallingStalactite);
        }

        world.spawnEntity(std::move(fallingEntity));

        // 向下移动
        mutablePos.move(Direction::Down);
        currentState = world.getBlockState(mutablePos.toImmutable());
    }
}

// ============================================================================
// getDripParticlePosition
// ============================================================================

Vector3 PointedDripstoneBlock::getDripParticlePosition(const BlockPos& pos)
{
    // Y = blockPos.y + STALACTITE_DRIP_START_PIXEL - 0.0625
    //   = blockPos.y + 5/16 - 1/16 = blockPos.y + 0.25
    // X = blockPos.x + 0.5（居中）
    // Z = blockPos.z + 0.5
    f64 particleX = static_cast<f64>(pos.x) + 0.5;
    f64 particleY = static_cast<f64>(pos.y) + STALACTITE_DRIP_START_PIXEL - 0.0625;
    f64 particleZ = static_cast<f64>(pos.z) + 0.5;

    return Vector3(static_cast<f32>(particleX), static_cast<f32>(particleY), static_cast<f32>(particleZ));
}

} // namespace blocks
} // namespace mc
