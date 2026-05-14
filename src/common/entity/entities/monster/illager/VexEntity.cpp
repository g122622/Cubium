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

#include "VexEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include <memory>

namespace mc {

VexEntity::VexEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 恼鬼体型小
    // 注册属性（基类构造函数中调用 registerAttributes() 不会派发到子类）
    registerAttributes();
}

std::unique_ptr<Entity> VexEntity::create(IWorld* /*world*/)
{
    return std::make_unique<VexEntity>(LegacyEntityType::Unknown, 0);
}

void VexEntity::tick()
{
    // MC 1.16.5: 恼鬼在 tick 期间可以穿墙
    // 参考 VexEntity.tick() 行 62-71
    setNoClip(true);
    MonsterEntity::tick();
    setNoClip(false);

    // 恼鬼始终不受重力影响
    setNoGravity(true);

    // 更新生命时间
    if (m_limitedLife && m_lifeTime > 0) {
        m_lifeTime--;

        // 生命结束时造成饥饿伤害
        // MC 1.16.5: limitedLifeTicks <= 0 时攻击自己造成 1.0 饥饿伤害
        if (m_lifeTime <= 0) {
            m_lifeTime = 20; // 重置为 20 tick 防止连续伤害
            auto damageSource = DamageSources::starve();
            hurt(damageSource, 1.0f);
        }
    }
}

void VexEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // TODO: 恼鬼特有 AI 目标
    // - VexAttackGoal (近战攻击)
    // - VexChargeGoal (充电攻击)
    // - VexMoveGoal (穿墙移动)
}

void VexEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // MC 1.16.5 VexEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 14.0f);
    // MOVEMENT_SPEED: 使用默认值（恼鬼飞行速度由移动控制器控制）
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0f); // MC 1.16.5: 铁剑伤害为 4.0
    // FOLLOW_RANGE: 使用默认值
}

} // namespace mc
