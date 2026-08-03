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

#include "SeedsItem.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/world/block/Block.hpp"
#include <utility>

namespace mc {
namespace item {
namespace items {

SeedsItem::SeedsItem(const Block& cropBlock, ItemProperties properties)
    : BlockItem(cropBlock, std::move(properties))
{
    // 种子物品的放置逻辑完全由 BlockItem 基类处理：
    // BlockItem::onItemUse() -> tryPlace() -> canPlace() -> isValidPosition()
    // 作物方块的 isValidPosition() 会检查下方是否为可支撑的方块（耕地）以及光照条件，
    // 因此种子只能在合法位置种植，无需在此重写任何方法。
}

} // namespace items
} // namespace item
} // namespace mc
