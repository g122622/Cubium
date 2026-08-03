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

#include "ItemGroup.hpp"
#include "Item.hpp"
#include "ItemStack.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {

ItemGroup::ItemGroup(Type type, std::string id)
    : m_type(type)
    , m_id(std::move(id))
{}

ItemStack ItemGroup::getIconItem() const
{
    if (m_iconItem != nullptr) {
        return ItemStack(*m_iconItem, 1);
    }
    return ItemStack();
}

void ItemGroup::fill(std::vector<ItemStack>& items) const
{
    if (m_fillFunc) {
        m_fillFunc(items);
    }
}

void ItemGroup::setIconItem(const Item* item)
{
    m_iconItem = item;
}

} // namespace mc
