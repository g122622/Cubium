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

#include "BubbleColumnBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

BubbleColumnBlock::BubbleColumnBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DRAG())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::DRAG(), false));
}

// ========== 静态方法实现 ==========

void BubbleColumnBlock::placeBubbleColumn(IWorld& world, const BlockPos& pos, bool drag)
{
    if (canHoldBubbleColumn(world, pos)) {
        const BlockState& bubbleState =
            VanillaBlocks::BUBBLE_COLUMN->defaultState().with(BlockStateProperties::DRAG(), drag);
        world.setBlockState(pos, &bubbleState, 2);
    }
}

bool BubbleColumnBlock::canHoldBubbleColumn(const IWorld& world, const BlockPos& pos)
{
    // 条件：是水方块 + 是水源方块
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 必须是水方块
    if (!state->is(VanillaBlocks::WATER)) {
        return false;
    }

    // 检查流体状态
    const fluid::FluidState* fluidState = state->getFluidState();
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 流体是水源（源头的等级固定为 SOURCE_LEVEL）
    return fluidState->isSource();
}

bool BubbleColumnBlock::getDrag(const IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);

    if (state == nullptr) {
        return true; // 默认下拖
    }

    // 如果下方是气泡柱，继承其 DRAG 状态
    if (VanillaBlocks::BUBBLE_COLUMN != nullptr && state->is(VanillaBlocks::BUBBLE_COLUMN)) {
        return state->get(BlockStateProperties::DRAG());
    }

    // 如果下方是灵魂沙，返回 false（上推）
    if (VanillaBlocks::SOUL_SAND != nullptr && state->is(VanillaBlocks::SOUL_SAND)) {
        return false;
    }

    // 其他情况（包括岩浆块）返回 true（下拖）
    return true;
}

bool BubbleColumnBlock::isDrag(const BlockState& state) const
{
    return state.get(BlockStateProperties::DRAG());
}

void BubbleColumnBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 气泡柱被添加时，在上方放置气泡柱
    // DRAG 状态由下方方块决定
    bool drag = getDrag(world, pos.down());
    placeBubbleColumn(world, pos.up(), drag);
}

BlockState BubbleColumnBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据下方方块决定是否为下拖
    bool drag = _checkSource(world, pos);

    return defaultState().with(BlockStateProperties::DRAG(), drag);
}

bool BubbleColumnBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 气泡柱只能出现在水中（或已有气泡柱位置）
    const BlockState* currentState = world.getBlockState(pos);
    const bool isWater =
        currentState != nullptr && VanillaBlocks::WATER != nullptr && currentState->is(VanillaBlocks::WATER);
    const bool isBubbleColumn = currentState != nullptr && currentState->is(this);
    if (!isWater && !isBubbleColumn) {
        return false;
    }

    // 检查下方是否是气泡源或另一段气泡柱
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }

    if (VanillaBlocks::MAGMA != nullptr && belowState->is(VanillaBlocks::MAGMA)) {
        return true;
    }
    if (VanillaBlocks::SOUL_SAND != nullptr && belowState->is(VanillaBlocks::SOUL_SAND)) {
        return true;
    }
    return belowState->is(this);
}

BlockState BubbleColumnBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 检查位置有效性
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        // 位置无效，变成水
        if (VanillaBlocks::WATER != nullptr) {
            return VanillaBlocks::WATER->defaultState();
        }
        return state;
    }

    if (facing == Direction::Down) {
        // 下方方块变化，更新 DRAG 状态
        bool newDrag = getDrag(world, facingPos);
        if (newDrag != isDrag(state)) {
            return state.with(BlockStateProperties::DRAG(), newDrag);
        }
    }

    if (facing == Direction::Up) {
        // 上方方块变化
        if (!facingState.is(this) && canHoldBubbleColumn(world, facingPos)) {
            // 上方是水（非气泡柱），调度 tick 传播气泡柱
            world.tickManager().scheduleBlockTick(currentPos, *this, 5);
        }
    }

    return state;
}

void BubbleColumnBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(&world);
    MC_UNUSED(&pos);

    // 气泡柱推动实体
    // 上推速度: 0.1 (灵魂沙)
    // 下拖速度: 0.03 (岩浆块，实际是 -0.03)
    if (isDrag(state)) {
        // 下拖：向下推动实体（岩浆块产生）
        entity.addVelocity(0.0, -0.03, 0.0);
    } else {
        // 上推：向上推动实体（灵魂沙产生）
        entity.addVelocity(0.0, 0.1, 0.0);
    }

    // 重置摔落距离
    entity.setFallDistance(0.0f);
}

void BubbleColumnBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 气泡柱传播逻辑：在上方位置放置气泡柱，继承当前 DRAG 状态
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    bool currentDrag = isDrag(state);

    placeBubbleColumn(world, abovePos, currentDrag);
}

const CollisionShape& BubbleColumnBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& BubbleColumnBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool BubbleColumnBlock::_checkSource(const IWorld& world, const BlockPos& pos) const
{
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 岩浆块产生下拖气泡柱
    if (VanillaBlocks::MAGMA != nullptr && belowState->is(VanillaBlocks::MAGMA)) {
        return true;
    }

    // 灵魂沙产生上升气泡柱
    if (VanillaBlocks::SOUL_SAND != nullptr && belowState->is(VanillaBlocks::SOUL_SAND)) {
        return false;
    }

    // 继承下方气泡柱的拖拽方向
    if (belowState->is(this)) {
        return isDrag(*belowState);
    }

    return false;
}

void BubbleColumnBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    const f32 centerX = static_cast<f32>(pos.x) + 0.5f;
    const f32 centerY = static_cast<f32>(pos.y);
    const f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

    if (isDrag(state)) {
        // 下拖模式（岩浆块产生）：生成向下水流粒子
        context.addAnimateParticle(particle::ParticleTypeId::CurrentDown,
            Vector3(centerX, centerY + 0.8f, static_cast<f32>(pos.z)),
            Vector3(0.0f, 0.0f, 0.0f));

        // 1/200 概率播放漩涡环境音
        if (random.nextInt(200) == 0) {
            context.playLocalSound(SoundEvents::BLOCK_BUBBLE_COLUMN_WHIRLPOOL_AMBIENT,
                sound::SoundCategory::Blocks,
                Vector3(centerX, centerY, centerZ),
                0.2f + random.nextFloat() * 0.2f,
                0.9f + random.nextFloat() * 0.15f);
        }
    } else {
        // 上推模式（灵魂沙产生）：生成向上气泡粒子
        context.addAnimateParticle(
            particle::ParticleTypeId::BubbleColumnUp, Vector3(centerX, centerY, centerZ), Vector3(0.0f, 0.04f, 0.0f));

        context.addAnimateParticle(particle::ParticleTypeId::BubbleColumnUp,
            Vector3(static_cast<f32>(pos.x) + random.nextFloat(),
                centerY + random.nextFloat(),
                static_cast<f32>(pos.z) + random.nextFloat()),
            Vector3(0.0f, 0.04f, 0.0f));

        // 1/200 概率播放上升环境音
        if (random.nextInt(200) == 0) {
            context.playLocalSound(SoundEvents::BLOCK_BUBBLE_COLUMN_UPWARDS_AMBIENT,
                sound::SoundCategory::Blocks,
                Vector3(centerX, centerY, centerZ),
                0.2f + random.nextFloat() * 0.2f,
                0.9f + random.nextFloat() * 0.15f);
        }
    }
}

} // namespace blocks
} // namespace mc
