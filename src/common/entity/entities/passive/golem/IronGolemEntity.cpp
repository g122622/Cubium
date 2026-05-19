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

#include "IronGolemEntity.hpp"

#include "entity/ai/controller/LookController.hpp"
#include "entity/ai/goal/GoalConstants.hpp"
#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "entity/ai/goal/goals/SwimGoal.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/monster/MonsterEntity.hpp"
#include "entity/entities/villager/VillagerEntity.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include <memory>

namespace mc {

IronGolemEntity::IronGolemEntity(EntityId id)
    : GolemEntity(id)
{
    // MC 1.16.5: IronGolemEntity 构造函数中设置 stepHeight = 1.0F
    // 铁傀儡可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> IronGolemEntity::create(IWorld* /*world*/)
{
    return std::make_unique<IronGolemEntity>(0);
}

void IronGolemEntity::tick()
{
    GolemEntity::tick();

    // 更新攻击动画
    if (m_attackTimer > 0) {
        m_attackTimer--;
        m_armsRaised = true;
        if (m_attackTimer <= 0) {
            m_armsRaised = false;
        }
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 更新持花状态
    if (m_holdRoseTick > 0) {
        m_holdRoseTick--;
        if (m_holdRoseTick <= 0) {
            // 持花结束
        }
    }
}

void IronGolemEntity::registerGoals()
{
    // 调用父类方法
    GolemEntity::registerGoals();

    // MC 1.16.5 IronGolemEntity.registerGoals()
    // 优先级 0: 游泳目标
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 近战攻击目标（MC 1.16.5: MeleeAttackGoal(this, 1.0D, true)）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));

    // 优先级 2: 向目标移动（MC 1.16.5: MoveTowardsTargetGoal(this, 0.9D, 32.0F)）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MoveTowardsTargetGoal>(this, 0.9, 32.0f));

    // 优先级 5: 给村民送花（MC 1.16.5: ShowVillagerFlowerGoal(this)）
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::ShowVillagerFlowerGoal>(this));

    // 优先级 7: 看向玩家（MC 1.16.5: LookAtGoal(this, PlayerEntity.class, 6.0F)）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 8: 随机看向（MC 1.16.5: LookRandomlyGoal(this)）
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 优先级 2: 被攻击后反击（MC 1.16.5: HurtByTargetGoal(this)）
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));

    // 优先级 3: 攻击敌对生物（MC 1.16.5: NearestAttackableTargetGoal<MobEntity>）
    // 排除苦力怕，因为铁傀儡不攻击苦力怕
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight
            5,    // chance (每5tick检查一次)
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 铁傀儡不攻击苦力怕
                if (entity->typeId() == entity::EntityTypeIdNumber::CREEPER) return false;
                // 攻击敌对生物（实现了 IMob 接口/是 MonsterEntity 子类）
                const MonsterEntity* monster = dynamic_cast<const MonsterEntity*>(entity);
                return monster != nullptr;
            }));

    // 优先级 4: 重置愤怒（MC 1.16.5: ResetAngerGoal<>(this, false)）
    // 当前项目暂未实现 UNIVERSAL_ANGER 游戏规则，暂时不添加
    // m_targetSelector.addGoal(4, std::make_unique<entity::ai::goal::ResetAngerGoal<IronGolemEntity>>(this, false));
}

void IronGolemEntity::registerAttributes()
{
    // 调用父类方法
    GolemEntity::registerAttributes();

    // 铁傀儡的属性
    // 参考 MC 1.16.5 铁傀儡属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

void IronGolemEntity::setHoldingRose(bool holding)
{
    if (holding) {
        m_holdRoseTick = 400; // MC 1.16.5: 400 ticks = 20秒
        // MC 1.16.5: 发送状态更新到客户端（byte 11）
        // 当前项目暂未实现网络同步
    } else {
        m_holdRoseTick = 0;
        // MC 1.16.5: 发送状态更新到客户端（byte 34）
    }
}

bool IronGolemEntity::attackEntityAsMob(LivingEntity& target)
{
    // MC 1.16.5: IronGolemEntity.attackEntityAsMob()

    // 设置攻击动画
    m_attackTimer = ATTACK_DURATION;
    m_armsRaised = true;

    // 计算伤害
    // MC 1.16.5: float f = (float)this.getAttributeValue(Attributes.ATTACK_DAMAGE);
    f32 damage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE));

    // MC 1.16.5: float f1 = (int)f > 0 ? f / 2.0F + (float)this.rand.nextInt((int)f) : f;
    math::Random rng = getRandom();
    if (static_cast<i32>(damage) > 0) {
        damage = damage / 2.0f + static_cast<f32>(rng.nextInt(static_cast<i32>(damage)));
    }

    // 应用伤害
    // MC 1.16.5: boolean flag = target.attackEntityFrom(DamageSource.causeMobDamage(this), f1);
    EntityDamageSource damageSource = DamageSources::mobAttack(this);
    bool success = target.hurt(damageSource, damage);

    if (success) {
        // MC 1.16.5: 应用击退
        // entityIn.setMotion(entityIn.getMotion().add(0.0D, (double)0.4F, 0.0D));
        Vector3 velocity = target.velocity();
        velocity.y += 0.4;
        target.setVelocity(velocity);

        // MC 1.16.5: 应用附魔效果
        // this.applyEnchantments(this, entityIn);
        // 当前项目暂未实现附魔系统
    }

    // 播放攻击声音
    // MC 1.16.5: this.playSound(SoundEvents.ENTITY_IRON_GOLEM_ATTACK, 1.0F, 1.0F);
    // 当前项目暂未实现声音系统

    return success;
}

bool IronGolemEntity::canAttackEntity(entity::EntityTypeId typeId) const
{
    // MC 1.16.5: IronGolemEntity.canAttack(EntityType<?> typeIn)

    // 玩家创建的铁傀儡不攻击玩家
    if (isPlayerCreated() && typeId == entity::EntityTypeIdNumber::PLAYER) {
        return false;
    }

    // 铁傀儡不攻击苦力怕
    if (typeId == entity::EntityTypeIdNumber::CREEPER) {
        return false;
    }

    // 其他情况由父类处理
    return true;
}

} // namespace mc
