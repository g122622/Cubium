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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include <limits>
#include <memory>
#include <string>
#include <utility>

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

CombatEntry::CombatEntry(CombatEntry&& other) noexcept
    : m_source(std::move(other.m_source))
    , m_damage(other.m_damage)
    , m_timestamp(other.m_timestamp)
    , m_health(other.m_health)
    , m_fallSuffix(std::move(other.m_fallSuffix))
    , m_fallDistance(other.m_fallDistance)
{}

CombatEntry& CombatEntry::operator=(CombatEntry&& other) noexcept
{
    if (this != &other) {
        m_source = std::move(other.m_source);
        m_damage = other.m_damage;
        m_timestamp = other.m_timestamp;
        m_health = other.m_health;
        m_fallSuffix = std::move(other.m_fallSuffix);
        m_fallDistance = other.m_fallDistance;
    }
    return *this;
}

bool CombatEntry::isLivingSource() const
{
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
    // 虚空伤害返回最大值，其他返回摔落距离
    // 用于计算摔落伤害与攻击伤害的关系
    if (m_source && m_source->type() == DamageType::OutOfWorld) {
        return std::numeric_limits<f32>::max();
    }
    return m_fallDistance;
}

} // namespace mc
