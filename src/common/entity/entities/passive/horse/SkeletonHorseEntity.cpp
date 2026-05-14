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

#include "SkeletonHorseEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

SkeletonHorseEntity::SkeletonHorseEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 骷髅马默认已驯服
    setTame(true);
    // 设置跳跃强度
    setJumpStrength(1.0f);
}

std::unique_ptr<Entity> SkeletonHorseEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SkeletonHorseEntity>(LegacyEntityType::Unknown, 0);
}

bool SkeletonHorseEntity::canBeRiddenBy(Player* player) const
{
    // 骷髅马不需要驯服即可骑乘
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }
    return true;
}

void SkeletonHorseEntity::setTrap(bool trap)
{
    m_trap = trap;
}

void SkeletonHorseEntity::triggerTrap()
{
    // MC 1.16.5: 触发陷阱
    // 1. 将陷阱马转换为普通骷髅马
    // 2. 生成骷髅骑手
    // TODO: 实现完整的陷阱触发逻辑
    // - 生成骷髅实体
    // - 给骷髅装备铁头盔
    // - 让骷髅骑上马
    // - 如果世界难度为 HARD，生成额外 2 只骷髅
    m_trap = false;
}

void SkeletonHorseEntity::onStruckByLightning()
{
    // MC 1.16.5: 陷阱马被闪电击中时触发陷阱
    if (m_trap) {
        triggerTrap();
    }
}

void SkeletonHorseEntity::tick()
{
    AbstractHorseEntity::tick();

    // MC 1.16.5: 陷阱马超时逻辑
    // 如果陷阱马存在超过 18000 ticks (15分钟)，自动消失
    if (m_trap) {
        m_trapTime++;
        if (m_trapTime >= TRAP_MAX_TIME) {
            remove();
            return;
        }
    }

    // 检查阳光燃烧
    if (shouldBurnInDaylight()) {
        // TODO: 实现阳光燃烧逻辑
    }
}

void SkeletonHorseEntity::registerGoals()
{
    AbstractHorseEntity::registerGoals();
    // 骷髅马没有额外 AI
}

void SkeletonHorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();

    // 骷髅马的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2f);
}

} // namespace mc
