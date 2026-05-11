#include "InventoryChangedTrigger.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== InventoryChangedTriggerInstance ==========

InventoryChangedTriggerInstance::InventoryChangedTriggerInstance(
    IntBounds slotsOccupied,
    IntBounds slotsFull,
    IntBounds slotsEmpty,
    std::vector<ItemPredicate> items
)
    : m_slotsOccupied(std::move(slotsOccupied))
    , m_slotsFull(std::move(slotsFull))
    , m_slotsEmpty(std::move(slotsEmpty))
    , m_items(std::move(items)) {
}

bool InventoryChangedTriggerInstance::test(ServerPlayer& player, const PlayerInventory& inventory) const {
    // 检查槽位数量
    i32 occupied = 0;
    i32 full = 0;
    i32 empty = 0;

    // TODO: 计算槽位数量
    // for (i32 i = 0; i < inventory.getSize(); ++i) {
    //     const auto& slot = inventory.getSlot(i);
    //     if (slot.isEmpty()) {
    //         ++empty;
    //     } else {
    //         ++occupied;
    //         if (slot.getCount() >= slot.getMaxStackSize()) {
    //             ++full;
    //         }
    //     }
    // }

    MC_UNUSED(player);
    MC_UNUSED(inventory);

    if (!m_slotsOccupied.test(occupied)) return false;
    if (!m_slotsFull.test(full)) return false;
    if (!m_slotsEmpty.test(empty)) return false;

    // 检查物品谓词
    if (!m_items.empty()) {
        for (const auto& predicate : m_items) {
            bool found = false;
            // TODO: 遍历物品栏查找匹配的物品
            // for (i32 i = 0; i < inventory.getSize(); ++i) {
            //     if (predicate.test(inventory.getSlot(i))) {
            //         found = true;
            //         break;
            //     }
            // }
            MC_UNUSED(predicate);
            if (!found) {
                return false;
            }
        }
    }

    return true;
}

Result<void> InventoryChangedTriggerInstance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("slots")) {
        const auto& slots = json["slots"];
        if (slots.contains("occupied")) {
            m_slotsOccupied = IntBounds::fromJson(slots["occupied"]);
        }
        if (slots.contains("full")) {
            m_slotsFull = IntBounds::fromJson(slots["full"]);
        }
        if (slots.contains("empty")) {
            m_slotsEmpty = IntBounds::fromJson(slots["empty"]);
        }
    }

    if (json.contains("items")) {
        for (const auto& itemJson : json["items"]) {
            auto result = ItemPredicate::fromJson(itemJson);
            if (result.failed()) {
                return result.error();
            }
            m_items.push_back(result.value());
        }
    }

    return {};
}

nlohmann::json InventoryChangedTriggerInstance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_slotsOccupied.isUnbounded() || !m_slotsFull.isUnbounded() || !m_slotsEmpty.isUnbounded()) {
        nlohmann::json slots;
        if (!m_slotsOccupied.isUnbounded()) {
            slots["occupied"] = m_slotsOccupied.toJson();
        }
        if (!m_slotsFull.isUnbounded()) {
            slots["full"] = m_slotsFull.toJson();
        }
        if (!m_slotsEmpty.isUnbounded()) {
            slots["empty"] = m_slotsEmpty.toJson();
        }
        json["slots"] = std::move(slots);
    }

    if (!m_items.empty()) {
        nlohmann::json items = nlohmann::json::array();
        for (const auto& item : m_items) {
            items.push_back(item.toJson());
        }
        json["items"] = std::move(items);
    }

    return json;
}

// ========== InventoryChangedTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> InventoryChangedTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<InventoryChangedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void InventoryChangedTrigger::trigger(ServerPlayer& player, const PlayerInventory& inventory) {
    // 物品栏变化触发器
    // 实际触发逻辑需要在服务端模块中实现，
    // 服务端事件系统会在物品栏变化时调用此方法
    // 这里会检查所有已注册的监听器，如果条件匹配则授予进度
    MC_UNUSED(player);
    MC_UNUSED(inventory);
}

std::shared_ptr<InventoryChangedTriggerInstance> InventoryChangedTrigger::hasItems(std::vector<ItemPredicate> items) {
    return std::make_shared<InventoryChangedTriggerInstance>(IntBounds(), IntBounds(), IntBounds(), std::move(items));
}

std::shared_ptr<InventoryChangedTriggerInstance> InventoryChangedTrigger::hasItem(const ItemPredicate& item) {
    return hasItems({item});
}

} // namespace mc::advancement
