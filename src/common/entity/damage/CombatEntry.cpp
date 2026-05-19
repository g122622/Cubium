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
 * @file CombatEntry.cpp
 * @brief 战斗条目实现
 */

#include "CombatEntry.hpp"
#include "../core/LivingEntity.hpp"
#include <cmath>
#include <limits>

namespace mc {

CombatEntry::CombatEntry(std::unique_ptr<DamageSource> source,
    f32 damage,
    i32 timestamp,
    f32 health,
    const std::string& fallSuffix,
    f32 fallDistance)
    : m_source(std::move(source))
    , m_damage(damage)
    , m_timestamp(timestamp)
    , m_health(health)
    , m_fallSuffix(fallSuffix)
    , m_fallDistance(fallDistance)
{}

bool CombatEntry::isLivingSource() const
{
    // MC 1.16.5: return this.damageSrc.getTrueSource() instanceof LivingEntity;
    // 检查真正的伤害来源是否是 LivingEntity
    if (!m_source) {
        return false;
    }
    Entity* trueSource = m_source->getTrueSource();
    return trueSource != nullptr && dynamic_cast<LivingEntity*>(trueSource) != nullptr;
}

bool CombatEntry::isPlayerSource() const
{
    return m_source && m_source->isPlayerSource();
}

f32 CombatEntry::getDamageAmount() const
{
    // MC 1.16.5 CombatEntry.getDamageAmount():
    // return this.damageSrc == DamageSource.OUT_OF_WORLD ? Float.MAX_VALUE : this.fallDistance;
    // 注意：这里返回的是 fallDistance，不是 damage！
    // 这个值用于计算摔落伤害与攻击伤害的关系
    if (m_source && m_source->type() == DamageType::OutOfWorld) {
        return std::numeric_limits<f32>::max();
    }
    return m_fallDistance;
}

} // namespace mc
