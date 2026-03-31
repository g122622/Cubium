#include "ItemRegistry.hpp"
#include "../Items.hpp"

namespace mc {

ItemRegistry& ItemRegistry::instance() {
    static ItemRegistry instance;
    return instance;
}

ItemRegistry::ItemRegistry() {
    // 预留空间
    m_itemsById.reserve(1024);
    m_itemsById.push_back(nullptr);  // ID 0 保留

    // 注册空气物品（表示无物品）
    // ID 0 留给空气，但不在普通查找中出现
}

Item& ItemRegistry::registerItem(const ResourceLocation& id, ItemProperties properties) {
    return registerItem<Item>(id, std::move(properties));
}

ItemId ItemRegistry::allocateItemId() {
    // 寻找可用的ID槽位
    while (m_nextItemId < m_itemsById.size() && m_itemsById[m_nextItemId] != nullptr) {
        ++m_nextItemId;
    }
    return m_nextItemId++;
}

} // namespace mc
