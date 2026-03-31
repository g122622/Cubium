#include "EffectManager.hpp"
#include "../core/LivingEntity.hpp"

namespace mc {
namespace entity {
namespace effect {

// ============================================================================
// EffectManager 实现
// ============================================================================

bool EffectManager::addEffect(EffectInstance effect, LivingEntity& entity) {
    // 查找是否已存在相同类型的效果
    i32 index = findEffectIndex(effect.type());

    if (index >= 0) {
        // 已存在，尝试合并
        return m_effects[index].merge(effect);
    } else {
        // 新效果，添加并应用
        effect.apply(entity);
        m_effects.push_back(std::move(effect));
        return true;
    }
}

void EffectManager::removeEffect(EffectType type, LivingEntity& entity) {
    i32 index = findEffectIndex(type);
    if (index >= 0) {
        m_effects[index].remove(entity);
        m_effects.erase(m_effects.begin() + index);
    }
}

void EffectManager::removeAllEffects(LivingEntity& entity) {
    for (auto& effect : m_effects) {
        effect.remove(entity);
    }
    m_effects.clear();
}

const EffectInstance* EffectManager::getEffect(EffectType type) const {
    i32 index = findEffectIndex(type);
    return index >= 0 ? &m_effects[index] : nullptr;
}

EffectInstance* EffectManager::getEffect(EffectType type) {
    i32 index = findEffectIndex(type);
    return index >= 0 ? &m_effects[index] : nullptr;
}

bool EffectManager::hasEffect(EffectType type) const {
    return findEffectIndex(type) >= 0;
}

i32 EffectManager::getEffectLevel(EffectType type) const {
    const EffectInstance* effect = getEffect(type);
    return effect ? effect->getEffectLevel() : 0;
}

void EffectManager::tick(LivingEntity& entity) {
    // 从后向前遍历，以便安全移除过期效果
    for (i32 i = static_cast<i32>(m_effects.size()) - 1; i >= 0; --i) {
        if (!m_effects[i].tick(entity)) {
            // 效果过期，移除
            m_effects.erase(m_effects.begin() + i);
        }
    }
}

bool EffectManager::hasBeneficialEffect() const {
    for (const auto& effect : m_effects) {
        if (isBeneficialEffect(effect.type())) {
            return true;
        }
    }
    return false;
}

bool EffectManager::hasHarmfulEffect() const {
    for (const auto& effect : m_effects) {
        if (!isBeneficialEffect(effect.type())) {
            return true;
        }
    }
    return false;
}

i32 EffectManager::findEffectIndex(EffectType type) const {
    for (size_t i = 0; i < m_effects.size(); ++i) {
        if (m_effects[i].type() == type) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

} // namespace effect
} // namespace entity
} // namespace mc
