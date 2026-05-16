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

#include "AbstractSkeletonEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../player/Player.hpp"
#include "../../passive/golem/IronGolemEntity.hpp"
#include "../../projectile/AbstractArrowEntity.hpp"

namespace mc {

AbstractSkeletonEntity::AbstractSkeletonEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 在构造函数中创建战斗目标（但不添加到选择器）
    // setCombatTask() 会在 onInitialSpawn() 或需要时被调用
    m_rangedAttackGoal = std::make_unique<entity::ai::goal::RangedBowAttackGoal>(
        this, RANGED_ATTACK_SPEED, ATTACK_INTERVAL_MIN, ATTACK_INTERVAL_MAX);

    // MC 1.16.5: 近战目标是一个匿名子类，在 startExecuting/resetTask 中设置 aggro 状态
    // 当前简化实现，使用标准 MeleeAttackGoal
    m_meleeAttackGoal = std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, MELEE_ATTACK_SPEED, false);
}

AbstractSkeletonEntity::~AbstractSkeletonEntity() = default;

void AbstractSkeletonEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // MC 1.16.5 AbstractSkeletonEntity.attackEntityWithRangedAttack()
    if (target == nullptr || world() == nullptr) {
        return;
    }

    // 重置弓箭状态
    m_chargingBow = false;
    m_attackTimer = 0;
    m_attackCooldown = ATTACK_COOLDOWN;

    // 创建箭矢实体
    // MC 1.16.5: ItemStack itemstack = this.findAmmo(this.getHeldItem(ProjectileHelper.getHandWith(this, Items.BOW)));
    //           AbstractArrowEntity abstractarrowentity = this.fireArrow(itemstack, distanceFactor);
    // 当前简化实现：直接创建普通箭矢（暂不考虑弹药和附魔）
    auto arrow = entity::ArrowEntity::createFromShooter(*this, world());
    if (arrow == nullptr) {
        return;
    }

    // MC 1.16.5: 计算射击方向
    // d0 = target.getPosX() - this.getPosX()
    // d1 = target.getPosYHeight(0.3333333333333333D) - abstractarrowentity.getPosY()
    // d2 = target.getPosZ() - this.getPosZ()
    // d3 = MathHelper.sqrt(d0 * d0 + d2 * d2)
    f64 dx = target->x() - x();
    f64 dy = (target->y() + target->height() * 0.3333333333333333) - arrow->y();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // MC 1.16.5: 不精确度计算
    // inaccuracy = 14 - world.getDifficulty().getId() * 4
    // 和平/简单: 14, 普通: 10, 困难: 6
    // 难度越高，不精确度越低，箭矢越精准
    i32 difficultyId = static_cast<i32>(world()->difficulty());
    f32 inaccuracy = static_cast<f32>(14 - difficultyId * 4);

    // MC 1.16.5: 设置箭矢伤害
    // damage = distanceFactor * 2.0 + randomGaussian * 0.25 + difficulty * 0.11
    // 当前简化实现：基础伤害 + 蓄力加成
    f32 damage = ARROW_DAMAGE + charge * 0.5f;
    arrow->setDamage(damage);

    // MC 1.16.5: 发射箭矢
    // abstractarrowentity.shoot(d0, d1 + d3 * 0.2, d2, 1.6F, inaccuracy)
    // 速度固定为 1.6F，Y轴补偿 horizontalDist * 0.2 用于抛物线弹道
    constexpr f32 ARROW_VELOCITY = 1.6f;
    arrow->shoot(static_cast<f32>(dx),
                 static_cast<f32>(dy + horizontalDist * 0.2),
                 static_cast<f32>(dz),
                 ARROW_VELOCITY,
                 inaccuracy);

    // MC 1.16.5: 播放射箭音效
    // this.playSound(SoundEvents.ENTITY_SKELETON_SHOOT, 1.0F, 1.0F / (this.getRNG().nextFloat() * 0.4F + 0.8F))
    math::Random rng = getRandom();
    f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 0.8f);
    playSound(SoundEvents::ENTITY_SKELETON_SHOOT, 1.0f, pitch);

    // MC 1.16.5: 将箭矢添加到世界
    world()->spawnEntity(std::move(arrow));
}

void AbstractSkeletonEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        --m_attackCooldown;
    }

    if (m_attackTimer > 0) {
        --m_attackTimer;
        m_chargingBow = true;
        if (m_attackTimer == 0) {
            m_chargingBow = false;
        }
    }
}

void AbstractSkeletonEntity::setCombatTask()
{
    // MC 1.16.5 AbstractSkeletonEntity.setCombatTask()
    // 先移除所有战斗目标，再根据装备添加正确的目标

    // 移除现有的战斗目标
    if (m_rangedAttackGoal) {
        m_goalSelector.removeGoal(m_rangedAttackGoal.get());
    }
    if (m_meleeAttackGoal) {
        m_goalSelector.removeGoal(m_meleeAttackGoal.get());
    }

    // MC 1.16.5: 检查是否持有弓
    // 当前简化实现：默认使用远程攻击
    // 子类可以重写此方法来选择不同的战斗目标
    //
    // TODO: 当物品系统完善后，应该检查装备：
    // ItemStack itemstack = getHeldItem(ProjectileHelper.getHandWith(this, Items.BOW));
    // if (itemstack.getItem() == Items.BOW) {
    //     m_goalSelector.addGoal(COMBAT_GOAL_PRIORITY, m_rangedAttackGoal.get());
    // } else {
    //     m_goalSelector.addGoal(COMBAT_GOAL_PRIORITY, m_meleeAttackGoal.get());
    // }

    // 默认使用远程攻击（普通骷髅和流浪者）
    if (m_rangedAttackGoal) {
        m_goalSelector.addGoal(COMBAT_GOAL_PRIORITY, m_rangedAttackGoal.get());
    }
}

void AbstractSkeletonEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // MC 1.16.5 AbstractSkeletonEntity.registerGoals()
    // 注意：战斗目标（远程/近战）通过 setCombatTask() 添加，不在这里注册
    // 子类（如 WitherSkeletonEntity）可以重写 setCombatTask() 来选择近战

    // ========== 行为目标 (goalSelector) ==========

    // 优先级 2: 限制阳光（不在阳光下移动）
    // TODO: RestrictSunGoal 未实现
    // m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RestrictSunGoal>(this));

    // 优先级 3: 躲避阳光
    // TODO: FleeSunGoal 未实现
    // m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::FleeSunGoal>(this, 1.0));

    // 优先级 3: 躲避狼
    // TODO: AvoidEntityGoal<WolfEntity> 需要实现模板版本
    // m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::AvoidEntityGoal<WolfEntity>>(this, 6.0f, 1.0, 1.2));

    // 优先级 4: 战斗目标（通过 setCombatTask() 动态添加）
    // 参考 setCombatTask()

    // 优先级 5: 避水随机行走
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // ========== 目标选择器 (targetSelector) ==========

    // 优先级 1: 被攻击后反击（已在 MonsterEntity::registerGoals() 中注册）

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));

    // 优先级 3: 攻击铁傀儡
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));

    // 优先级 3: 攻击幼年海龟
    // TODO: 需要添加 TurtleEntity 的目标过滤（只攻击幼年海龟）
    // MC 1.16.5: NearestAttackableTargetGoal<TurtleEntity>(this, 10, true, false, TurtleEntity.TARGET_DRY_BABY)
}

void AbstractSkeletonEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ARROW_DAMAGE);
}

} // namespace mc
