#include "PlayerKilledEntityTrigger.hpp"

namespace mc::advancement {

// ========== PlayerKilledEntityTriggerInstance ==========

PlayerKilledEntityTriggerInstance::PlayerKilledEntityTriggerInstance(EntityPredicate entity, DamageSourcePredicate killingBlow)
    : m_entity(std::move(entity))
    , m_killingBlow(std::move(killingBlow)) {
}

bool PlayerKilledEntityTriggerInstance::test(
    ServerPlayer& player,
    const Entity& entity,
    const DamageSource& source
) const {
    // 检查实体谓词
    if (!m_entity.test(entity, source)) {
        return false;
    }

    // 检查伤害源谓词
    if (!m_killingBlow.test(source)) {
        return false;
    }

    return true;
}

Result<void> PlayerKilledEntityTriggerInstance::fromJson(const nlohmann::json& json) {
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

    if (json.contains("killing_blow")) {
        auto result = DamageSourcePredicate::fromJson(json["killing_blow"]);
        if (result.failed()) {
            return result.error();
        }
        m_killingBlow = result.value();
    }

    return {};
}

nlohmann::json PlayerKilledEntityTriggerInstance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_entity.isAny()) {
        json["entity"] = m_entity.toJson();
    }
    if (!m_killingBlow.isAny()) {
        json["killing_blow"] = m_killingBlow.toJson();
    }

    return json;
}

// ========== PlayerKilledEntityTrigger ==========

Result<std::shared_ptr<PlayerKilledEntityTriggerInstance>> PlayerKilledEntityTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<PlayerKilledEntityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlayerKilledEntityTrigger::trigger(
    ServerPlayer& player,
    const Entity& entity,
    const DamageSource& source
) {
    for (const auto& listener : getListeners(*player.getAdvancements())) {
        if (listener.getInstance().test(player, entity, source)) {
            listener.grantCriterion(*player.getAdvancements());
        }
    }
}

std::shared_ptr<PlayerKilledEntityTriggerInstance> PlayerKilledEntityTrigger::entityKilled() {
    return std::make_shared<PlayerKilledEntityTriggerInstance>();
}

std::shared_ptr<PlayerKilledEntityTriggerInstance> PlayerKilledEntityTrigger::entityKilled(const EntityPredicate& entity) {
    return std::make_shared<PlayerKilledEntityTriggerInstance>(entity, DamageSourcePredicate{});
}

std::shared_ptr<PlayerKilledEntityTriggerInstance> PlayerKilledEntityTrigger::killedByEntity(const EntityPredicate& killer) {
    return std::make_shared<PlayerKilledEntityTriggerInstance>(killer, DamageSourcePredicate{});
}

// ========== EntityKilledPlayerTriggerInstance ==========

EntityKilledPlayerTriggerInstance::EntityKilledPlayerTriggerInstance(EntityPredicate entity, DamageSourcePredicate killingBlow)
    : m_entity(std::move(entity))
    , m_killingBlow(std::move(killingBlow)) {
}

bool EntityKilledPlayerTriggerInstance::test(
    ServerPlayer& player,
    const Entity& entity,
    const DamageSource& source
) const {
    MC_UNUSED(player);
    if (!m_entity.test(entity, source)) {
        return false;
    }
    if (!m_killingBlow.test(source)) {
        return false;
    }
    return true;
}

Result<void> EntityKilledPlayerTriggerInstance::fromJson(const nlohmann::json& json) {
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

    if (json.contains("killing_blow")) {
        auto result = DamageSourcePredicate::fromJson(json["killing_blow"]);
        if (result.failed()) {
            return result.error();
        }
        m_killingBlow = result.value();
    }

    return {};
}

nlohmann::json EntityKilledPlayerTriggerInstance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_entity.isAny()) {
        json["entity"] = m_entity.toJson();
    }
    if (!m_killingBlow.isAny()) {
        json["killing_blow"] = m_killingBlow.toJson();
    }

    return json;
}

// ========== EntityKilledPlayerTrigger ==========

Result<std::shared_ptr<EntityKilledPlayerTriggerInstance>> EntityKilledPlayerTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<EntityKilledPlayerTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EntityKilledPlayerTrigger::trigger(
    ServerPlayer& player,
    const Entity& entity,
    const DamageSource& source
) {
    for (const auto& listener : getListeners(*player.getAdvancements())) {
        if (listener.getInstance().test(player, entity, source)) {
            listener.grantCriterion(*player.getAdvancements());
        }
    }
}

} // namespace mc::advancement
