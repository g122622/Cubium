#include "ItemTriggers.hpp"

namespace mc::advancement {

// ========== ConsumeItemTrigger ==========

ConsumeItemTrigger::Instance::Instance(ItemPredicate item)
    : m_item(std::move(item)) {
}

bool ConsumeItemTrigger::Instance::test(const ItemStack& item) const {
    return m_item.test(item);
}

Result<void> ConsumeItemTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    return {};
}

nlohmann::json ConsumeItemTrigger::Instance::conditionsToJson() const {
    if (!m_item.isAny()) {
        return {{"item", m_item.toJson()}};
    }
    return nullptr;
}

Result<std::shared_ptr<ConsumeItemTrigger::Instance>> ConsumeItemTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ConsumeItemTrigger::trigger(ServerPlayer& player, const ItemStack& item) {
    // [TODO 阶段2+3：事件系统集成] 由 ConsumeItemEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
}

std::shared_ptr<ConsumeItemTrigger::Instance> ConsumeItemTrigger::item(const ItemPredicate& item) {
    return std::make_shared<Instance>(item);
}

// ========== ItemDurabilityTrigger ==========

ItemDurabilityTrigger::Instance::Instance(ItemPredicate item, IntBounds durability, IntBounds delta)
    : m_item(std::move(item))
    , m_durability(std::move(durability))
    , m_delta(std::move(delta)) {
}

bool ItemDurabilityTrigger::Instance::test(const ItemStack& item, i32 oldDurability) const {
    if (!m_item.test(item)) {
        return false;
    }

    i32 newDurability = item.getMaxDamage() - item.getDamage();
    if (!m_durability.test(newDurability)) {
        return false;
    }

    i32 delta = oldDurability - newDurability;
    if (!m_delta.test(delta)) {
        return false;
    }

    return true;
}

Result<void> ItemDurabilityTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    if (json.contains("durability")) {
        m_durability = IntBounds::fromJson(json["durability"]);
    }

    if (json.contains("delta")) {
        m_delta = IntBounds::fromJson(json["delta"]);
    }

    return {};
}

nlohmann::json ItemDurabilityTrigger::Instance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_durability.isUnbounded()) {
        json["durability"] = m_durability.toJson();
    }
    if (!m_delta.isUnbounded()) {
        json["delta"] = m_delta.toJson();
    }

    return json;
}

Result<std::shared_ptr<ItemDurabilityTrigger::Instance>> ItemDurabilityTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ItemDurabilityTrigger::trigger(ServerPlayer& player, const ItemStack& item, i32 oldDurability) {
    // [TODO 阶段2+3：事件系统集成] 由 ItemDurabilityEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(oldDurability);
}

// ========== EnchantedItemTrigger ==========

EnchantedItemTrigger::Instance::Instance(ItemPredicate item, IntBounds levels)
    : m_item(std::move(item))
    , m_levels(std::move(levels)) {
}

bool EnchantedItemTrigger::Instance::test(const ItemStack& item, i32 levels) const {
    if (!m_item.test(item)) {
        return false;
    }
    if (!m_levels.test(levels)) {
        return false;
    }
    return true;
}

Result<void> EnchantedItemTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    if (json.contains("levels")) {
        m_levels = IntBounds::fromJson(json["levels"]);
    }

    return {};
}

nlohmann::json EnchantedItemTrigger::Instance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_levels.isUnbounded()) {
        json["levels"] = m_levels.toJson();
    }

    return json;
}

Result<std::shared_ptr<EnchantedItemTrigger::Instance>> EnchantedItemTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EnchantedItemTrigger::trigger(ServerPlayer& player, const ItemStack& item, i32 levels) {
    // [TODO 阶段2+3：事件系统集成] 由 EnchantItemEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(levels);
}

// ========== FilledBucketTrigger ==========

FilledBucketTrigger::Instance::Instance(ItemPredicate item)
    : m_item(std::move(item)) {
}

bool FilledBucketTrigger::Instance::test(const ItemStack& item) const {
    return m_item.test(item);
}

Result<void> FilledBucketTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    return {};
}

nlohmann::json FilledBucketTrigger::Instance::conditionsToJson() const {
    if (!m_item.isAny()) {
        return {{"item", m_item.toJson()}};
    }
    return nullptr;
}

Result<std::shared_ptr<FilledBucketTrigger::Instance>> FilledBucketTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void FilledBucketTrigger::trigger(ServerPlayer& player, const ItemStack& item) {
    // [TODO 阶段2+3：事件系统集成] 由 FilledBucketEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
}

} // namespace mc::advancement
