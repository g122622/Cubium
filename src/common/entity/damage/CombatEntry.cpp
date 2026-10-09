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
    f32 fallDistance,
    EntityInstanceId trueSourceId,
    EntityInstanceId directSourceId)
    : m_source(std::move(source))
    , m_damage(damage)
    , m_timestamp(timestamp)
    , m_health(health)
    , m_fallSuffix(fallSuffix)
    , m_fallDistance(fallDistance)
    , m_trueSourceId(trueSourceId)
    , m_directSourceId(directSourceId)
{}

CombatEntry::CombatEntry(CombatEntry&& other) noexcept
    : m_source(std::move(other.m_source))
    , m_damage(other.m_damage)
    , m_timestamp(other.m_timestamp)
    , m_health(other.m_health)
    , m_fallSuffix(std::move(other.m_fallSuffix))
    , m_fallDistance(other.m_fallDistance)
    , m_trueSourceId(other.m_trueSourceId)
    , m_directSourceId(other.m_directSourceId)
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
        m_trueSourceId = other.m_trueSourceId;
        m_directSourceId = other.m_directSourceId;
    }
    return *this;
}

bool CombatEntry::isLivingSource() const
{
    // 语义：本条目是否来自生物（用于死亡消息等处）。判断依据是「是否存在真凶实体」
    // 这一**静态**事实（id 非 INVALID），不解引用 m_source 内的裸实体指针——真凶析构后
    // 该指针悬垂，解引用即 UAF。调用方若要进一步确认「是不是 LivingEntity / 是否还活着」，
    // 应经 IWorld::getEntity(trueSourceId()) 校验后再判定。
    //
    // TODO: 严格对齐 vanilla 需要区分「真凶是 LivingEntity」与「真凶只是任意实体」，
    //       当前仅能区分「有/无真凶」。待有实际消费方需要该区分时再补 world 参数。
    return m_trueSourceId != INVALID_ENTITY_ID;
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
