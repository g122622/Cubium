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

#include "BedItem.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <utility>

namespace mc {

BedItem::BedItem(const Block& block, ItemProperties properties)
    : BlockItem(block, std::move(properties))
{}

const BlockState* BedItem::getStateForPlacement(const BlockItemUseContext& context) const
{
    Direction facing = context.horizontalDirection();
    BlockPos headPos = context.placementPos().offset(facing);

    // 检查头部位置是否可替换（空气、花草等 canBeReplaced=true 的方块）
    // 注意：getBlockState 在未设置位置返回 nullptr，等同于空气，可以被替换
    const BlockState* headState = context.getWorld().getBlockState(headPos);
    if (headState != nullptr && !headState->canBeReplaced()) {
        return nullptr;
    }

    // 返回带有正确朝向的脚部方块状态
    return &block().defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

} // namespace mc
