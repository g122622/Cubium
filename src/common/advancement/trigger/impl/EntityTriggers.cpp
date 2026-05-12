#include "EntityTriggers.hpp"
#include "common/util/assert/AssertAll.hpp"

// 注意：trigger() 方法的完整实现需要服务端模块的支持
// 服务端代码应包含 server/advancement/TriggerInstantiation.hpp
// 并使用 triggerWithPredicate() 方法或直接调用基类的 trigger() 模板方法

namespace mc::advancement {

// ========== TameAnimalTriggerInstance ==========

TameAnimalTriggerInstance::TameAnimalTriggerInstance(EntityPredicate entity)
    : m_entity(std::move(entity)) {
}

bool TameAnimalTriggerInstance::test(const Entity& entity) const {
    return m_entity.test(entity);
}

Result<void> TameAnimalTriggerInstance::fromJson(const nlohmann::json& json) {
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

nlohmann::json TameAnimalTriggerInstance::conditionsToJson() const {
    if (!m_entity.isAny()) {
        return {{"entity", m_entity.toJson()}};
    }
    return nullptr;
}

// ========== TameAnimalTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> TameAnimalTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<TameAnimalTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void TameAnimalTrigger::trigger(ServerPlayer& player, const Entity& entity) {
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端代码应使用以下方式之一触发检测：
    //
    // 方法1：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<TameAnimalTrigger>();
    // trigger->AbstractCriterionTrigger<TameAnimalTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&entity](const TameAnimalTriggerInstance& instance) {
    //         return instance.test(entity);
    //     }
    // );
    //
    // 方法2：在 setTamedBy 中直接调用（推荐服务端代码处理）
    // 参考：server/advancement/AdvancementEventHandler.hpp
    MC_UNUSED(player);
    MC_UNUSED(entity);
}

// ========== BredAnimalsTriggerInstance ==========

BredAnimalsTriggerInstance::BredAnimalsTriggerInstance(EntityPredicate child, EntityPredicate parent, EntityPredicate partner)
    : m_child(std::move(child))
    , m_parent(std::move(parent))
    , m_partner(std::move(partner)) {
}

bool BredAnimalsTriggerInstance::test(const Entity& child, const Entity& parent, const Entity& partner) const {
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

Result<void> BredAnimalsTriggerInstance::fromJson(const nlohmann::json& json) {
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

nlohmann::json BredAnimalsTriggerInstance::conditionsToJson() const {
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

// ========== BredAnimalsTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> BredAnimalsTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<BredAnimalsTriggerInstance>();
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

// ========== SummonedEntityTriggerInstance ==========

SummonedEntityTriggerInstance::SummonedEntityTriggerInstance(EntityPredicate entity)
    : m_entity(std::move(entity)) {
}

bool SummonedEntityTriggerInstance::test(const Entity& entity) const {
    return m_entity.test(entity);
}

Result<void> SummonedEntityTriggerInstance::fromJson(const nlohmann::json& json) {
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

nlohmann::json SummonedEntityTriggerInstance::conditionsToJson() const {
    if (!m_entity.isAny()) {
        return {{"entity", m_entity.toJson()}};
    }
    return nullptr;
}

// ========== SummonedEntityTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> SummonedEntityTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<SummonedEntityTriggerInstance>();
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

// ========== CuredZombieVillagerTriggerInstance ==========

CuredZombieVillagerTriggerInstance::CuredZombieVillagerTriggerInstance(EntityPredicate zombie, EntityPredicate villager)
    : m_zombie(std::move(zombie))
    , m_villager(std::move(villager)) {
}

bool CuredZombieVillagerTriggerInstance::test(const Entity& zombie, const Entity& villager) const {
    if (!m_zombie.test(zombie)) {
        return false;
    }
    if (!m_villager.test(villager)) {
        return false;
    }
    return true;
}

Result<void> CuredZombieVillagerTriggerInstance::fromJson(const nlohmann::json& json) {
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

nlohmann::json CuredZombieVillagerTriggerInstance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_zombie.isAny()) {
        json["zombie"] = m_zombie.toJson();
    }
    if (!m_villager.isAny()) {
        json["villager"] = m_villager.toJson();
    }

    return json;
}

// ========== CuredZombieVillagerTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> CuredZombieVillagerTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<CuredZombieVillagerTriggerInstance>();
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

// ========== VillagerTradeTriggerInstance ==========

VillagerTradeTriggerInstance::VillagerTradeTriggerInstance(EntityPredicate villager, ItemPredicate item)
    : m_villager(std::move(villager))
    , m_item(std::move(item)) {
}

bool VillagerTradeTriggerInstance::test(const Entity& villager, const ItemStack& item) const {
    if (!m_villager.test(villager)) {
        return false;
    }
    if (!m_item.test(item)) {
        return false;
    }
    return true;
}

Result<void> VillagerTradeTriggerInstance::fromJson(const nlohmann::json& json) {
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

nlohmann::json VillagerTradeTriggerInstance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_villager.isAny()) {
        json["villager"] = m_villager.toJson();
    }
    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }

    return json;
}

// ========== VillagerTradeTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> VillagerTradeTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<VillagerTradeTriggerInstance>();
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
