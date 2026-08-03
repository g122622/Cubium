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

#include "ItemTag.hpp"

#include "../../util/assert/AssertAll.hpp"
#include "../core/Item.hpp"
#include "../core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace item::tag {

ItemTag::ItemTag(ResourceLocation id, bool replace)
    : m_id(std::move(id))
    , m_replace(replace)
{}

void ItemTag::add(const Item* item)
{
    if (item == nullptr) {
        spdlog::warn("ItemTag::add(): Attempted to add null item to tag '{}', skipping. "
                     "This usually indicates the item has not been registered yet.",
            m_id.toString());
        return;
    }
    m_items.insert(item);
}

bool ItemTag::contains(const Item* item) const noexcept
{
    return m_items.find(item) != m_items.end();
}

bool ItemTag::contains(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return false;
    }
    return contains(stack.getItem());
}

std::vector<const Item*> ItemTag::getItemsList() const
{
    return std::vector<const Item*>(m_items.begin(), m_items.end());
}

void ItemTag::addAll(const std::vector<const Item*>& items)
{
    for (const auto* item : items) {
        add(item);
    }
}

void ItemTag::clear()
{
    m_items.clear();
}

} // namespace item::tag
} // namespace mc
