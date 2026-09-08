/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file CombatTracker.cpp
 * @brief 战斗追踪器实现
 */

#include "CombatTracker.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../../world/block/BlockState.hpp"
#include "../core/Entity.hpp"
#include "../core/LivingEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/damage/CombatEntry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>

namespace mc {

CombatTracker::CombatTracker(LivingEntity* owner)
    : m_owner(owner)
{}

void CombatTracker::trackDamage(DamageSource& source, f32 health, f32 damage)
{
    if (damage <= 0.0f || !m_owner) {
        return;
    }

    i32 currentTime = m_owner->ticksExisted();

    // 先尝试重置（清理过期战斗）
    reset();

    // 计算摔落后缀
    _calculateFallSuffix();

    // 创建战斗条目
    m_entries.emplace_back(source.clone(), damage, currentTime, health, m_fallSuffix, m_owner->fallDistance());

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

void CombatTracker::reset()
{
    if (!m_owner) {
        return;
    }

    i32 currentTime = m_owner->ticksExisted();

    // 如果在战斗中，300 tick 后重置；否则 100 tick 后重置
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

void CombatTracker::recheckStatus()
{
    // 对齐 MC Java 1.21.11 CombatTracker.recheckStatus（CombatTracker.java:145-158）：
    //   int i = this.inCombat ? 300 : 100;
    //   if (this.takingDamage && (!this.mob.isAlive() || this.mob.tickCount - this.lastDamageTime > i)) {
    //       boolean flag = this.inCombat;
    //       this.takingDamage = false;
    //       this.inCombat = false;
    //       this.combatEndTime = this.mob.tickCount;
    //       if (flag) { this.mob.onLeaveCombat(); }
    //       this.entries.clear();
    //   }
    // Cubium 的 reset() 已实现等价逻辑，此处复用以保持单一实现源。
    // 注意：原版 recheckStatus 不重新计算 bestEntry——1.21.11 的 CombatTracker 没有 bestEntry 概念。
    reset();
}

const CombatEntry* CombatTracker::getLastEntry() const
{
    if (m_entries.empty()) {
        return nullptr;
    }
    return &m_entries.back();
}

const CombatEntry* CombatTracker::getBestEntry() const
{
    if (m_entries.empty()) {
        return nullptr;
    }
    if (m_bestEntryIndex >= m_entries.size()) {
        return &m_entries.front();
    }
    return &m_entries[m_bestEntryIndex];
}

Entity* CombatTracker::getLastAttacker() const
{
    const CombatEntry* entry = getLastEntry();
    if (!entry || !entry->source()) {
        return nullptr;
    }
    return entry->source()->getEntity();
}

Entity* CombatTracker::getBestAttacker() const
{
    // 找到造成最多伤害的生物和玩家
    // 只有当玩家伤害 >= 生物总伤害的 1/3 时才返回玩家

    LivingEntity* bestMob = nullptr;
    LivingEntity* bestPlayer = nullptr;
    f32 mobDamage = 0.0f;
    f32 playerDamage = 0.0f;

    for (const auto& entry : m_entries) {
        if (!entry.source()) continue;

        Entity* trueSource = entry.source()->getTrueSource();
        if (!trueSource) continue;

        LivingEntity* livingSource = dynamic_cast<LivingEntity*>(trueSource);
        if (!livingSource) continue;

        // 使用 getDamageAmount() 而不是 damage()
        // 因为虚空伤害返回 Float.MAX_VALUE
        f32 damage = entry.getDamageAmount();

        if (entry.source()->isPlayerSource()) {
            // 玩家来源
            if (damage > playerDamage) {
                playerDamage = damage;
                bestPlayer = livingSource;
            }
        } else {
            // 其他生物来源
            if (damage > mobDamage) {
                mobDamage = damage;
                bestMob = livingSource;
            }
        }
    }

    // 只有当玩家伤害 >= 生物总伤害的 1/3 时才返回玩家
    if (bestPlayer != nullptr && playerDamage >= mobDamage / 3.0f) {
        return bestPlayer;
    }

    return bestMob;
}

LivingEntity* CombatTracker::getBestAttackerLiving() const
{
    // 直接调用 getBestAttacker() 并转换为 LivingEntity
    Entity* attacker = getBestAttacker();
    if (!attacker) {
        return nullptr;
    }
    // getBestAttacker() 只返回 LivingEntity，所以这个转换应该成功
    return dynamic_cast<LivingEntity*>(attacker);
}

std::string CombatTracker::getDeathMessage() const
{
    if (!m_owner) {
        return "entity died";
    }

    std::string ownerName = m_owner->getDisplayName()->getUnformattedText();

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
                return ownerName + " fell from a high place whilst trying to escape " +
                    attacker->getDisplayName()->getUnformattedText();
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
    std::string deathKey = source->deathMessageKey();

    if (attacker) {
        // 使用带攻击者的死亡消息
        return ownerName + " was slain by " + attacker->getDisplayName()->getUnformattedText();
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

i32 CombatTracker::getCombatDuration() const
{
    if (m_entries.empty()) {
        return 0;
    }
    return m_entries.back().timestamp() - m_entries.front().timestamp();
}

void CombatTracker::_calculateFallSuffix()
{
    if (!m_owner) {
        m_fallSuffix.clear();
        return;
    }

    // 根据攀爬位置确定摔落后缀
    // 如果在梯子、藤蔓、脚手架等上面摔落，后缀不同

    // 重置摔落后缀
    m_fallSuffix.clear();

    // 获取攀爬位置
    const std::optional<BlockPos>& climbPos = m_owner->getLastClimbPos();

    if (climbPos.has_value()) {
        // 有攀爬位置，检查该位置的方块类型
        IWorld* world = m_owner->world();
        if (world == nullptr) {
            return;
        }

        const BlockState* blockState = world->getBlockState(*climbPos);
        if (blockState == nullptr) {
            return;
        }

        const Block& block = blockState->getBlock();
        const ResourceLocation& blockId = block.blockLocation();

        // 检查方块类型
        // 优先检查梯子和活板门
        if (blockId == ResourceLocation("minecraft", "ladder")) {
            m_fallSuffix = "ladder";
            return;
        }

        // 检查是否是活板门（通过BlockTags检查）
        // 由于活板门在isLadder中检查了OPEN状态，攀爬位置记录的活板门一定是打开的
        const std::string& namespace_ = blockId.namespace_();
        const std::string& path = blockId.path();
        if (namespace_ == "minecraft" &&
            (path.find("trapdoor") != std::string::npos || path.find("_trapdoor") != std::string::npos)) {
            m_fallSuffix = "ladder";
            return;
        }

        // 检查藤蔓
        if (blockId == ResourceLocation("minecraft", "vine")) {
            m_fallSuffix = "vines";
            return;
        }

        // 检查垂泪藤
        if (blockId == ResourceLocation("minecraft", "weeping_vines") ||
            blockId == ResourceLocation("minecraft", "weeping_vines_plant")) {
            m_fallSuffix = "weeping_vines";
            return;
        }

        // 检查扭曲藤
        if (blockId == ResourceLocation("minecraft", "twisting_vines") ||
            blockId == ResourceLocation("minecraft", "twisting_vines_plant")) {
            m_fallSuffix = "twisting_vines";
            return;
        }

        // 检查脚手架
        if (blockId == ResourceLocation("minecraft", "scaffolding")) {
            m_fallSuffix = "scaffolding";
            return;
        }

        // 其他可攀爬方块
        m_fallSuffix = "other_climbable";
    } else if (m_owner->isInWater()) {
        // 如果在水中，返回 water 后缀
        m_fallSuffix = "water";
    }
    // 没有攀爬位置也不在水中，保持空后缀
}

void CombatTracker::_cleanupOldEntries(i32 currentTime)
{
    // 移除超过战斗超时时间的条目
    auto it = std::remove_if(m_entries.begin(), m_entries.end(), [currentTime, this](const CombatEntry& entry) {
        return (currentTime - entry.timestamp()) > COMBAT_TIMEOUT;
    });

    // 计算移除的伤害
    for (auto removeIt = it; removeIt != m_entries.end(); ++removeIt) {
        m_totalDamage -= removeIt->damage();
    }

    m_entries.erase(it, m_entries.end());

    // 重新计算最佳伤害记录
    _updateBestEntry();
}

void CombatTracker::_updateBestEntry()
{
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

CombatEntry* CombatTracker::_getBestCombatEntry()
{
    // 找到最佳战斗条目，用于摔落组合死亡消息

    if (m_entries.empty()) {
        return nullptr;
    }

    // 从后往前找摔落或虚空伤害
    for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it) {
        const DamageSource* source = it->source();
        if (!source) continue;

        // 检查是否是摔落或虚空伤害
        bool isFallOrVoid = source->isFall() || source->type() == DamageType::OutOfWorld;

        if (isFallOrVoid) {
            // 找到了摔落/虚空伤害，现在往前找攻击条目
            f32 fallDamage = it->getDamageAmount();

            // 如果摔落伤害 > 5.0，找之前的攻击条目
            if (fallDamage > 5.0f) {
                // 从当前条目往前找实体攻击
                for (auto prevIt = std::next(it); prevIt != m_entries.rend(); ++prevIt) {
                    const DamageSource* prevSource = prevIt->source();
                    if (prevSource && prevSource->isEntitySource()) {
                        return &(*prevIt);
                    }
                }
            }

            // 如果有 fallSuffix 且伤害 > 5.0F，返回这个条目
            if (!it->fallSuffix().empty() && fallDamage > 5.0f) {
                return &(*it);
            }
        }
    }

    // 没有找到摔落/虚空伤害，返回最后的实体攻击条目
    for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it) {
        const DamageSource* source = it->source();
        if (source && source->isEntitySource()) {
            return &(*it);
        }
    }

    return nullptr;
}

} // namespace mc
