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

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/FleeSunGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/RestrictSunGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/SpecialDates.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

AbstractSkeletonEntity::AbstractSkeletonEntity(EntityId id)
    : MonsterEntity(id)
{
    // 在构造函数中创建战斗目标（但不添加到选择器）
    // setCombatTask() 会在 onInitialSpawn() 或需要时被调用
    m_rangedAttackGoal = std::make_unique<entity::ai::goal::RangedBowAttackGoal>(
        this, RANGED_ATTACK_SPEED, ATTACK_INTERVAL_MIN, ATTACK_INTERVAL_MAX);

    // 近战目标是一个匿名子类，在 startExecuting/resetTask 中设置 aggro 状态
    // 当前简化实现，使用标准 MeleeAttackGoal
    m_meleeAttackGoal = std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, MELEE_ATTACK_SPEED, false);
}

AbstractSkeletonEntity::~AbstractSkeletonEntity() = default;

void AbstractSkeletonEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    if (target == nullptr || world() == nullptr) {
        return;
    }

    // 重置弓箭状态
    m_chargingBow = false;
    m_attackTimer = 0;
    m_attackCooldown = ATTACK_COOLDOWN;

    // 创建箭矢实体
    auto arrow = entity::ArrowEntity::createFromShooter(*this, world());
    if (arrow == nullptr) {
        return;
    }

    // 计算射击方向
    f64 dx = target->x() - x();
    f64 dy = (target->y() + target->height() * 0.3333333333333333) - arrow->y();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 不精确度：难度越高，不精确度越低，箭矢越精准
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(world()->difficulty());

    // 使用生物箭矢伤害公式设置基础伤害
    arrow->setBaseDamageFromMob(charge);

    // 发射箭矢：速度固定为 1.6F，Y轴补偿 horizontalDist * 0.2 用于抛物线弹道
    constexpr f32 ARROW_VELOCITY = 1.6f;
    arrow->shoot(static_cast<f32>(dx),
        static_cast<f32>(dy + horizontalDist * 0.2),
        static_cast<f32>(dz),
        ARROW_VELOCITY,
        inaccuracy);

    // 播放射箭音效
    math::Random rng = getRandom();
    f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 0.8f);
    playSound(SoundEvents::ENTITY_SKELETON_SHOOT, 1.0f, pitch);

    // 将箭矢添加到世界
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
    // 先移除所有战斗目标，再根据装备添加正确的目标

    // 移除现有的战斗目标
    if (m_rangedAttackGoal) {
        m_goalSelector.removeGoal(m_rangedAttackGoal.get());
    }
    if (m_meleeAttackGoal) {
        m_goalSelector.removeGoal(m_meleeAttackGoal.get());
    }

    // 检查是否持有弓
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

    // 注意：战斗目标（远程/近战）通过 setCombatTask() 添加，不在这里注册
    // 子类（如 WitherSkeletonEntity）可以重写 setCombatTask() 来选择近战

    // ========== 行为目标 (goalSelector) ==========

    // 优先级 2: 限制阳光（不在阳光下移动）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RestrictSunGoal>(this));

    // 优先级 3: 躲避阳光
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::FleeSunGoal>(this, 1.0));

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
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));

    // 优先级 3: 攻击铁傀儡
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));

    // 优先级 3: 攻击幼年海龟
    // TODO: 需要添加 TurtleEntity 的目标过滤（只攻击幼年海龟）
}

void AbstractSkeletonEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ARROW_DAMAGE);
}

void AbstractSkeletonEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    MonsterEntity::finalizeSpawn(world, difficulty, spawnReason);

    math::Random rng = getRandom();

    // 重新评估战斗目标（远程/近战）
    setCombatTask();

    // 设置拾取物品能力
    setCanPickUpLoot(rng.nextFloat() < 0.55f * difficulty.getSpecialMultiplier());

    // 万圣节南瓜头：10月31日，25% 概率
    if (util::SpecialDates::isHalloween() && rng.nextFloat() < 0.25f) {
        if (getEquipment(EquipmentSlot::Head).isEmpty()) {
            const Item* pumpkinItem = rng.nextFloat() < 0.1f ? Items::JACK_O_LANTERN : Items::CARVED_PUMPKIN;
            if (pumpkinItem != nullptr) {
                setEquipment(EquipmentSlot::Head, ItemStack(*pumpkinItem, 1));
                setEquipmentDropChance(EquipmentSlot::Head, 0.0f);
            }
        }
    }
}

} // namespace mc
