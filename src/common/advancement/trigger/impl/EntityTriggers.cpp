#include "EntityTriggers.hpp"

namespace mc::advancement {

// ========== TameAnimalTrigger ==========

TameAnimalTrigger::Instance::Instance(EntityPredicate entity)
    : m_entity(std::move(entity)) {
}

bool TameAnimalTrigger::Instance::test(const Entity& entity) const {
    return m_entity.test(entity);
}

Result<void> TameAnimalTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("entity")) {
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = result.value();
    }

    return {};
}

nlohmann::json TameAnimalTrigger::Instance::conditionsToJson() const {
    if (!m_entity.isAny()) {
        return {{"entity", m_entity.toJson()}};
    }
    return nullptr;
}

Result<std::shared_ptr<TameAnimalTrigger::Instance>> TameAnimalTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void TameAnimalTrigger::trigger(ServerPlayer& player, const Entity& entity) {
    // [TODO 阶段2+3：事件系统集成] 由 TameAnimalEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(entity);
}

// ========== BredAnimalsTrigger ==========

BredAnimalsTrigger::Instance::Instance(EntityPredicate child, EntityPredicate parent, EntityPredicate partner)
    : m_child(std::move(child))
    , m_parent(std::move(parent))
    , m_partner(std::move(partner)) {
}

bool BredAnimalsTrigger::Instance::test(const Entity& child, const Entity& parent, const Entity& partner) const {
    if (!m_child.test(child)) {
        return false;
    }
    if (!m_parent.test(parent)) {
        return false;
    }
    if (!m_partner.test(partner)) {
        return false;
    }
    return true;
}

Result<void> BredAnimalsTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("child")) {
        auto result = EntityPredicate::fromJson(json["child"]);
        if (result.failed()) {
            return result.error();
        }
        m_child = result.value();
    }

    if (json.contains("parent")) {
        auto result = EntityPredicate::fromJson(json["parent"]);
        if (result.failed()) {
            return result.error();
        }
        m_parent = result.value();
    }

    if (json.contains("partner")) {
        auto result = EntityPredicate::fromJson(json["partner"]);
        if (result.failed()) {
            return result.error();
        }
        m_partner = result.value();
    }

    return {};
}

nlohmann::json BredAnimalsTrigger::Instance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_child.isAny()) {
        json["child"] = m_child.toJson();
    }
    if (!m_parent.isAny()) {
        json["parent"] = m_parent.toJson();
    }
    if (!m_partner.isAny()) {
        json["partner"] = m_partner.toJson();
    }

    return json;
}

Result<std::shared_ptr<BredAnimalsTrigger::Instance>> BredAnimalsTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void BredAnimalsTrigger::trigger(
    ServerPlayer& player,
    const Entity& child,
    const Entity& parent,
    const Entity& partner
) {
    // [TODO 阶段2+3：事件系统集成] 由 BredAnimalsEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(child);
    MC_UNUSED(parent);
    MC_UNUSED(partner);
}

// ========== SummonedEntityTrigger ==========

SummonedEntityTrigger::Instance::Instance(EntityPredicate entity)
    : m_entity(std::move(entity)) {
}

bool SummonedEntityTrigger::Instance::test(const Entity& entity) const {
    return m_entity.test(entity);
}

Result<void> SummonedEntityTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("entity")) {
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = result.value();
    }

    return {};
}

nlohmann::json SummonedEntityTrigger::Instance::conditionsToJson() const {
    if (!m_entity.isAny()) {
        return {{"entity", m_entity.toJson()}};
    }
    return nullptr;
}

Result<std::shared_ptr<SummonedEntityTrigger::Instance>> SummonedEntityTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void SummonedEntityTrigger::trigger(ServerPlayer& player, const Entity& entity) {
    // [TODO 阶段2+3：事件系统集成] 由 SummonedEntityEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(entity);
}
}

// ========== CuredZombieVillagerTrigger ==========

CuredZombieVillagerTrigger::Instance::Instance(EntityPredicate zombie, EntityPredicate villager)
    : m_zombie(std::move(zombie))
    , m_villager(std::move(villager)) {
}

bool CuredZombieVillagerTrigger::Instance::test(const Entity& zombie, const Entity& villager) const {
    if (!m_zombie.test(zombie)) {
        return false;
    }
    if (!m_villager.test(villager)) {
        return false;
    }
    return true;
}

Result<void> CuredZombieVillagerTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("zombie")) {
        auto result = EntityPredicate::fromJson(json["zombie"]);
        if (result.failed()) {
            return result.error();
        }
        m_zombie = result.value();
    }

    if (json.contains("villager")) {
        auto result = EntityPredicate::fromJson(json["villager"]);
        if (result.failed()) {
            return result.error();
        }
        m_villager = result.value();
    }

    return {};
}

nlohmann::json CuredZombieVillagerTrigger::Instance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_zombie.isAny()) {
        json["zombie"] = m_zombie.toJson();
    }
    if (!m_villager.isAny()) {
        json["villager"] = m_villager.toJson();
    }

    return json;
}

Result<std::shared_ptr<CuredZombieVillagerTrigger::Instance>> CuredZombieVillagerTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void CuredZombieVillagerTrigger::trigger(ServerPlayer& player, const Entity& zombie, const Entity& villager) {
    // [TODO 阶段2+3：事件系统集成] 由 CuredZombieVillagerEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(zombie);
    MC_UNUSED(villager);
}

// ========== VillagerTradeTrigger ==========

VillagerTradeTrigger::Instance::Instance(EntityPredicate villager, ItemPredicate item)
    : m_villager(std::move(villager))
    , m_item(std::move(item)) {
}

bool VillagerTradeTrigger::Instance::test(const Entity& villager, const ItemStack& item) const {
    if (!m_villager.test(villager)) {
        return false;
    }
    if (!m_item.test(item)) {
        return false;
    }
    return true;
}

Result<void> VillagerTradeTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("villager")) {
        auto result = EntityPredicate::fromJson(json["villager"]);
        if (result.failed()) {
            return result.error();
        }
        m_villager = result.value();
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

nlohmann::json VillagerTradeTrigger::Instance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_villager.isAny()) {
        json["villager"] = m_villager.toJson();
    }
    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }

    return json;
}

Result<std::shared_ptr<VillagerTradeTrigger::Instance>> VillagerTradeTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void VillagerTradeTrigger::trigger(ServerPlayer& player, const Entity& villager, const ItemStack& item) {
    // [TODO 阶段2+3：事件系统集成] 由 VillagerTradeEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(villager);
    MC_UNUSED(item);
}

} // namespace mc::advancement
