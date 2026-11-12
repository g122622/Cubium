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

#include "common/world/block/blocks/sculk/SculkShriekerBlock.hpp"

#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include "entity/core/Entity.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/Direction.hpp"
#include "util/math/MathUtils.hpp"
#include "world/IWorld.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/gameevent/GameEvents.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <memory>

namespace mc {
namespace blocks {
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
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

const CollisionShape& SculkShriekerBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // vanilla SculkShriekerBlock 只把碰撞形状与遮挡形状覆盖成 SHAPE_COLLISION（8 像素高的台子），
    // getShape 仍继承 BaseEntityBlock 的整格立方体。
    return m_shape;
}

const CollisionShape& SculkShriekerBlock::getOcclusionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& SculkShriekerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // vanilla 未重写 getShape：仍是整格立方体。这一点直接决定光照——propagatesSkylightDown
    // 的默认公式按形状判「整格 → 不透传」。早期把 getShape 也改成 8 像素台子，幽匿尖啸体
    // 因此被误判为天空光直通格（lightBlock=0），原版是 1。
    static const CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

bool SculkShriekerBlock::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 按 vanilla 默认公式原样求值（形状取 getShape，而不是本项目默认公式用的 getOcclusionShape）：
    // getShape 是整格 → false；含水时同样不通透。
    if (getShape(state).coversFullBlock()) {
        return false;
    }
    const fluid::FluidState* fluidState = getFluidState(state);
    return fluidState == nullptr || fluidState->isEmpty();
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
        world.setBlockState(pos, newState, world::BlockUpdateFlags::UPDATE_ALL);

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

void SculkShriekerBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
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

    Block::onBlockRemoved(world, pos, state, movedByPiston);
}

void SculkShriekerBlock::shriek(IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* sourceEntity)
{
    // 1. 设置 SHRIEKING 方块状态为 true
    const BlockState* newState = &state.with(BlockStateProperties::SHRIEKING(), true);
    world.setBlockState(pos, newState, world::BlockUpdateFlags::UPDATE_ALL);

    // 2. 调度 tick：SHRIEKING_TICKS 后触发状态转换回 false
    world.tickManager().scheduleBlockTick(pos, state.getBlock(), SHRIEKING_TICKS);

    // 3. 发出尖啸粒子效果（事件 ID 3007）
    world.playEvent(::mc::world::LevelEventId::PARTICLES_SCULK_SHRIEK, pos, 0);

    // 4. 发出 SHRIEK 游戏事件（通知附近的幽匿感测体/尖啸体振动系统）
    const BlockState* currentState = world.getBlockState(pos);
    world.gameEvent(gameevent::GameEvents::SHRIEK,
        pos,
        gameevent::GameEvent::Context(sourceEntity, currentState != nullptr ? currentState : &state));
}

} // namespace blocks
} // namespace mc
