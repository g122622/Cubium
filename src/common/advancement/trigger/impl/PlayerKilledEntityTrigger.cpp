#include "PlayerKilledEntityTrigger.hpp"

namespace mc::advancement {

// ========== PlayerKilledEntityTrigger::Instance ==========

PlayerKilledEntityTrigger::Instance::Instance(EntityPredicate entity, DamageSourcePredicate killingBlow)
    : m_entity(std::move(entity))
    , m_killingBlow(std::move(killingBlow)) {
}

bool PlayerKilledEntityTrigger::Instance::test(
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

Result<void> PlayerKilledEntityTrigger::Instance::fromJson(const nlohmann::json& json) {
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

nlohmann::json PlayerKilledEntityTrigger::Instance::conditionsToJson() const {
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

Result<std::shared_ptr<PlayerKilledEntityTrigger::Instance>> PlayerKilledEntityTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
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

std::shared_ptr<PlayerKilledEntityTrigger::Instance> PlayerKilledEntityTrigger::entityKilled() {
    return std::make_shared<Instance>();
}

std::shared_ptr<PlayerKilledEntityTrigger::Instance> PlayerKilledEntityTrigger::entityKilled(const EntityPredicate& entity) {
    return std::make_shared<Instance>(entity, DamageSourcePredicate{});
}

std::shared_ptr<PlayerKilledEntityTrigger::Instance> PlayerKilledEntityTrigger::killedByEntity(const EntityPredicate& killer) {
    return std::make_shared<Instance>(killer, DamageSourcePredicate{});
}

// ========== EntityKilledPlayerTrigger::Instance ==========

EntityKilledPlayerTrigger::Instance::Instance(EntityPredicate entity, DamageSourcePredicate killingBlow)
    : m_entity(std::move(entity))
    , m_killingBlow(std::move(killingBlow)) {
}

bool EntityKilledPlayerTrigger::Instance::test(
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

Result<void> EntityKilledPlayerTrigger::Instance::fromJson(const nlohmann::json& json) {
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

nlohmann::json EntityKilledPlayerTrigger::Instance::conditionsToJson() const {
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

Result<std::shared_ptr<EntityKilledPlayerTrigger::Instance>> EntityKilledPlayerTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
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
