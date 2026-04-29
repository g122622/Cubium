/**
 * @file CombatTracker.cpp
 * @brief 战斗追踪器实现
 */

#include "CombatTracker.hpp"
#include "../core/LivingEntity.hpp"
#include "../core/Entity.hpp"
#include <algorithm>
#include <limits>

namespace mc {

CombatTracker::CombatTracker(LivingEntity* owner)
    : m_owner(owner)
{
}

void CombatTracker::trackDamage(DamageSource& source, f32 health, f32 damage) {
    if (damage <= 0.0f || !m_owner) {
        return;
    }

    i32 currentTime = m_owner->ticksExisted();

    // 先尝试重置（清理过期战斗）
    reset();

    // 计算摔落后缀
    calculateFallSuffix();

    // 创建战斗条目
    m_entries.emplace_back(
        source.clone(),
        damage,
        currentTime,
        health,
        m_fallSuffix,
        m_owner->fallDistance()
    );

    m_totalDamage += damage;
    m_lastDamageTime = currentTime;
    m_takingDamage = true;

    // 更新最佳伤害记录
    if (m_entries.size() == 1 || damage > m_entries[m_bestEntryIndex].damage()) {
        m_bestEntryIndex = m_entries.size() - 1;
    }

    // 如果来自生物且不在战斗中，进入战斗状态
    if (!m_inCombat && source.isEntitySource() && m_owner->isAlive()) {
        m_inCombat = true;
        m_combatStartTime = currentTime;
        m_combatEndTime = m_combatStartTime;
        // 注意：sendEnterCombat() 由 LivingEntity::actuallyHurt() 调用
    }
}

void CombatTracker::reset() {
    if (!m_owner) {
        return;
    }

    i32 currentTime = m_owner->ticksExisted();

    // MC 1.16.5: 如果在战斗中，300 tick 后重置；否则 100 tick 后重置
    i32 timeout = m_inCombat ? 300 : 100;

    if (m_takingDamage && (!m_owner->isAlive() || (currentTime - m_lastDamageTime) > timeout)) {
        bool wasInCombat = m_inCombat;
        m_takingDamage = false;
        m_inCombat = false;
        m_combatEndTime = currentTime;

        if (wasInCombat) {
            // 注意：sendEndCombat() 由 LivingEntity 处理
        }

        m_entries.clear();
        m_totalDamage = 0.0f;
        m_bestEntryIndex = 0;
    }
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
    CombatEntry* bestEntry = const_cast<CombatTracker*>(this)->getBestCombatEntry();
    if (!bestEntry || !bestEntry->source()) {
        return nullptr;
    }
    return bestEntry->source()->getEntity();
}

LivingEntity* CombatTracker::getBestAttackerLiving() const {
    Entity* attacker = getBestAttacker();
    if (!attacker) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(attacker);
}

String CombatTracker::getDeathMessage() const {
    if (!m_owner) {
        return "entity died";
    }

    String ownerName = m_owner->getDisplayName();

    // 检查是否有摔落伤害
    if (!m_entries.empty()) {
        const CombatEntry* fallEntry = nullptr;
        const CombatEntry* attackEntry = nullptr;
        f32 fallDamage = 0.0f;

        // 找到摔落伤害和攻击伤害
        for (const auto& entry : m_entries) {
            const DamageSource* source = entry.source();
            if (!source) continue;

            if (source->isFall()) {
                if (entry.damage() > fallDamage) {
                    fallDamage = entry.damage();
                    fallEntry = &entry;
                }
            } else if (source->isEntitySource()) {
                attackEntry = &entry;
            }
        }

        // 如果有攻击后有摔落，使用摔落死亡消息
        if (fallEntry && attackEntry && !fallEntry->fallSuffix().empty()) {
            Entity* attacker = attackEntry->source()->getEntity();
            if (attacker) {
                return ownerName + " fell from a high place whilst trying to escape " + attacker->getDisplayName();
            }
        }
    }

    const CombatEntry* bestEntry = getBestEntry();
    if (!bestEntry) {
        return ownerName + " died";
    }

    const DamageSource* source = bestEntry->source();
    if (!source) {
        return ownerName + " died";
    }

    // 根据伤害来源类型生成死亡消息
    Entity* attacker = source->getEntity();
    String deathKey = source->deathMessageKey();

    if (attacker) {
        // 使用带攻击者的死亡消息
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

i32 CombatTracker::getCombatDuration() const {
    if (m_entries.empty()) {
        return 0;
    }
    return m_entries.back().timestamp() - m_entries.front().timestamp();
}

void CombatTracker::calculateFallSuffix() {
    if (!m_owner) {
        m_fallSuffix.clear();
        return;
    }

    // MC 1.16.5: 根据位置确定摔落后缀
    // 如果在梯子、藤蔓、脚手架等上面摔落，后缀不同
    // TODO: 实现方块检测

    f32 fallDistance = m_owner->fallDistance();
    if (fallDistance > 0.0f) {
        m_fallSuffix = "fall";
    } else {
        m_fallSuffix.clear();
    }
}

void CombatTracker::cleanupOldEntries(i32 currentTime) {
    // 移除超过战斗超时时间的条目
    auto it = std::remove_if(m_entries.begin(), m_entries.end(),
        [currentTime, this](const CombatEntry& entry) {
            return (currentTime - entry.timestamp()) > COMBAT_TIMEOUT;
        });

    // 计算移除的伤害
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

CombatEntry* CombatTracker::getBestCombatEntry() {
    if (m_entries.empty()) {
        return nullptr;
    }

    // MC 1.16.5: 优先选择玩家造成的伤害
    CombatEntry* bestPlayerEntry = nullptr;
    f32 maxPlayerDamage = 0.0f;

    CombatEntry* bestMobEntry = nullptr;
    f32 maxMobDamage = 0.0f;

    for (auto& entry : m_entries) {
        if (!entry.source()) continue;

        if (entry.source()->isPlayerSource()) {
            if (entry.damage() > maxPlayerDamage) {
                maxPlayerDamage = entry.damage();
                bestPlayerEntry = &entry;
            }
        } else if (entry.source()->isEntitySource()) {
            if (entry.damage() > maxMobDamage) {
                maxMobDamage = entry.damage();
                bestMobEntry = &entry;
            }
        }
    }

    // 优先返回玩家
    if (bestPlayerEntry) {
        return bestPlayerEntry;
    }

    return bestMobEntry;
}

} // namespace mc
