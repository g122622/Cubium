#include "DispenserBlockEntity.hpp"
#include "entity/loot/LootTable.hpp"
#include "entity/loot/LootContext.hpp"
#include "item/core/ItemStack.hpp"
#include <random>

namespace mc {
namespace blockentity {

DispenserBlockEntity::DispenserBlockEntity(BlockEntityType type, const BlockPos& pos)
    : LootableContainerBlockEntity(type, pos)
    , m_inventory(INVENTORY_SIZE)
    , m_rng(std::random_device{}()) {
}

bool DispenserBlockEntity::load(const nlohmann::json& data) {
    if (!LootableContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载库存
    if (data.contains("Items") && data["Items"].is_array()) {
        const auto& items = data["Items"];
        for (const auto& itemData : items) {
            if (itemData.contains("Slot") && itemData["Slot"].is_number()) {
                i32 slot = itemData["Slot"].get<i32>();
                if (slot >= 0 && slot < INVENTORY_SIZE) {
                    auto stackResult = ItemStack::fromJson(itemData);
                    if (stackResult.success()) {
                        m_inventory.setItem(slot, stackResult.value());
                    }
                }
            }
        }
    }

    return true;
}

void DispenserBlockEntity::save(nlohmann::json& data) const {
    LootableContainerBlockEntity::save(data);

    // 保存库存
    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            nlohmann::json itemJson = stack.toJson();
            itemJson["Slot"] = i;
            itemsJson.push_back(itemJson);
        }
    }
    data["Items"] = itemsJson;
}

std::unique_ptr<BlockEntity> DispenserBlockEntity::clone() const {
    auto cloned = std::make_unique<DispenserBlockEntity>(m_type, m_pos);

    // 复制库存内容
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(i, stack.copy());
        }
    }

    return cloned;
}

void DispenserBlockEntity::clearContainer() {
    m_inventory.clear();
    setChanged();
}

i32 DispenserBlockEntity::getRandomSlot() {
    i32 nonEmptyCount = 0;
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            ++nonEmptyCount;
        }
    }

    if (nonEmptyCount == 0) {
        return -1;
    }

    i32 selectedIndex = m_rng.nextInt(nonEmptyCount);
    i32 currentIndex = 0;
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            if (currentIndex == selectedIndex) {
                return i;
            }
            ++currentIndex;
        }
    }

    return -1;
}

i32 DispenserBlockEntity::getDispenseSlot() {
    // MC 储水池采样算法：每个非空槽位被选中的概率相等
    i32 selectedSlot = -1;
    i32 nonEmptyCount = 0;

    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            ++nonEmptyCount;
            // 以 1/nonEmptyCount 的概率替换当前选择
            if (m_rng.nextInt(nonEmptyCount) == 0) {
                selectedSlot = i;
            }
        }
    }

    return selectedSlot;
}

i32 DispenserBlockEntity::addItemStack(const ItemStack& stack) {
    // MC 1.16.5: 查找第一个空槽位，将整个物品放入该槽位
    // 不尝试与现有堆叠合并
    if (stack.isEmpty()) {
        return -1;
    }

    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (m_inventory.getItem(i).isEmpty()) {
            m_inventory.setItem(i, stack.copy());
            setChanged();
            return i;
        }
    }

    return -1;
}

} // namespace blockentity
} // namespace mc
