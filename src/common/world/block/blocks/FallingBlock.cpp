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

#include "FallingBlock.hpp"

#include "common/core/Constants.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

FallingBlock::FallingBlock(const BlockProperties& properties)
    : Block(properties)
{}

void FallingBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    world.tickManager().scheduleBlockTick(pos, *this, getFallDelay(), world::tick::TickPriority::Normal);
}

void FallingBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);
    world.tickManager().scheduleBlockTick(pos, *this, getFallDelay(), world::tick::TickPriority::Normal);
}

BlockState FallingBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 当邻居更新时也调度 tick
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    world.tickManager().scheduleBlockTick(currentPos, *this, getFallDelay(), world::tick::TickPriority::Normal);
    return state;
}

void FallingBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    if (pos.y <= world::MIN_BUILD_HEIGHT) {
        return;
    }

    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || currentState->isAir() || !currentState->is(this)) {
        return;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!canFallThrough(belowState)) {
        return;
    }

    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState == nullptr) {
        return;
    }

    if (!world.setBlockState(pos, airState, 3)) {
        return;
    }

    auto fallingEntity = std::make_unique<entity::FallingBlockEntity>();
    fallingEntity->setTypeId(entity::EntityTypeKeys::FALLING_BLOCK);
    fallingEntity->setPosition(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
    fallingEntity->setVelocity(0.0f, 0.0f, 0.0f);
    fallingEntity->setBlockId(currentState->blockId());
    fallingEntity->setFallingState(currentState);
    fallingEntity->setFallStartPos(static_cast<f64>(pos.y));

    // 调用开始下落回调
    onStartFalling(world, pos, *fallingEntity);

    const EntityInstanceId entityId = world.spawnEntity(std::move(fallingEntity));
    if (entityId == 0) {
        world.setBlockState(pos, currentState, 3);
    }
}

// 检查方块状态是否可穿透
// 对齐 MC 1.21.11 FallingBlock.isFree()
bool FallingBlock::canFallThrough(const BlockState* state)
{
    if (state == nullptr) {
        return true;
    }

    // 空气
    if (state->isAir()) {
        return true;
    }

    // 火焰标签方块
    if (BlockTags::FIRE().contains(*state)) {
        return true;
    }

    // 液体方块
    if (state->isLiquid()) {
        return true;
    }

    // 可替换方块（花草、雪层等）
    return state->canBeReplaced();
}

} // namespace blocks
} // namespace mc
