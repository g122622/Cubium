#include "item/crafting/Ingredient.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/tag/ItemTags.hpp"
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

    item::tag::ItemTag* itemTag = item::tag::ItemTags::getTag(tag);
    if (itemTag != nullptr) {
        ing.m_tagItems = itemTag->getItemsList();
        ing.m_tagResolved = true;
    }

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

    if (m_hasTag) {
        if (!m_tagResolved) {
            item::tag::ItemTag* itemTag = item::tag::ItemTags::getTag(m_tag);
            m_tagItems.clear();
            if (itemTag != nullptr) {
                m_tagItems = itemTag->getItemsList();
            }
            m_tagResolved = true;
        }

        const Item* stackItem = stack.getItem();
        for (const Item* taggedItem : m_tagItems) {
            if (taggedItem == stackItem) {
                return true;
            }
        }
        return false;
    }

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

    if (m_hasTag) {
        if (!m_tagResolved) {
            item::tag::ItemTag* itemTag = item::tag::ItemTags::getTag(m_tag);
            m_tagItems.clear();
            if (itemTag != nullptr) {
                m_tagItems = itemTag->getItemsList();
            }
            m_tagResolved = true;
        }

        for (const Item* taggedItem : m_tagItems) {
            if (taggedItem == item) {
                return true;
            }
        }
        return false;
    }

    for (const ItemStack& matchingStack : m_matchingStacks) {
        if (matchingStack.getItem() == item) {
            return true;
        }
    }

    return false;
}

bool Ingredient::operator==(const Ingredient& other) const {
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

size_t Ingredient::hash() const {
    size_t h = 0;

    if (m_hasTag) {
        return std::hash<String>{}(m_tag);
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