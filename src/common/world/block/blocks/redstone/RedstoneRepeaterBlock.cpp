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

#include "RedstoneRepeaterBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/RedstoneDiodeBlock.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneRepeaterBlock::RedstoneRepeaterBlock(const BlockProperties& properties)
    : RedstoneDiodeBlock("redstone_repeater", properties)
{

    // 创建状态容器 - 包含基类的 FACING 和 POWERED，以及中继器特有的 DELAY 和 LOCKED
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::DELAY_1_4())
            .add(BlockStateProperties::LOCKED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::DELAY_1_4(), 1)
            .with(BlockStateProperties::LOCKED(), false));
}

BlockState RedstoneRepeaterBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 如果更新方向不是中继器的朝向方向，更新 LOCKED 状态
    Direction blockFacing = getFacing(state);
    if (Directions::getAxis(facing) != Directions::getAxis(blockFacing)) {
        // 检查是否被锁定
        bool locked = isLocked(world, currentPos, state);
        if (isLockedState(state) != locked) {
            return withLocked(state, locked);
        }
    }

    // 调用基类实现
    return RedstoneDiodeBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

i32 RedstoneRepeaterBlock::getDelay(const BlockState& state) const
{
    return getDelaySetting(state) * DELAY_MULTIPLIER;
}

i32 RedstoneRepeaterBlock::getDelaySetting(const BlockState& state)
{
    return state.get(BlockStateProperties::DELAY_1_4());
}

BlockState RedstoneRepeaterBlock::withDelay(BlockState state, i32 delay)
{
    return state.with(BlockStateProperties::DELAY_1_4(), std::clamp(delay, MIN_DELAY, MAX_DELAY));
}

bool RedstoneRepeaterBlock::isLockedState(const BlockState& state)
{
    return state.get(BlockStateProperties::LOCKED());
}

BlockState RedstoneRepeaterBlock::withLocked(BlockState state, bool locked)
{
    return state.with(BlockStateProperties::LOCKED(), locked);
}

bool RedstoneRepeaterBlock::shouldBePowered(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 如果被锁定，保持当前状态
    if (isLockedState(state)) {
        return isPowered(state);
    }
    // 获取输入信号
    return getInputSignal(world, pos, state) > 0;
}

bool RedstoneRepeaterBlock::isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // 检查侧面是否有来自其他二极管的信号
    return getPowerOnSides(world, pos, state) > 0;
}

BlockActionResult RedstoneRepeaterBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 冒险/旁观模式下无建造权限时，禁止切换中继器延迟
    if (!player.mayBuild()) {
        return ActionResultType::Pass;
    }

    // 右键点击中继器可以在 1-4 档延迟之间循环切换
    // 只有未被锁定的中继器才能调整延迟
    if (isLockedState(state)) {
        return ActionResultType::Pass;
    }

    // 获取当前延迟档位并循环切换
    i32 currentDelay = getDelaySetting(state);
    i32 newDelay = (currentDelay % MAX_DELAY) + 1; // 1 -> 2 -> 3 -> 4 -> 1

    // 设置新的延迟档位
    BlockState newState = withDelay(state, newDelay);
    world.setBlockState(pos, &newState, 3);

    // 播放点击音效
    world.playSound(
        ResourceLocation("minecraft:block.repeater.click"), sound::SoundCategory::Blocks, pos.center(), 0.3f, 0.5f);

    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
