#include "Potion.hpp"
#include "../../entity/effect/EffectType.hpp"

namespace mc {
namespace potion {

// ========== Potion 实现 ==========

Potion::Potion()
    : m_baseName("")
    , m_effects()
    , m_id(nullptr) {
}

Potion::Potion(std::string_view baseName)
    : m_baseName(baseName)
    , m_effects()
    , m_id(nullptr) {
}

Potion::Potion(std::string_view baseName, std::vector<entity::effect::EffectInstance> effects)
    : m_baseName(baseName)
    , m_effects(std::move(effects))
    , m_id(nullptr) {
}

Potion::Potion(const entity::effect::EffectInstance& effect)
    : m_baseName("")
    , m_effects({effect})
    , m_id(nullptr) {
}

bool Potion::hasInstantEffect() const {
    for (const auto& effect : m_effects) {
        // 瞬间治疗和瞬间伤害是瞬间效果
        if (effect.type() == entity::effect::EffectType::InstantHealth ||
            effect.type() == entity::effect::EffectType::InstantDamage) {
            return true;
        }
    }
    return false;
}

std::string Potion::getNamePrefixed(std::string_view prefix) const {
    if (m_baseName.empty()) {
        return std::string(prefix);
    }
    return std::string(prefix) + ".effect.minecraft." + m_baseName;
}

} // namespace potion
} // namespace mc
