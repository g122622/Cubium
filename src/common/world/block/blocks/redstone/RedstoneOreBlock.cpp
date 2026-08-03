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

#include "RedstoneOreBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneOreBlock::RedstoneOreBlock(const BlockProperties& properties)
    : Block(properties)
{
    m_ticksRandomly = true;

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LIT())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::LIT(), false));
}

void RedstoneOreBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

void RedstoneOreBlock::attack(const BlockState& state, IWorld& world, const BlockPos& pos, Player& player)
{
    MC_UNUSED(player);
    interact(world, pos, const_cast<BlockState&>(state));
}

void RedstoneOreBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(entity);
    interact(world, pos, const_cast<BlockState&>(state));
}

void RedstoneOreBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    if (state.get(BlockStateProperties::LIT())) {
        auto newState = state.with(BlockStateProperties::LIT(), false);
        world.setBlockState(pos, &newState, 3);
    }
}

void RedstoneOreBlock::interact(IWorld& world, const BlockPos& pos, BlockState& state) const
{
    if (!state.get(BlockStateProperties::LIT())) {
        auto newState = state.with(BlockStateProperties::LIT(), true);
        world.setBlockState(pos, &newState, 3);
        // 调度tick以在一段时间后熄灭
        world.tickManager().scheduleBlockTick(pos, const_cast<RedstoneOreBlock&>(*this), 30); // MC中熄灭延迟约30 ticks
    }
}

} // namespace blocks
} // namespace mc
