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

#include "RedstoneLampBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/redstone/RedstonePower.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneLampBlock::RedstoneLampBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
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

    // 设置默认状态（熄灭）
    setDefaultState(defaultState().with(BlockStateProperties::LIT(), false));
}

bool RedstoneLampBlock::isLit(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::LIT());
}

BlockState RedstoneLampBlock::withLit(BlockState state, bool lit) noexcept
{
    return state.with(BlockStateProperties::LIT(), lit);
}

void RedstoneLampBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 检查是否应该点亮
    bool shouldLit = world::redstone::RedstonePower::isPowered(world, pos);
    if (shouldLit != isLit(state)) {
        if (shouldLit) {
            BlockState newState = withLit(state, true);
            world.setBlockState(pos, &newState, 2);
        } else {
            world.tickManager().scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
    }
}

void RedstoneLampBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查是否应该点亮
    bool shouldLit = world::redstone::RedstonePower::isPowered(world, pos);
    bool isCurrentlyLit = isLit(*state);

    if (shouldLit != isCurrentlyLit) {
        if (shouldLit) {
            // 被充能，立即点亮
            BlockState newState = withLit(*state, true);
            world.setBlockState(pos, &newState, 2);
        } else {
            // 失去信号，调度熄灭
            world.tickManager().scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
    }
}

void RedstoneLampBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 检查是否应该熄灭
    bool shouldLit = world::redstone::RedstonePower::isPowered(world, pos);
    if (!shouldLit && isLit(state)) {
        BlockState newState = withLit(state, false);
        world.setBlockState(pos, &newState, 2);
    }
}

} // namespace blocks
} // namespace mc
