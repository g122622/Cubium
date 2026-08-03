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

#include "DirectionalBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

DirectionalBlock::DirectionalBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 默认朝向为北
    setDefaultState(withFacing(defaultState(), Direction::North));
}

BlockState DirectionalBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家朝向设置方块方向
    // 玩家面朝的方向的反方向就是方块的 FACING
    Direction facing = context.horizontalDirection();

    // 对于可放置在墙上/地面的方块，需要根据击中面调整
    // 但这里只处理水平朝向的简单情况
    return withFacing(defaultState(), facing);
}

const BlockState& DirectionalBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = getFacing(state);
    Direction newFacing = Directions::rotateDirection(currentFacing, rotation);
    return withFacing(state, newFacing);
}

const BlockState& DirectionalBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = getFacing(state);
    Rotation rot = Directions::mirrorToRotation(mirror, currentFacing);
    Direction newFacing = Directions::rotateDirection(currentFacing, rot);
    return withFacing(state, newFacing);
}

Direction DirectionalBlock::getFacing(const BlockState& state) const
{
    return state.get(FACING());
}

const BlockState& DirectionalBlock::withFacing(const BlockState& state, Direction facing) const
{
    return state.with(FACING(), facing);
}

} // namespace blocks
} // namespace mc
