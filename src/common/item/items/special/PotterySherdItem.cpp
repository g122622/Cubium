/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "item/items/special/PotterySherdItem.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/blockentity/interactive/DecoratedPotPattern.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace item {

PotterySherdItem::PotterySherdItem(blockentity::DecoratedPotPattern pattern, ItemProperties properties)
    : Item(std::move(properties))
    , m_pattern(pattern)
{}

void PotterySherdItem::addInformation(
    const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const
{
    Item::addInformation(stack, world, tooltip, advanced);

    // MC Java 中陶片物品没有额外的 tooltip 描述行
    // 原版 PotterySherdItem 不覆盖 appendHoverText，仅显示物品名
}

} // namespace item
} // namespace mc
