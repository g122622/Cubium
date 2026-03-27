#include "DispenserBlockEntity.hpp"
#include "item/ItemStack.hpp"
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
                    auto stackResult = ItemStack::fromJson(itemData);
                    if (stackResult.success()) {
                        m_inventory.setItem(slot, stackResult.value());
                    }
                }
            }
        }
    }

    // 加载战利品表
    if (data.contains("LootTable") && data["LootTable"].is_string()) {
        m_lootTable = data["LootTable"].get<String>();
    }
    if (data.contains("LootTableSeed") && data["LootTableSeed"].is_number()) {
        m_lootTableSeed = data["LootTableSeed"].get<u64>();
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
            nlohmann::json itemJson = stack.toJson();
            itemJson["Slot"] = i;
            itemsJson.push_back(itemJson);
        }
    }
    data["Items"] = itemsJson;

    // 保存战利品表
    if (!m_lootTable.empty()) {
        data["LootTable"] = m_lootTable;
        data["LootTableSeed"] = m_lootTableSeed;
    }
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

i32 DispenserBlockEntity::getDispenseSlot() {
    // MC 储水池采样算法：每个非空槽位被选中的概率相等
    i32 selectedSlot = -1;
    i32 nonEmptyCount = 0;

    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            ++nonEmptyCount;
            // 以 1/nonEmptyCount 的概率替换当前选择
            std::uniform_int_distribution<i32> dist(1, nonEmptyCount);
            if (dist(m_rng) == 1) {
                selectedSlot = i;
            }
        }
    }

    return selectedSlot;
}

ItemStack DispenserBlockEntity::addItemStack(ItemStack stack) {
    if (stack.isEmpty()) {
        return ItemStack::EMPTY;
    }

    // 尝试与现有堆叠合并
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        ItemStack existing = m_inventory.getItem(i);
        if (!existing.isEmpty() && existing.canMergeWith(stack)) {
            i32 maxStack = existing.getMaxStackSize();
            i32 space = maxStack - existing.getCount();
            i32 toAdd = std::min(space, stack.getCount());

            existing.grow(toAdd);
            m_inventory.setItem(i, existing);
            stack.shrink(toAdd);

            if (stack.isEmpty()) {
                setChanged();
                return ItemStack::EMPTY;
            }
        }
    }

    // 尝试放入空槽位
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (m_inventory.getItem(i).isEmpty()) {
            m_inventory.setItem(i, stack);
            setChanged();
            return ItemStack::EMPTY;
        }
    }

    setChanged();
    return stack;
}

void DispenserBlockEntity::setLootTable(const String& lootTable, u64 seed) {
    m_lootTable = lootTable;
    m_lootTableSeed = seed;
}

} // namespace blockentity
} // namespace mc
