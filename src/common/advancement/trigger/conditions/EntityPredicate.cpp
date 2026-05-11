#include "EntityPredicate.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== MobEffectsPredicate ==========

bool MobEffectsPredicate::test(const Entity& entity) const {
    if (m_isAny) {
        return true;
    }
    // TODO: 检查效果
    return true;
}

Result<MobEffectsPredicate> MobEffectsPredicate::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    return MobEffectsPredicate{};
}

nlohmann::json MobEffectsPredicate::toJson() const {
    return nullptr;
}

// ========== EntityPredicate ==========

bool EntityPredicate::test(const Entity& entity) const {
    if (m_isAny) {
        return true;
    }

    // 检查实体类型
    if (m_type.has_value()) {
        // TODO: 获取实体类型ID进行比较
        // if (entity.getType().getId() != m_type.value()) return false;
    }

    // TODO: 检查其他条件
    return true;
}

bool EntityPredicate::test(const Entity& entity, const DamageSource& source) const {
    MC_UNUSED(source);
    return test(entity);
}

Result<EntityPredicate> EntityPredicate::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return EntityPredicate{};
    }

    std::optional<ResourceLocation> type;

    if (json.contains("type")) {
        type = ResourceLocation(json["type"].get<std::string>());
    }

    // TODO: 解析其他条件

    EntityPredicate predicate;
    predicate.m_type = std::move(type);
    predicate.m_isAny = !predicate.m_type.has_value();
    return predicate;
}

nlohmann::json EntityPredicate::toJson() const {
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_type.has_value()) {
        json["type"] = m_type.value().toString();
    }
    return json;
}

// ========== DamageSourcePredicate ==========

bool DamageSourcePredicate::test(const DamageSource& source) const {
    if (m_isAny) {
        return true;
    }
    // TODO: 检查伤害源
    MC_UNUSED(source);
    return true;
}

Result<DamageSourcePredicate> DamageSourcePredicate::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    return DamageSourcePredicate{};
}

nlohmann::json DamageSourcePredicate::toJson() const {
    return nullptr;
}

} // namespace mc::advancement
