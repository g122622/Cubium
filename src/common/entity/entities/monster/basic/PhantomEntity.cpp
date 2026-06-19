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

#include "common/entity/ai/goal/goals/special/PhantomGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

std::unique_ptr<Entity> PhantomEntity::create(IWorld* world)
{
    return std::make_unique<PhantomEntity>(EntityId(0));
}

PhantomEntity::PhantomEntity(EntityId id)
    : FlyingEntity(id)
{
    // 幻翼在阳光下燃烧
    setExperienceValue(5);
}

void PhantomEntity::setPhantomSize(i32 size)
{
    m_phantomSize = std::clamp(size, 0, MAX_PHANTOM_SIZE);
    // 更新攻击力
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE,
        BASE_ATTACK_DAMAGE + static_cast<f32>(m_phantomSize) * SIZE_ATTACK_BONUS);
    refreshDimensions();
}

entity::EntitySize PhantomEntity::getDimensions(EntityPose pose) const
{
    // 尺寸随幻翼大小变化
    f32 baseWidth = 0.9f;
    f32 baseHeight = 0.5f;
    f32 scaleFactor = 1.0f + 0.2f * static_cast<f32>(m_phantomSize);
    return entity::EntitySize::flexible(baseWidth * scaleFactor, baseHeight * scaleFactor);
}

std::optional<ResourceLocation> PhantomEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> PhantomEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> PhantomEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

void PhantomEntity::tick()
{
    FlyingEntity::tick();

    // 幻翼在阳光下燃烧（MC 原版中在 BURN_IN_DAYLIGHT 标签中）
    burnUndead();
}

bool PhantomEntity::canAttackType(entity::EntityTypeId /*typeId*/) const
{
    // MC 原版 Phantom.canAttackType() 返回 true
    // 覆盖 Mob 基类排除恶魂的限制，幻翼本身是飞行生物，可以攻击空中目标
    return true;
}

void PhantomEntity::registerGoals()
{
    FlyingEntity::registerGoals();

    // 攻击目标选择器：
    // 优先级 1: PickAttackGoal - 选择攻击阶段
    // 优先级 2: SweepAttackGoal - 俯冲攻击
    // 优先级 3: OrbitPointGoal - 环绕飞行
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

    // 幻翼属性：生命值20，移动速度0（飞行生物不使用地面速度），攻击力6，追踪距离64
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, BASE_ATTACK_DAMAGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

void PhantomEntity::updateAITasks()
{
    FlyingEntity::updateAITasks();
    // TODO: 实现幻翼特定的AI任务更新逻辑
}

} // namespace mc
