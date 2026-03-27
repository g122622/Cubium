#include "DispenserBlockEntity.hpp"
#include <algorithm>

namespace mc {
namespace blockentity {

DispenserBlockEntity::DispenserBlockEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
    , m_inventory(INVENTORY_SIZE)
    , m_rng(std::random_device{}()) {
}

bool DispenserBlockEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载库存
    if (data.contains("Items") && data["Items"].is_array()) {
        const auto& items = data["Items"];
        for (const auto& itemData : items) {
            if (itemData.contains("Slot") && itemData["Slot"].is_number()) {
                i32 slot = itemData["Slot"].get<i32>();
                if (slot >= 0 && slot < INVENTORY_SIZE) {
                    // TODO: 从 JSON 加载 ItemStack
                    // m_inventory.setItem(slot, ItemStack::fromJson(itemData));
                }
            }
        }
    }

    return true;
}

void DispenserBlockEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    // 保存库存
    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            nlohmann::json itemJson;
            itemJson["Slot"] = i;
            // TODO: 保存 ItemStack 数据
            // itemJson["id"] = stack.getItem()->getId().toString();
            // itemJson["Count"] = stack.getCount();
            itemsJson.push_back(itemJson);
        }
    }
    data["Items"] = itemsJson;
}

bool DispenserBlockEntity::isEmpty() const {
    return m_inventory.isEmpty();
}

void DispenserBlockEntity::clearContainer() {
    m_inventory.clear();
    setChanged();
}

i32 DispenserBlockEntity::getRandomSlot() {
    // 收集所有非空槽位
    std::vector<i32> nonEmptySlots;
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            nonEmptySlots.push_back(i);
        }
    }

    if (nonEmptySlots.empty()) {
        return -1;
    }

    // 随机选择一个非空槽位
    std::uniform_int_distribution<i32> dist(0, static_cast<i32>(nonEmptySlots.size()) - 1);
    return nonEmptySlots[dist(m_rng)];
}

} // namespace blockentity
} // namespace mc
