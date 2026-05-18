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

#include "TameableEntity.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../entities/player/Player.hpp"

namespace mc {

TameableEntity::TameableEntity(EntityId id)
    : AnimalEntity(id)
{
    // 注册属性
    registerAttributes();
}

void TameableEntity::setTamed(bool tamed)
{
    if (m_tamed != tamed) {
        m_tamed = tamed;
        onTamed(tamed);
    }
}

void TameableEntity::setSitting(bool sitting)
{
    if (m_sitting != sitting) {
        m_sitting = sitting;
        // 坐下时停止移动
        if (sitting) {
            clearNavigation();
        }
    }
}

void TameableEntity::setAttackTarget(LivingEntity* target)
{
    m_attackTarget = target;
    if (target != nullptr) {
        setAngerTime(MAX_ANGER_TIME);
    }
}

void TameableEntity::setRevengeTarget(LivingEntity* target)
{
    if (target != nullptr) {
        m_revengeTargetId = target->id();
        m_revengeTimer = MAX_ANGER_TIME;
        setAngerTime(MAX_ANGER_TIME);
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* TameableEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(this->world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void TameableEntity::setAngry(bool angry)
{
    if (angry) {
        setAngerTime(MAX_ANGER_TIME);
    } else {
        setAngerTime(0);
        m_attackTarget = nullptr;
        m_revengeTargetId = std::nullopt;
    }
}

void TameableEntity::tick()
{
    AnimalEntity::tick();
    updateAnger();
}

void TameableEntity::updateAnger()
{
    if (m_angerTime > 0) {
        --m_angerTime;
        if (m_angerTime <= 0) {
            // 愤怒时间结束，清除攻击目标
            m_attackTarget = nullptr;
            m_revengeTargetId = std::nullopt;
        }
    }
}

void TameableEntity::registerGoals()
{
    // 基础目标由子类添加
    // 子类应该调用此方法然后添加自己的目标
    AnimalEntity::registerGoals();
}

void TameableEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 驯服动物的基础属性（子类可以覆盖）
    // 参考 MC 1.16.5 TameableEntity
    // 大多数驯服动物的属性由子类设置
}

Player* TameableEntity::getOwner() const
{
    // MC 1.16.5: TameableEntity.getOwner()
    // 通过主人ID在世界中查找玩家实体
    if (!m_ownerId.has_value()) {
        return nullptr;
    }

    IWorld* worldPtr = const_cast<IWorld*>(this->world());
    if (!worldPtr) {
        return nullptr;
    }

    // 获取所有玩家，查找匹配的主人
    std::vector<Entity*> players = worldPtr->getPlayers();
    for (Entity* entity : players) {
        if (entity == nullptr) {
            continue;
        }
        Player* player = dynamic_cast<Player*>(entity);
        if (player != nullptr && player->playerId() == m_ownerId.value()) {
            return player;
        }
    }

    return nullptr;
}

} // namespace mc
