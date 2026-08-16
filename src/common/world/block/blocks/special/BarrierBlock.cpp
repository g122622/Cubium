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

#include "BarrierBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ========== BarrierBlock ==========

BarrierBlock::BarrierBlock(const BlockProperties& properties)
    : Block(properties)
{
    // vanilla 1.21.11 barrier 持有 waterlogged 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), false));
}

const CollisionShape& BarrierBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

const fluid::FluidState* BarrierBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

bool BarrierBlock::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 对齐 vanilla BarrierBlock#propagatesSkylightDown = state.getFluidState().isEmpty()。
    // 屏障无水时透传天空光（opacity=0），含水时阻断（opacity=1）。
    const fluid::FluidState* fluidState = getFluidState(state);
    return fluidState == nullptr || fluidState->isEmpty();
}

} // namespace blocks
} // namespace mc
