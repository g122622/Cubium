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
 * The above copyright notice and this permission shall be included in all
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

#include "DirtPathBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/FenceGateBlock.hpp"
#include "common/world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

DirtPathBlock::DirtPathBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 15.0f / 16.0f, 1.0f))
{}

// ========== 放置和更新 ==========

BlockState DirtPathBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 目标位置上方为固体方块时土径无法存活，此时直接改放泥土（而非放弃放置）。
    // pushEntitiesUp 会把嵌入新增碰撞形状（15/16 格高 → 完整方块）的实体向上推出。
    IBlockReader& blockReader = static_cast<IBlockReader&>(context.getWorld());
    if (!isValidPosition(defaultState(), blockReader, context.placementPos())) {
        return Block::pushEntitiesUp(
            defaultState(), VanillaBlocks::DIRT->defaultState(), context.getWorld(), context.placementPos());
    }

    return defaultState();
}

bool DirtPathBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 上方为空气或非固体方块时土径可存活；栅栏门例外——栅栏门虽非固体，
    // 但关闭时其碰撞箱横跨整格，允许土径保留在栅栏门下方。
    const BlockState* aboveState = world.getBlockState(pos.up());
    if (aboveState == nullptr) {
        return true;
    }

    return !aboveState->isSolid() || dynamic_cast<const FenceGateBlock*>(&aboveState->getBlock()) != nullptr;
}

BlockState DirtPathBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 仅上方方块变化时才可能影响存活。不满足存活条件时安排 1 tick 后的计划刻转变泥土，
    // 而非同步替换——与耕地退化保持同一时序。
    if (facing == Direction::Up && !isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        world.tickManager().scheduleBlockTick(currentPos, *this, 1);
    }

    return state;
}

void DirtPathBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    if (!isValidPosition(state, static_cast<IBlockReader&>(world), pos)) {
        // 与耕地退化共用同一实现（内部经 pushEntitiesUp 推出实体后写入泥土）。
        FarmlandBlock::turnToDirt(nullptr, world, pos, state);
    }
}

// ========== 形状 ==========

const CollisionShape& DirtPathBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& DirtPathBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool DirtPathBlock::useShapeForLightOcclusion(const BlockState& state) const
{
    MC_UNUSED(state);
    // 土径高度不足一整格，须以形状而非整格判定光照遮挡
    return true;
}

bool DirtPathBlock::allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return false;
}

} // namespace blocks
} // namespace mc
