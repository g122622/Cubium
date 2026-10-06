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

#include "LightBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

LightBlock::LightBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 光源方块持有 LEVEL（0-15）与 WATERLOGGED 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LEVEL_0_15())
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

    // 对齐 vanilla：默认 LEVEL=15, WATERLOGGED=false
    setDefaultState(defaultState()
            .with(BlockStateProperties::LEVEL_0_15(), MAX_LEVEL)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

u8 LightBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 对齐 vanilla LightBlock.LIGHT_EMISSION = state -> state.getValue(LEVEL)
    return static_cast<u8>(state.get(BlockStateProperties::LEVEL_0_15()));
}

const CollisionShape& LightBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 光源方块默认不可见（无渲染轮廓）；持有光源方块物品时的完整轮廓由渲染层处理。
    static const CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

BlockActionResult LightBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 对齐 vanilla LightBlock#useWithoutItem：服务端且玩家具备管理员权限时循环亮度
    if (!world.isClientSide() && player.canUseGameMasterBlocks()) {
        const BlockState& next = state.cycle(BlockStateProperties::LEVEL_0_15());
        world.setBlockState(pos, &next, world::BlockUpdateFlags::UPDATE_CLIENTS);
        return ActionResultType::Success;
    }
    return ActionResultType::Consume;
}

const fluid::FluidState* LightBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

bool LightBlock::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 对齐 vanilla LightBlock#propagatesSkylightDown = state.getFluidState().isEmpty()
    const fluid::FluidState* fluidState = getFluidState(state);
    return fluidState == nullptr || fluidState->isEmpty();
}

BlockState LightBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingPos);

    // 对齐 vanilla LightBlock#updateShape：含水时调度水流体刻
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

} // namespace blocks
} // namespace mc
