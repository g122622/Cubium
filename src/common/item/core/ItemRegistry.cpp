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

#include "ItemRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <utility>

namespace mc {

ItemRegistry& ItemRegistry::instance()
{
    static ItemRegistry instance;
    return instance;
}

ItemRegistry::ItemRegistry()
{
    // 预留空间
    m_itemsById.reserve(1024);
    m_itemsById.push_back(nullptr); // ID 0 保留

    // 注册空气物品（表示无物品）
    // ID 0 留给空气，但不在普通查找中出现
}

Item& ItemRegistry::registerItem(const ResourceLocation& id, ItemProperties properties)
{
    return registerItem<Item>(id, std::move(properties));
}

ItemId ItemRegistry::_allocateItemId()
{
    // 寻找可用的ID槽位
    while (m_nextItemId < m_itemsById.size() && m_itemsById[m_nextItemId] != nullptr) {
        ++m_nextItemId;
    }
    return m_nextItemId++;
}

} // namespace mc
