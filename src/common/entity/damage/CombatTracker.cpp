/**
 * @file CombatTracker.cpp
 * @brief 战斗追踪器实现
 */

#include "CombatTracker.hpp"
#include "../core/LivingEntity.hpp"
#include <algorithm>

namespace mc {

CombatTracker::CombatTracker(LivingEntity* owner)
    : m_owner(owner)
{
}

void CombatTracker::recordDamage(std::unique_ptr<DamageSource> source, f32 damage, u32 timestamp) {
    if (!source || damage <= 0.0f) {
        return;
    }

    // 清理过期条目
    cleanupOldEntries(timestamp);

    // 添加新条目
    m_entries.emplace_back(std::move(source), damage, timestamp);
    m_totalDamage += damage;

    // 增量更新最佳伤害记录 - 只需与新条目比较
    // 仅当清理后需要重新扫描时才调用完整的updateBestEntry()
    if (m_entries.size() == 1 || damage > m_entries[m_bestEntryIndex].damage()) {
        m_bestEntryIndex = m_entries.size() - 1;
    }
}

void CombatTracker::reset() {
    m_entries.clear();
    m_totalDamage = 0.0f;
    m_bestEntryIndex = 0;
}

const CombatEntry* CombatTracker::getLastEntry() const {
    if (m_entries.empty()) {
        return nullptr;
    }
    return &m_entries.back();
}

const CombatEntry* CombatTracker::getBestEntry() const {
    if (m_entries.empty()) {
        return nullptr;
    }
    if (m_bestEntryIndex >= m_entries.size()) {
        return &m_entries.front();
    }
    return &m_entries[m_bestEntryIndex];
}

Entity* CombatTracker::getLastAttacker() const {
    const CombatEntry* entry = getLastEntry();
    if (!entry || !entry->source()) {
        return nullptr;
    }
    return entry->source()->getEntity();
}

Entity* CombatTracker::getBestAttacker() const {
    const CombatEntry* entry = getBestEntry();
    if (!entry || !entry->source()) {
        return nullptr;
    }
    return entry->source()->getEntity();
}

String CombatTracker::getDeathMessage() const {
    if (!m_owner) {
        return "entity died";
    }

    const CombatEntry* bestEntry = getBestEntry();
    if (!bestEntry) {
        // 没有战斗记录，自然死亡
        return m_owner->getDisplayName() + " died";
    }

    const DamageSource* source = bestEntry->source();
    if (!source) {
        return m_owner->getDisplayName() + " died";
    }

    // 根据伤害来源类型生成死亡消息
    // TODO: 实现完整的死亡消息系统，支持多种死亡原因
    Entity* attacker = source->getEntity();
    String ownerName = m_owner->getDisplayName();

    if (attacker) {
        return ownerName + " was slain by " + attacker->getDisplayName();
    }

    // 环境伤害
    if (source->isFire()) {
        return ownerName + " burned to death";
    }
    if (source->isLava()) {
        return ownerName + " tried to swim in lava";
    }
    if (source->isDrown()) {
        return ownerName + " drowned";
    }
    if (source->isFall()) {
        return ownerName + " fell from a high place";
    }
    if (source->isExplosion()) {
        return ownerName + " blew up";
    }
    if (source->isMagic()) {
        return ownerName + " was killed by magic";
    }
    if (source->isStarve()) {
        return ownerName + " starved to death";
    }
    if (source->isCactus()) {
        return ownerName + " was pricked to death";
    }

    return ownerName + " died";
}

u32 CombatTracker::getCombatDuration() const {
    if (m_entries.empty()) {
        return 0;
    }
    return m_entries.back().timestamp() - m_entries.front().timestamp();
}

bool CombatTracker::isInCombat(u32 currentTime, u32 threshold) const {
    if (m_entries.empty()) {
        return false;
    }
    u32 lastDamageTime = m_entries.back().timestamp();
    return (currentTime - lastDamageTime) < threshold;
}

void CombatTracker::cleanupOldEntries(u32 currentTime) {
    // 移除超过100 tick（5秒）的条目
    constexpr u32 ENTRY_LIFETIME = COMBAT_TIMEOUT;

    auto it = std::remove_if(m_entries.begin(), m_entries.end(),
        [currentTime, ENTRY_LIFETIME](const CombatEntry& entry) {
            return (currentTime - entry.timestamp()) > ENTRY_LIFETIME;
        });

    // 需要重新计算移除的伤害
    for (auto removeIt = it; removeIt != m_entries.end(); ++removeIt) {
        m_totalDamage -= removeIt->damage();
    }

    m_entries.erase(it, m_entries.end());

    // 重新计算最佳伤害记录
    updateBestEntry();
}

void CombatTracker::updateBestEntry() {
    if (m_entries.empty()) {
        m_bestEntryIndex = 0;
        return;
    }

    f32 maxDamage = 0.0f;
    size_t bestIndex = 0;

    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].damage() > maxDamage) {
            maxDamage = m_entries[i].damage();
            bestIndex = i;
        }
    }

    m_bestEntryIndex = bestIndex;
}

} // namespace mc
