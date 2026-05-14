#include "ItemTriggers.hpp"

namespace mc::advancement {

// ========== ConsumeItemTriggerInstance ==========

ConsumeItemTriggerInstance::ConsumeItemTriggerInstance(ItemPredicate item)
    : m_item(std::move(item))
{}

bool ConsumeItemTriggerInstance::test(const ItemStack& item) const
{
    return m_item.test(item);
}

Result<void> ConsumeItemTriggerInstance::fromJson(const nlohmann::json& json)
{
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

nlohmann::json ConsumeItemTriggerInstance::conditionsToJson() const
{
    if (!m_item.isAny()) {
        return {{"item", m_item.toJson()}};
    }
    return nullptr;
}

// ========== ConsumeItemTrigger ==========

Result<std::shared_ptr<ConsumeItemTriggerInstance>> ConsumeItemTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<ConsumeItemTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ConsumeItemTrigger::trigger(ServerPlayer& player, const ItemStack& item)
{
    // [TODO 阶段2+3：事件系统集成] 由 ConsumeItemEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
}

std::shared_ptr<ConsumeItemTriggerInstance> ConsumeItemTrigger::item(const ItemPredicate& item)
{
    return std::make_shared<ConsumeItemTriggerInstance>(item);
}

// ========== ItemDurabilityTriggerInstance ==========

ItemDurabilityTriggerInstance::ItemDurabilityTriggerInstance(ItemPredicate item, IntBounds durability, IntBounds delta)
    : m_item(std::move(item))
    , m_durability(std::move(durability))
    , m_delta(std::move(delta))
{}

bool ItemDurabilityTriggerInstance::test(const ItemStack& item, i32 oldDurability) const
{
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

Result<void> ItemDurabilityTriggerInstance::fromJson(const nlohmann::json& json)
{
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

nlohmann::json ItemDurabilityTriggerInstance::conditionsToJson() const
{
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

// ========== ItemDurabilityTrigger ==========

Result<std::shared_ptr<ItemDurabilityTriggerInstance>> ItemDurabilityTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<ItemDurabilityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ItemDurabilityTrigger::trigger(ServerPlayer& player, const ItemStack& item, i32 oldDurability)
{
    // [TODO 阶段2+3：事件系统集成] 由 ItemDurabilityEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(oldDurability);
}

// ========== EnchantedItemTriggerInstance ==========

EnchantedItemTriggerInstance::EnchantedItemTriggerInstance(ItemPredicate item, IntBounds levels)
    : m_item(std::move(item))
    , m_levels(std::move(levels))
{}

bool EnchantedItemTriggerInstance::test(const ItemStack& item, i32 levels) const
{
    if (!m_item.test(item)) {
        return false;
    }
    if (!m_levels.test(levels)) {
        return false;
    }
    return true;
}

Result<void> EnchantedItemTriggerInstance::fromJson(const nlohmann::json& json)
{
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

nlohmann::json EnchantedItemTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_levels.isUnbounded()) {
        json["levels"] = m_levels.toJson();
    }

    return json;
}

// ========== EnchantedItemTrigger ==========

Result<std::shared_ptr<EnchantedItemTriggerInstance>> EnchantedItemTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<EnchantedItemTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EnchantedItemTrigger::trigger(ServerPlayer& player, const ItemStack& item, i32 levels)
{
    // [TODO 阶段2+3：事件系统集成] 由 EnchantItemEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(levels);
}

// ========== FilledBucketTriggerInstance ==========

FilledBucketTriggerInstance::FilledBucketTriggerInstance(ItemPredicate item)
    : m_item(std::move(item))
{}

bool FilledBucketTriggerInstance::test(const ItemStack& item) const
{
    return m_item.test(item);
}

Result<void> FilledBucketTriggerInstance::fromJson(const nlohmann::json& json)
{
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

nlohmann::json FilledBucketTriggerInstance::conditionsToJson() const
{
    if (!m_item.isAny()) {
        return {{"item", m_item.toJson()}};
    }
    return nullptr;
}

// ========== FilledBucketTrigger ==========

Result<std::shared_ptr<FilledBucketTriggerInstance>> FilledBucketTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<FilledBucketTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void FilledBucketTrigger::trigger(ServerPlayer& player, const ItemStack& item)
{
    // [TODO 阶段2+3：事件系统集成] 由 FilledBucketEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(item);
}

} // namespace mc::advancement
