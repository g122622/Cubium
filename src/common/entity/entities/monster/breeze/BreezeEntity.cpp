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
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"

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

    // TODO(trial_chambers): 实现旋风人动画状态机
    // 空闲 → 滑行 → 长跳蓄力 → 长跳中 → 射击
}

void BreezeEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 行为目标
    m_goalSelector.addGoal(1, new entity::ai::goal::SwimGoal(this));
    // TODO(trial_chambers): 实现BreezeShootGoal - 向目标投掷风弹
    // TODO(trial_chambers): 实现BreezeLongJumpGoal - 长跳移动
    // TODO(trial_chambers): 实现BreezeSlideGoal - 地面滑行移动
    // TODO(trial_chambers): 实现BreezeShootWhenStuckGoal - 卡住时紧急射击
    m_goalSelector.addGoal(5, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.35));
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0F, 0.02F));
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));

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
    // TODO(trial_chambers): 检查投射物是否是WindChargeEntity
    // if (dynamic_cast<const WindChargeEntity*>(&projectile) != nullptr) {
    //     return false;
    // }
    // return true;
    (void)projectile;
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

    // TODO(trial_chambers): 完整实现风弹投掷逻辑
    // 1. 创建WindChargeEntity
    // 2. 设置射击者为this
    // 3. 计算射击方向（朝向目标，加少量随机偏移）
    // 4. 调用shoot()方法设定初速度
    // 5. 将弹射物添加到世界
    // 6. 设置射击冷却
    // 7. 播放射击音效

    m_shootCooldown = 20; // 1秒冷却
}

} // namespace mc
