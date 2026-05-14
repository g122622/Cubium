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

#include "RavagerEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

RavagerEntity::RavagerEntity(LegacyEntityType type, EntityId id)
    : AbstractRaiderEntity(type, id)
{
    // 劫掠兽体型大 - 通过 width()/height() 设置
}

std::unique_ptr<Entity> RavagerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<RavagerEntity>(LegacyEntityType::Unknown, 0);
}

void RavagerEntity::startRoaring()
{
    m_roaring = true;
    m_roarTime = ROAR_DURATION;
}

void RavagerEntity::startCharging()
{
    m_charging = true;
    m_chargeTime = CHARGE_DURATION;
}

void RavagerEntity::tick()
{
    AbstractRaiderEntity::tick();

    // 更新咆哮状态
    if (m_roaring && m_roarTime > 0) {
        m_roarTime--;
        if (m_roarTime <= 0) {
            m_roaring = false;
        }
    }

    // 更新冲撞状态
    if (m_charging && m_chargeTime > 0) {
        m_chargeTime--;

        // 冲撞时破坏方块
        if (m_canBreakBlocks) {
            // TODO: 破坏前方方块
        }

        if (m_chargeTime <= 0) {
            m_charging = false;
        }
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }
}

void RavagerEntity::registerGoals()
{
    AbstractRaiderEntity::registerGoals();

    // TODO: 劫掠兽特有 AI 目标
    // - RavagerAttackGoal (近战攻击)
    // - RavagerChargeGoal (冲撞)
    // - RavagerRoarGoal (咆哮)
}

void RavagerEntity::registerAttributes()
{
    AbstractRaiderEntity::registerAttributes();

    // 劫掠兽属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 32.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.75f);
}

} // namespace mc
