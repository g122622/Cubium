/**
 * @file CombatEntry.cpp
 * @brief 战斗条目实现
 */

#include "CombatEntry.hpp"
#include <cmath>
#include <limits>

namespace mc {

CombatEntry::CombatEntry(std::unique_ptr<DamageSource> source, f32 damage, i32 timestamp,
                         f32 health, const String& fallSuffix, f32 fallDistance)
    : m_source(std::move(source))
    , m_damage(damage)
    , m_timestamp(timestamp)
    , m_health(health)
    , m_fallSuffix(fallSuffix)
    , m_fallDistance(fallDistance)
{
}

bool CombatEntry::isLivingSource() const {
    return m_source && m_source->isEntitySource();
}

bool CombatEntry::isPlayerSource() const {
    return m_source && m_source->isPlayerSource();
}

f32 CombatEntry::getDamageAmount() const {
    // 虚空伤害返回最大值
    if (m_source && m_source->type() == DamageType::OutOfWorld) {
        return std::numeric_limits<f32>::max();
    }
    return m_damage;
}

} // namespace mc
