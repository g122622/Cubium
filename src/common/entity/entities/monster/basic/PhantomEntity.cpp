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

#include "PhantomEntity.hpp"
#include "../../../ai/goal/goals/special/PhantomGoals.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include <cmath>

namespace mc {

std::unique_ptr<Entity> PhantomEntity::create(IWorld* world)
{
    return std::make_unique<PhantomEntity>(EntityId(0));
}

PhantomEntity::PhantomEntity(EntityId id)
    : FlyingEntity(id)
{
    // MC 1.16.5: 幻翼在阳光下燃烧
    setExperienceValue(5);
}

void PhantomEntity::setPhantomSize(i32 size)
{
    m_phantomSize = std::clamp(size, 0, MAX_PHANTOM_SIZE);
    // MC 1.16.5: 更新攻击力
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE,
        BASE_ATTACK_DAMAGE + static_cast<f32>(m_phantomSize) * SIZE_ATTACK_BONUS);
    refreshDimensions();
}

entity::EntitySize PhantomEntity::getDimensions(EntityPose pose) const
{
    // MC 1.16.5: 尺寸随幻翼大小变化
    f32 baseWidth = 0.9f;
    f32 baseHeight = 0.5f;
    f32 scaleFactor = 1.0f + 0.2f * static_cast<f32>(m_phantomSize);
    return entity::EntitySize::flexible(baseWidth * scaleFactor, baseHeight * scaleFactor);
}

std::optional<ResourceLocation> PhantomEntity::getAmbientSound() const
{
    // MC 1.16.5: entity.phantom.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> PhantomEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.phantom.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> PhantomEntity::getDeathSound() const
{
    // MC 1.16.5: entity.phantom.death
    return makeSoundEventId("death");
}

void PhantomEntity::tick()
{
    // MC 1.16.5 PhantomEntity.tick()
    FlyingEntity::tick();

    // MC 1.16.5: 在阳光下着火8秒 (livingTick 中调用)
    // 参考: if (this.isAlive() && this.isInDaylight()) { this.setFire(8); }
    if (isAlive() && isInDaylight()) {
        setFire(8);
    }
}

void PhantomEntity::registerGoals()
{
    FlyingEntity::registerGoals();

    // MC 1.16.5 PhantomEntity.registerGoals()
    // 优先级 1: PickAttackGoal - 选择攻击阶段
    // 优先级 2: SweepAttackGoal - 俯冲攻击
    // 优先级 3: OrbitPointGoal - 环绕飞行
    //
    // 目标选择器：
    // 优先级 1: AttackPlayerTargetGoal - 攻击玩家

    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PhantomPickAttackGoal>(this));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::PhantomSweepAttackGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::PhantomOrbitPointGoal>(this));
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::PhantomAttackPlayerTargetGoal>(this));
}

void PhantomEntity::registerAttributes()
{
    FlyingEntity::registerAttributes();

    // MC 1.16.5 PhantomEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0); // 幻翼飞行，不使用地面速度
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, BASE_ATTACK_DAMAGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

void PhantomEntity::updateAITasks()
{
    FlyingEntity::updateAITasks();
    // MC 1.16.5: 更新AI任务
}

} // namespace mc
