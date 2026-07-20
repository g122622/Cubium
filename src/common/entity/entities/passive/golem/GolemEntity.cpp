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

#include "GolemEntity.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

GolemEntity::GolemEntity(EntityInstanceId id)
    : CreatureEntity(id)
{
    // 注册属性
    registerAttributes();
}

void GolemEntity::setRevengeTarget(LivingEntity* target)
{
    setAttackTarget(target);
    if (target) {
        m_angerTime = MAX_ANGER_TIME;
        m_revengeTimer = MAX_ANGER_TIME;
        m_revengeTargetId = target->id();
    } else {
        m_revengeTargetId = std::nullopt;
    }
}

LivingEntity* GolemEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void GolemEntity::setAngry(bool angry)
{
    if (angry) {
        m_angerTime = MAX_ANGER_TIME;
    } else {
        m_angerTime = 0;
        setAttackTarget(nullptr);
    }
}

void GolemEntity::tick()
{
    MobEntity::tick();

    // 更新愤怒状态
    updateAnger();
}

void GolemEntity::registerAttributes()
{
    // 调用父类方法
    MobEntity::registerAttributes();

    // 傀儡的基础属性
    // 子类会覆盖具体值
}

void GolemEntity::updateAnger()
{
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            setAttackTarget(nullptr);
        }
    }
}

} // namespace mc
