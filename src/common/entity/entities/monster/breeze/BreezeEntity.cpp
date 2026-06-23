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

#include "BreezeEntity.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/BreezeGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

BreezeEntity::BreezeEntity(EntityId id)
    : MonsterEntity(id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> BreezeEntity::create(IWorld* /*world*/)
{
    return std::make_unique<BreezeEntity>(EntityId(0));
}

void BreezeEntity::tick()
{
    MonsterEntity::tick();

    // 更新射击冷却
    if (m_shootCooldown > 0) {
        --m_shootCooldown;
    }

    // 更新长跳冷却
    if (m_jumpCooldown > 0) {
        --m_jumpCooldown;
    }

    // 更新射击许可计时器
    if (m_shootPermitTicks > 0) {
        --m_shootPermitTicks;
    }

    // 着陆时清除长跳状态
    if (m_isLongJumping && onGround()) {
        m_isLongJumping = false;
    }

    // TODO(trial_chambers): 实现旋风人动画状态机
    // 空闲 → 滑行 → 长跳蓄力 → 长跳中 → 射击
}

void BreezeEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 行为目标（参考 MC Java BreezeAi.FIGHT 行为优先级）
    m_goalSelector.addGoal(1, new entity::ai::goal::SwimGoal(this));
    m_goalSelector.addGoal(2, new entity::ai::goal::BreezeShootGoal(this));          // 射击风弹
    m_goalSelector.addGoal(3, new entity::ai::goal::BreezeLongJumpGoal(this));       // 长跳移动
    m_goalSelector.addGoal(4, new entity::ai::goal::BreezeShootWhenStuckGoal(this)); // 卡住时紧急射击
    m_goalSelector.addGoal(5, new entity::ai::goal::BreezeSlideGoal(this));          // 滑行移动
    m_goalSelector.addGoal(6, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.35));
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 8.0F, 0.02F));
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, false));
    m_targetSelector.addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true));
}

void BreezeEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, MAX_HEALTH);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, MOVEMENT_SPEED);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, FOLLOW_RANGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

bool BreezeEntity::canAttackType(entity::EntityTypeId typeId) const
{
    // MC 原版 Breeze.canAttackType()：仅允许攻击玩家和铁傀儡
    // 旋风人采用白名单模式，其余所有实体类型都不能被攻击
    return typeId == entity::EntityTypeIdNumber::PLAYER || typeId == entity::EntityTypeIdNumber::IRON_GOLEM;
}

bool BreezeEntity::shouldDeflectProjectile(const entity::ProjectileEntity& projectile) const
{
    // 风弹不应被偏转
    if (dynamic_cast<const entity::WindChargeEntity*>(&projectile) != nullptr) {
        return false;
    }
    return true;
}

void BreezeEntity::shootWindCharge()
{
    if (m_shootCooldown > 0) {
        return;
    }

    if (m_world == nullptr || m_attackTarget == nullptr) {
        return;
    }

    // 计算从旋风人到目标的方向向量
    // 旋风人发射位置：身体中心偏上0.3格
    const f32 firingY = y() + height() * 0.5f + 0.3f;
    const Vector3 firingPos(x(), firingY, z());
    const Vector3 targetPos = m_attackTarget->position();

    // 方向向量
    const f32 dx = targetPos.x - firingPos.x;
    const f32 dy = targetPos.y + m_attackTarget->height() * 0.5f - firingPos.y;
    const f32 dz = targetPos.z - firingPos.z;

    // 创建风弹弹射物实体（通过发射者类型自动判定为旋风人风弹）
    auto entity = std::make_unique<entity::WindChargeEntity>(EntityId(0));
    entity->setWorld(m_world);
    entity->setPosition(firingPos.x, firingPos.y, firingPos.z);
    entity->setShooter(this);

    entity::WindChargeEntity* projectile = entity.get();
    m_world->spawnEntity(std::move(entity));

    // 设置射击参数：速度0.7，散布随难度降低（简单=1, 普通=1, 困难=0）
    // 项目中散布暂时使用1.0
    projectile->shoot(dx, dy, dz, 0.7f, 1.0f);

    // 播放旋风人射击音效
    m_world->playSound(
        SoundEvents::ENTITY_BREEZE_WIND_CHARGE_BURST, sound::SoundCategory::Hostile, firingPos, 1.0f, 1.0f);

    // 设置射击冷却
    m_shootCooldown = 20; // 1秒冷却
}

} // namespace mc
