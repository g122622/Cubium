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

#include "SnowyDirtBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/property/StateContainer.hpp"
#include "../../../IWorld.hpp"
#include "../../registry/VanillaBlocks.hpp"
#include "../ice/SnowBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::blocks {

// ============================================================================
// SnowyDirtBlock 实现
// ============================================================================

SnowyDirtBlock::SnowyDirtBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 创建状态容器，添加 SNOWY 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(SNOWY()).create(
        [this](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 设置默认状态：无雪
    setDefaultState(defaultState().with(SNOWY(), false));
}

BlockState SnowyDirtBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 检查放置位置上方是否有雪块或雪层
    const IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    // 检查 SNOW_BLOCK 或 SNOW（任意层数）
    const bool hasSnow =
        aboveState != nullptr && (aboveState->is(VanillaBlocks::SNOW_BLOCK) || aboveState->is(VanillaBlocks::SNOW));

    return defaultState().with(SNOWY(), hasSnow);
}

BlockState SnowyDirtBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 只有上方方块变化时才更新 SNOWY 状态
    if (facing == Direction::Up) {
        // 检查上方是否为雪块或雪层
        const bool hasSnow = facingState.is(VanillaBlocks::SNOW_BLOCK) || facingState.is(VanillaBlocks::SNOW);
        return state.with(SNOWY(), hasSnow);
    }

    return state;
}

} // namespace mc::blocks
