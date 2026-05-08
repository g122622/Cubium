#include "ItemGroup.hpp"
#include "Item.hpp"
#include "ItemStack.hpp"

namespace mc {

ItemGroup::ItemGroup(Type type, std::string id)
    : m_type(type)
    , m_id(std::move(id)) {
}

ItemStack ItemGroup::getIconItem() const {
    if (m_iconItem != nullptr) {
        return ItemStack(*m_iconItem, 1);
    }
    return ItemStack();
}

void ItemGroup::fill(std::vector<ItemStack>& items) const {
    if (m_fillFunc) {
        m_fillFunc(items);
    }
}

void ItemGroup::setIconItem(const Item* item) {
    m_iconItem = item;
}

} // namespace mc
