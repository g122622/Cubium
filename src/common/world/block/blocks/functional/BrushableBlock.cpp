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

#include "common/world/block/blocks/functional/BrushableBlock.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BrushableBlockEntity.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "util/math/random/IRandom.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ============================================================================
// BrushableBlock
// ============================================================================

BrushableBlock::BrushableBlock(const BlockProperties& properties,
    const Block* turnsInto,
    ResourceLocation brushSound,
    ResourceLocation brushCompletedSound)
    : FallingBlock(properties)
    , m_turnsInto(turnsInto)
    , m_brushSound(std::move(brushSound))
    , m_brushCompletedSound(std::move(brushCompletedSound))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DUSTED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::DUSTED(), 0));
}

void BrushableBlock::onBroken(IWorld& world, const BlockPos& /*pos*/, entity::FallingBlockEntity& entity)
{
    // 原版 onBrokenAfterFall：以实体的包围盒中心为准（方块实体可能落在格子里偏一点），
    // 先放破碎粒子，再发 block_destroy。
    const Vector3 center = entity.position();
    const BlockPos effectPos(static_cast<i32>(std::floor(center.x)),
        static_cast<i32>(std::floor(center.y)),
        static_cast<i32>(std::floor(center.z)));

    world.playEvent(::mc::world::LevelEventId::PARTICLES_DESTROY_BLOCK, effectPos, 0);
    // Cubium 的 gameEvent 只接受方块位置；原版这里传的是 Vec3 中心点，
    // 在整数格坐标上与方块位置等价（周围一圈格子的振动半径远大于 1 格）。
    const BlockState* fallingState = entity.getFallingState();
    world.gameEvent(
        gameevent::GameEvents::BLOCK_DESTROY, effectPos, fallingState != nullptr ? fallingState : &defaultState());
}

void BrushableBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

std::unique_ptr<BlockEntity> BrushableBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BrushableBlockEntity>(pos);
}

void BrushableBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对齐 MC 1.21.11 BrushableBlock.tick：
    // 1. 获取 BrushableBlockEntity 并调用 checkReset()
    // 2. 执行 FallingBlock 的下落检测逻辑

    // 步骤 1：调用 checkReset
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::BrushableBlock) {
        auto* brushableEntity = static_cast<blockentity::BrushableBlockEntity*>(blockEntity);
        brushableEntity->checkReset(world);
    }

    // 步骤 2：委托基类执行下落检测
    FallingBlock::tick(world, pos, state, random);
}

} // namespace blocks
} // namespace mc
