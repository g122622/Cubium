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

#include "item/crafting/Ingredient.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/tag/ItemTags.hpp"
#include <algorithm>
#include <set>

namespace mc {
namespace crafting {

// 空 Ingredient 常量定义
const Ingredient Ingredient::EMPTY;

Ingredient Ingredient::fromItem(const Item& item)
{
    return fromItem(&item);
}

Ingredient Ingredient::fromItem(const Item* item)
{
    if (item == nullptr) {
        return Ingredient();
    }
    Ingredient ing;
    ing.m_matchingStacks.emplace_back(*item, 1);
    ing._updateSimple();
    return ing;
}

Ingredient Ingredient::fromItems(std::vector<const Item*> items)
{
    Ingredient ing;
    ing.m_matchingStacks.reserve(items.size());
    for (const Item* item : items) {
        if (item != nullptr) {
            ing.m_matchingStacks.emplace_back(*item, 1);
        }
    }
    ing._updateSimple();
    return ing;
}

Ingredient Ingredient::fromTag(const std::string& tag)
{
    Ingredient ing;
    ing.m_tag = tag;
    ing.m_hasTag = true;

    // 尝试立即解析标签内容
    item::tag::ItemTag* itemTag = item::tag::ItemTags::getTag(tag);
    if (itemTag != nullptr) {
        ing.m_tagItems = itemTag->getItemsList();
        ing.m_tagResolved = true;
    }

    ing._updateSimple();

    return ing;
}

Ingredient Ingredient::fromStacks(std::vector<ItemStack> stacks)
{
    Ingredient ing;
    ing.m_matchingStacks = std::move(stacks);
    ing._updateSimple();
    return ing;
}

Ingredient Ingredient::merge(const std::vector<Ingredient>& parts)
{
    Ingredient result;
    std::set<ItemId> addedIds; // 去重

    for (const Ingredient& part : parts) {
        for (const Item* item : part.getAllMatchingItems()) {
            if (item && addedIds.find(item->itemId()) == addedIds.end()) {
                result.m_matchingStacks.emplace_back(*item, 1);
                addedIds.insert(item->itemId());
            }
        }
    }

    result._updateSimple();
    return result;
}

bool Ingredient::test(const ItemStack& stack) const
{
    // 空 Ingredient 只匹配空物品堆
    if (isEmpty()) {
        return stack.isEmpty();
    }

    if (stack.isEmpty()) {
        return false;
    }

    // 标签匹配
    if (m_hasTag) {
        _resolveTagIfNeeded();
        const Item* stackItem = stack.getItem();
        for (const Item* taggedItem : m_tagItems) {
            if (taggedItem == stackItem) {
                return true;
            }
        }
        return false;
    }

    // 物品列表匹配（只比较物品类型，不检查 NBT 数据）
    for (const ItemStack& matchingStack : m_matchingStacks) {
        if (matchingStack.isSameItem(stack)) {
            return true;
        }
    }

    return false;
}

bool Ingredient::test(const Item& item) const
{
    return test(&item);
}

bool Ingredient::test(const Item* item) const
{
    if (isEmpty()) {
        return item == nullptr;
    }

    if (item == nullptr) {
        return false;
    }

    // 标签匹配
    if (m_hasTag) {
        _resolveTagIfNeeded();
        for (const Item* taggedItem : m_tagItems) {
            if (taggedItem == item) {
                return true;
            }
        }
        return false;
    }

    // 物品列表匹配
    for (const ItemStack& matchingStack : m_matchingStacks) {
        if (matchingStack.getItem() == item) {
            return true;
        }
    }

    return false;
}

bool Ingredient::isSimple() const
{
    // 空 Ingredient 视为简单原料
    if (isEmpty()) {
        return true;
    }
    return m_isSimple;
}

bool Ingredient::hasNoMatchingItems() const
{
    if (m_hasTag) {
        _resolveTagIfNeeded();
        return m_tagItems.empty() && m_matchingStacks.empty();
    }
    return m_matchingStacks.empty();
}

std::vector<const Item*> Ingredient::getAllMatchingItems() const
{
    std::vector<const Item*> items;

    // 添加显式物品列表中的物品
    for (const ItemStack& stack : m_matchingStacks) {
        if (stack.getItem() != nullptr) {
            items.push_back(stack.getItem());
        }
    }

    // 如果有标签，解析并添加标签中的物品
    if (m_hasTag) {
        _resolveTagIfNeeded();
        for (const Item* item : m_tagItems) {
            if (item != nullptr) {
                items.push_back(item);
            }
        }
    }

    return items;
}

void Ingredient::_updateSimple() const
{
    m_isSimple = true;

    // 检查显式物品列表中是否包含可损坏物品
    for (const ItemStack& stack : m_matchingStacks) {
        const Item* item = stack.getItem();
        if (item != nullptr && item->isDamageable()) {
            m_isSimple = false;
            return;
        }
    }

    // 检查标签解析后的物品列表中是否包含可损坏物品
    if (m_hasTag) {
        if (m_tagResolved) {
            // 标签已解析：检查标签中的物品是否可损坏
            for (const Item* item : m_tagItems) {
                if (item != nullptr && item->isDamageable()) {
                    m_isSimple = false;
                    return;
                }
            }
            // 标签已解析且所有标签物品都不可损坏时，
            // 只有当标签确实包含物品时才视为简单原料
            // 空标签（标签不存在或无物品）保守地视为非简单原料
            if (m_tagItems.empty()) {
                m_isSimple = false;
            }
        } else {
            // 标签尚未解析：无法确定标签内容，保守地视为非简单原料
            m_isSimple = false;
        }
    }
}

void Ingredient::_resolveTagIfNeeded() const
{
    if (m_hasTag && !m_tagResolved) {
        item::tag::ItemTag* itemTag = item::tag::ItemTags::getTag(m_tag);
        m_tagItems.clear();
        if (itemTag != nullptr) {
            m_tagItems = itemTag->getItemsList();
        }
        m_tagResolved = true;

        // 延迟解析后更新isSimple标志
        _updateSimple();
    }
}

bool Ingredient::operator==(const Ingredient& other) const
{
    if (m_hasTag != other.m_hasTag) {
        return false;
    }
    if (m_hasTag && m_tag != other.m_tag) {
        return false;
    }

    if (m_matchingStacks.size() != other.m_matchingStacks.size()) {
        return false;
    }

    std::set<ItemId> thisItems;
    std::set<ItemId> otherItems;

    for (const ItemStack& stack : m_matchingStacks) {
        thisItems.insert(stack.getItem()->itemId());
    }
    for (const ItemStack& stack : other.m_matchingStacks) {
        otherItems.insert(stack.getItem()->itemId());
    }

    return thisItems == otherItems;
}

size_t Ingredient::hash() const
{
    size_t h = 0;

    if (m_hasTag) {
        return std::hash<std::string>{}(m_tag);
    }

    std::set<ItemId> ids;
    for (const ItemStack& stack : m_matchingStacks) {
        if (stack.getItem()) {
            ids.insert(stack.getItem()->itemId());
        }
    }
    for (ItemId id : ids) {
        h ^= std::hash<ItemId>{}(id) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}

} // namespace crafting
} // namespace mc