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

#include "EndRodBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

EndRodBlock::EndRodBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器：FACING（6 向）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认朝向 UP（对齐 MC EndRodBlock.registerDefaultState(FACING, UP)）
    setDefaultState(defaultState().with(BlockStateProperties::FACING(), Direction::Up));
}

void EndRodBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
    // 状态容器已在构造函数中通过 Builder 创建
}

BlockState EndRodBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 MC 1.21.11 EndRodBlock.getStateForPlacement：
    //   Direction direction = ctx.getClickedFace();
    //   BlockState neighbor = level.getBlockState(ctx.getClickedPos().relative(direction.getOpposite()));
    //   return neighbor.is(this) && neighbor.getValue(FACING) == direction
    //       ? defaultState().setValue(FACING, direction.getOpposite())
    //       : defaultState().setValue(FACING, direction);
    //
    // 关键语义：vanilla 的 ctx.getClickedPos() 是 BlockPlaceContext 重写版，返回「放置目标格」
    // （replaceClicked ? clickedPos : clickedPos.relative(clickedFace)），而非 UseOnContext 父类返回的
    // 「被点击现有方块」。Cubium 中与之等价的是 context.placementPos()。曾误用 context.blockPos()
    // （被点击方块），导致「背靠背反向放置」判定查错邻居——点击已有 end_rod 顶面放新块时，本应取
    // 放置目标格再反向回指被点击的现有 end_rod，却误取被点击方块再反向回指其下方格子，条件不成立
    // 而 fallback 为正向朝向，与 vanilla 不符。改为 placementPos() 后与 vanilla 一致。
    const Direction direction = context.getClickedFace();
    const IWorld& world = context.getWorld();
    const BlockPos neighborPos = context.placementPos().offset(Directions::opposite(direction));
    const BlockState* neighborState = world.getBlockState(neighborPos);

    Direction facing = direction;
    if (neighborState != nullptr && neighborState->is(this) &&
        neighborState->get(BlockStateProperties::FACING()) == direction) {
        facing = Directions::opposite(direction);
    }

    return defaultState().with(BlockStateProperties::FACING(), facing);
}

const BlockState& EndRodBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    return state.with(BlockStateProperties::FACING(), Directions::rotateDirection(facing, rotation));
}

const BlockState& EndRodBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    return state.with(BlockStateProperties::FACING(), Directions::rotateDirection(facing, rot));
}

} // namespace blocks
} // namespace mc
