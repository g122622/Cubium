#include "ItemTag.hpp"
#include "../core/Item.hpp"
#include "../core/ItemStack.hpp"

namespace mc {
namespace item::tag {

ItemTag::ItemTag(ResourceLocation id)
    : m_id(std::move(id)) {
}

void ItemTag::add(const Item* item) {
    if (item != nullptr) {
        m_items.insert(item);
    }
}

bool ItemTag::contains(const Item* item) const {
    return m_items.find(item) != m_items.end();
}

bool ItemTag::contains(const ItemStack& stack) const {
    if (stack.isEmpty()) {
        return false;
    }
    return contains(stack.getItem());
}

std::vector<const Item*> ItemTag::getItemsList() const {
    return std::vector<const Item*>(m_items.begin(), m_items.end());
}

} // namespace item::tag
} // namespace mc
