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

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/CauldronBlock.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

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

    // MC 1.21.11: isSecondaryUseActive = 玩家正在潜行
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
            // 使用 Block 的 spawnAfterBreak 掉落物品
            spawnAfterBreak(world, pos, state, nullptr, false);
            world.setBlockState(pos, airState, 3);
        }
    } else if (isStalactite(state)) {
        // 钟乳石失去支撑：整个钟乳石柱掉落
        // TODO: 当 FallingBlockEntity 完善后，应逐个生成掉落方块实体
        // 钟乳石掉落时应对下方实体造成伤害：伤害 = max(高度差, 6) * 1.0，上限 40
        BlockPosMutable mutablePos(pos.x, pos.y, pos.z);
        const BlockState* currentState = world.getBlockState(pos);
        while (currentState != nullptr && isStalactite(*currentState)) {
            spawnAfterBreak(world, mutablePos.toImmutable(), *currentState, nullptr, false);
            const BlockState* air = BlockRegistry::instance().airState();
            if (air != nullptr) {
                world.setBlockState(mutablePos.toImmutable(), air, 3);
            }
            mutablePos.move(Direction::Down);
            currentState = world.getBlockState(mutablePos.toImmutable());
        }
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
    if (state.get(BlockStateProperties::VERTICAL_DIRECTION()) == Direction::Up &&
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()) == BlockStateProperties::DripstoneThickness::Tip) {
        // MC: causeFallDamage(fallDistance + 2.5, 2.0, damageSources.stalagmite())
        // TODO: 当伤害系统完善后实现正确的石笋伤害倍率
        // 当前委托给基类处理
    }

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
    // MC 1.21.11: 上方1格是滴水石块 + 上方2格是水源
    const BlockState* aboveState = world.getBlockState(pos.up());
    if (aboveState == nullptr || !aboveState->is(VanillaBlocks::DRIPSTONE_BLOCK)) {
        return false;
    }

    // 检查上方2格是否为水源
    // MC: p_154142_.is(Blocks.WATER) && p_154142_.getFluidState().isSource()
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

        // MC 1.21.11: 检查是否为可接收滴水的炼药锅
        // 当前仅支持水炼药锅（CauldronBlock），岩浆炼药锅尚未实现
        if (state->is(block_registry::BuildingBlocks::CAULDRON)) {
            // 水可以滴入未满的炼药锅
            if (fluid.isIn(fluid::FluidTags::WATER()) && !CauldronBlock::isFull(*state)) {
                return mutablePos.toImmutable();
            }
            // TODO: 岩浆可以滴入岩浆炼药锅（当 LavaCauldronBlock 实现后）
        }

        // 检查是否可以穿过
        if (!canDripThrough(world, mutablePos.toImmutable(), state)) {
            break;
        }
    }

    return std::nullopt;
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
    // 其他方块检查碰撞箱
    // TODO: 完整实现需要检查碰撞箱是否与 4x16x4 中心柱相交
    // 当前简化实现：非实心非流体的方块视为可穿透
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
    // MC源码中：如果 upState.getValue(TIP_DIRECTION) == UP，则 upPos=pos, downPos=pos.below()
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
    // TODO: 泥巴变粘土的逻辑需要 Mud 方块和 Clay 方块支持，当前跳过
    const fluid::FluidState* fluidState = world.getFluidState(fluidPos);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return;
    }

    const fluid::Fluid& fluid = fluidState->getFluid();
    bool isWater = fluid.isIn(fluid::FluidTags::WATER());
    bool isLava = fluid.isIn(fluid::FluidTags::LAVA());

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

    // MC 1.21.11: 泥巴变粘土逻辑
    // 如果根方块上方是泥巴且流体为水，将泥巴替换为粘土
    // TODO: 当 Mud 方块和 Clay 方块注册后，启用此逻辑
    // if (fluidBlockState != nullptr && fluidBlockState->is(VanillaBlocks::MUD) && isWater) {
    //     const BlockState* clayState = &VanillaBlocks::CLAY->defaultState();
    //     world.setBlockState(fluidPos, clayState, 3);
    //     // TODO: 触发 GameEvent::BLOCK_CHANGE
    //     // TODO: 播放 levelEvent 1504（滴水粒子/音效）
    //     return;
    // }

    // MC 1.21.11: 寻找下方可接收流体的炼药锅
    std::optional<BlockPos> cauldronPosOpt = findFillableCauldronBelow(world, tipPos, fluid);
    if (cauldronPosOpt.has_value()) {
        BlockPos cauldronPos = cauldronPosOpt.value();
        // TODO: 播放 levelEvent 1504（尖端滴水粒子/音效）
        // 计算延迟：50 + (尖端Y - 炼药锅Y) tick
        i32 delay = 50 + (tipPos.y - cauldronPos.y);
        const BlockState* cauldronState = world.getBlockState(cauldronPos);
        if (cauldronState != nullptr) {
            world.tickManager().scheduleBlockTick(cauldronPos, const_cast<Block&>(cauldronState->getBlock()), delay);
        }
    }
}

} // namespace blocks
} // namespace mc
