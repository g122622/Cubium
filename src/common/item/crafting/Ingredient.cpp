#include "item/crafting/Ingredient.hpp"
#include "item/core/ItemRegistry.hpp"
#include <algorithm>
#include <set>

namespace mc {
namespace crafting {

Ingredient Ingredient::fromItem(const Item& item) {
    return fromItem(&item);
}

Ingredient Ingredient::fromItem(const Item* item) {
    if (item == nullptr) {
        return Ingredient();
    }
    Ingredient ing;
    ing.m_matchingStacks.emplace_back(*item, 1);
    return ing;
}

Ingredient Ingredient::fromItems(std::vector<const Item*> items) {
    Ingredient ing;
    ing.m_matchingStacks.reserve(items.size());
    for (const Item* item : items) {
        if (item != nullptr) {
            ing.m_matchingStacks.emplace_back(*item, 1);
        }
    }
    return ing;
}

Ingredient Ingredient::fromTag(const String& tag) {
    Ingredient ing;
    ing.m_tag = tag;
    ing.m_hasTag = true;

    // TODO: 从物品标签系统解析物品
    // 当前版本标签系统尚未实现，返回空Ingredient
    // 未来实现：ItemTags::getItemsInTag(tag)

    return ing;
}

Ingredient Ingredient::fromStacks(std::vector<ItemStack> stacks) {
    Ingredient ing;
    ing.m_matchingStacks = std::move(stacks);
    return ing;
}

bool Ingredient::test(const ItemStack& stack) const {
    if (isEmpty()) {
        return false;
    }

    if (stack.isEmpty()) {
        return false;
    }

    // 如果有标签，检查物品是否在标签中
    if (m_hasTag) {
        // TODO: 实现标签检查
        // 当前版本返回false
        return false;
    }

    // 检查物品是否在匹配列表中
    for (const ItemStack& matchingStack : m_matchingStacks) {
        if (matchingStack.isSameItem(stack)) {
            return true;
        }
    }

    return false;
}

bool Ingredient::test(const Item& item) const {
    return test(&item);
}

bool Ingredient::test(const Item* item) const {
    if (isEmpty()) {
        return false;
    }

    if (item == nullptr) {
        return false;
    }

    // 如果有标签，检查物品是否在标签中
    if (m_hasTag) {
        // TODO: 实现标签检查
        return false;
    }

    // 检查物品是否在匹配列表中
    for (const ItemStack& matchingStack : m_matchingStacks) {
        if (matchingStack.getItem() == item) {
            return true;
        }
    }

    return false;
}

bool Ingredient::operator==(const Ingredient& other) const {
    // 检查标签匹配
    if (m_hasTag != other.m_hasTag) {
        return false;
    }
    if (m_hasTag && m_tag != other.m_tag) {
        return false;
    }

    // 如果都有匹配堆，检查它们是否相同
    if (m_matchingStacks.size() != other.m_matchingStacks.size()) {
        return false;
    }

    // 创建物品ID集合进行比较
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

size_t Ingredient::hash() const {
    size_t h = 0;

    if (m_hasTag) {
        // 使用 std::hash<String> 进行哈希
        return std::hash<String>{}(m_tag);
    }

    // 组合物品ID哈希
    // 使用排序后的ID列表确保一致性
    std::set<ItemId> ids;
    for (const ItemStack& stack : m_matchingStacks) {
        if (stack.getItem()) {
            ids.insert(stack.getItem()->itemId());
        }
    }
    for (ItemId id : ids) {
        // 使用标准哈希组合模式
        h ^= std::hash<ItemId>{}(id) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return h;
}

} // namespace crafting
} // namespace mc
