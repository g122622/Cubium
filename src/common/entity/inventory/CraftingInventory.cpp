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

#include "entity/inventory/CraftingInventory.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/crafting/IRecipe.hpp"
#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {

// ========== CraftingInventory ==========

CraftingInventory::CraftingInventory(i32 width, i32 height)
    : m_width(width)
    , m_height(height)
{
    m_items.resize(static_cast<size_t>(width * height));
}

bool CraftingInventory::isEmpty() const
{
    for (const ItemStack& stack : m_items) {
        if (!stack.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack CraftingInventory::getItem(i32 slot) const
{
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return ItemStack();
    }
    return m_items[static_cast<size_t>(slot)];
}

void CraftingInventory::setItem(i32 slot, const ItemStack& stack)
{
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return;
    }
    m_items[static_cast<size_t>(slot)] = stack;
    setChanged();
}

ItemStack CraftingInventory::removeItem(i32 slot, i32 count)
{
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return ItemStack();
    }

    ItemStack& stack = m_items[static_cast<size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack result = stack.split(count);
    if (!result.isEmpty()) {
        setChanged();
    }
    return result;
}

ItemStack CraftingInventory::removeItemNoUpdate(i32 slot)
{
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return ItemStack();
    }

    ItemStack result = std::move(m_items[static_cast<size_t>(slot)]);
    m_items[static_cast<size_t>(slot)] = ItemStack();
    return result;
}

void CraftingInventory::clear()
{
    for (ItemStack& stack : m_items) {
        stack = ItemStack();
    }
    setChanged();
}

void CraftingInventory::setChanged()
{
    IInventory::setChanged();
    if (m_onContentChanged) {
        m_onContentChanged();
    }
}

ItemStack CraftingInventory::getItemAt(i32 x, i32 y) const
{
    i32 slot = posToSlot(x, y);
    if (slot < 0) {
        return ItemStack();
    }
    return m_items[static_cast<size_t>(slot)];
}

void CraftingInventory::setItemAt(i32 x, i32 y, const ItemStack& stack)
{
    i32 slot = posToSlot(x, y);
    if (slot < 0) {
        return;
    }
    m_items[static_cast<size_t>(slot)] = stack;
    setChanged();
}

ItemStack CraftingInventory::removeItemAt(i32 x, i32 y, i32 count)
{
    i32 slot = posToSlot(x, y);
    if (slot < 0) {
        return ItemStack();
    }
    return removeItem(slot, count);
}

i32 CraftingInventory::posToSlot(i32 x, i32 y) const
{
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return -1;
    }
    return y * m_width + x;
}

bool CraftingInventory::slotToPos(i32 slot, i32& outX, i32& outY) const
{
    if (slot < 0 || slot >= m_width * m_height) {
        return false;
    }
    outX = slot % m_width;
    outY = slot / m_width;
    return true;
}

void CraftingInventory::setItems(const std::vector<ItemStack>& items)
{
    if (static_cast<i32>(items.size()) != m_width * m_height) {
        return; // 大小不匹配，忽略
    }
    m_items = items;
    setChanged();
}

bool CraftingInventory::isAllEmpty() const
{
    return isEmpty();
}

bool CraftingInventory::getContentBounds(i32& outMinX, i32& outMinY, i32& outMaxX, i32& outMaxY) const
{
    bool found = false;
    outMinX = m_width;
    outMinY = m_height;
    outMaxX = -1;
    outMaxY = -1;

    for (i32 y = 0; y < m_height; ++y) {
        for (i32 x = 0; x < m_width; ++x) {
            if (!getItemAt(x, y).isEmpty()) {
                found = true;
                outMinX = std::min(outMinX, x);
                outMinY = std::min(outMinY, y);
                outMaxX = std::max(outMaxX, x);
                outMaxY = std::max(outMaxY, y);
            }
        }
    }

    return found;
}

void CraftingInventory::fillStackedContents(std::unordered_map<i32, i32>& itemCounts) const
{
    // 遍历所有物品，只计数"普通"物品（未损坏、未附魔、无自定义名称）
    for (const ItemStack& stack : m_items) {
        if (stack.isEmpty()) {
            continue;
        }
        // 检查是否为"普通"物品
        if (!stack.isDamaged() && !stack.hasEnchantments() && !stack.hasDisplayName()) {
            const Item* item = stack.getItem();
            if (item != nullptr) {
                i32 itemId = static_cast<i32>(item->itemId());
                i32 count = stack.getCount();
                itemCounts[itemId] += count;
            }
        }
    }
}

// ========== CraftResultInventory ==========

ItemStack CraftResultInventory::getItem(i32 slot) const
{
    (void)slot;
    return m_result;
}

void CraftResultInventory::setItem(i32 slot, const ItemStack& stack)
{
    (void)slot;
    m_result = stack;
    setChanged();
}

ItemStack CraftResultInventory::removeItem(i32 slot, i32 count)
{
    if (slot != 0 || m_result.isEmpty()) {
        return ItemStack();
    }

    ItemStack result = std::move(m_result);
    m_result = ItemStack();
    setChanged();
    return result;
    (void)count; // 忽略count参数
}

ItemStack CraftResultInventory::removeItemNoUpdate(i32 slot)
{
    if (slot != 0) {
        return ItemStack();
    }
    ItemStack result = std::move(m_result);
    m_result = ItemStack();
    return result;
}

void CraftResultInventory::clear()
{
    m_result = ItemStack();
    setChanged();
}

void CraftResultInventory::setChanged()
{
    IInventory::setChanged();
}

void CraftResultInventory::onCrafting(Player& player)
{
    if (m_craftingRecipeUsed != nullptr && !m_craftingRecipeUsed->isDynamic()) {
        ResourceLocation recipeId = m_craftingRecipeUsed->getId();
        player.unlockRecipe(recipeId);
        m_craftingRecipeUsed = nullptr;
    }
}

ResourceLocation CraftResultInventory::getRecipeUsedId() const
{
    return m_craftingRecipeUsed != nullptr ? m_craftingRecipeUsed->getId() : ResourceLocation();
}

} // namespace mc
