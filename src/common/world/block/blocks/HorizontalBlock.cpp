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

#include "HorizontalBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
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

HorizontalBlock::HorizontalBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
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

BlockState HorizontalBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家水平朝向设置方块方向
    Direction facing = context.horizontalDirection();

    // 某些方块可能需要反向（如熔炉面向玩家，活塞朝向玩家）
    // 子类可以重写此方法来改变行为
    return withFacing(defaultState(), facing);
}

const BlockState& HorizontalBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = getFacing(state);

    // 水平方向旋转
    Direction newFacing = currentFacing;
    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(currentFacing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(currentFacing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(currentFacing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return withFacing(state, newFacing);
}

const BlockState& HorizontalBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = getFacing(state);
    Direction newFacing = currentFacing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (currentFacing == Direction::East) {
                newFacing = Direction::West;
            } else if (currentFacing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (currentFacing == Direction::North) {
                newFacing = Direction::South;
            } else if (currentFacing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return withFacing(state, newFacing);
}

Direction HorizontalBlock::getFacing(const BlockState& state) const
{
    Direction facing = state.get(FACING());
    // 确保朝向是水平的
    MC_ASSERT_RELEASE(Directions::isHorizontal(facing));
    return facing;
}

const BlockState& HorizontalBlock::withFacing(const BlockState& state, Direction facing) const
{
    // 确保朝向是水平的
    MC_ASSERT_RELEASE(Directions::isHorizontal(facing));
    return state.with(FACING(), facing);
}

} // namespace blocks
} // namespace mc
