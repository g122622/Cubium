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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "IronGolemEntity.hpp"

#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {

IronGolemEntity::IronGolemEntity(EntityInstanceId id)
    : GolemEntity(id)
{
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

    // 优先级 0: 游泳目标
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 近战攻击目标
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));

    // 优先级 2: 向目标移动
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MoveTowardsTargetGoal>(this, 0.9, 32.0f));

    // 优先级 5: 给村民/铜傀儡赠花
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::OfferFlowerGoal>(this));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 优先级 2: 被攻击后反击
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));

    // 优先级 3: 攻击敌对生物
    // canAttackType 已在 TargetGoal::isSuitableTarget 中自动调用，排除苦力怕
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight
            5,    // chance (每5tick检查一次)
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 攻击敌对生物（实现了 IMob 接口/是 MonsterEntity 子类）
                const MonsterEntity* monster = dynamic_cast<const MonsterEntity*>(entity);
                return monster != nullptr;
            }));
}

void IronGolemEntity::registerAttributes()
{
    // 调用父类方法
    GolemEntity::registerAttributes();

    // 铁傀儡的属性
    // ATTACK_DAMAGE 需要先注册（GolemEntity 继承链中未注册此属性，MonsterEntity 才注册）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

std::optional<ResourceLocation> IronGolemEntity::getAmbientSound() const
{
    // 铁傀儡无环境音，对齐原版 AbstractGolem.getAmbientSound 返回 null。
    // sounds.json 中无 entity.iron_golem.ambient，仅 attack/step/hurt/death/repair。
    return std::nullopt;
}

void IronGolemEntity::setHoldingRose(bool holding)
{
    if (holding) {
        m_holdRoseTick = 400; // 400 ticks = 20秒
        // 广播实体状态到客户端：开始持花
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(
                id(), static_cast<u8>(network::EntityStatusPacket::Status::IronGolemHoldRose));
        }
    } else {
        m_holdRoseTick = 0;
        // 广播实体状态到客户端：停止持花
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(
                id(), static_cast<u8>(network::EntityStatusPacket::Status::IronGolemStopRose));
        }
    }
}

bool IronGolemEntity::attackEntityAsMob(LivingEntity& target)
{
    // 设置攻击动画
    m_attackTimer = ATTACK_DURATION;
    m_armsRaised = true;

    // 广播攻击动画到客户端
    if (m_world != nullptr) {
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatusPacket::Status::IronGolemAttack));
    }

    // 计算伤害：随机化伤害值
    // 对应 MC 原版 IronGolem.doHurtTarget:
    //   float f = this.getAttackDamage();
    //   float f1 = (int)f > 0 ? f / 2.0F + this.random.nextInt((int)f) : f;
    // 注意：(int)f 是截断取整而非向上取整，但对于整数 ATTACK_DAMAGE=7.0 无差异
    f32 damage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE));

    math::Random& rng = getRandom();
    if (static_cast<i32>(damage) > 0) {
        damage = damage / 2.0f + static_cast<f32>(rng.nextInt(static_cast<i32>(damage)));
    }

    // 应用伤害
    EntityDamageSource damageSource = DamageSources::mobAttack(this);
    bool success = target.hurt(damageSource, damage);

    if (success) {
        // 铁傀儡击退：向上击飞，考虑目标击退抗性
        f64 knockbackResistance = target.getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
        f64 knockbackMultiplier = std::max(0.0, 1.0 - knockbackResistance);
        target.addVelocity(0.0, 0.4 * knockbackMultiplier, 0.0);

        // 触发附魔后续效果（节肢杀手减速等）
        onAttackEntity(target);
    }

    // 播放攻击声音（无论是否命中都播放）
    playSound(SoundEvents::ENTITY_IRON_GOLEM_ATTACK, 1.0f, 1.0f);

    return success;
}

void IronGolemEntity::playAttackSound(LivingEntity& /*target*/)
{
    // 攻击声音在 attackEntityAsMob 中已经播放（无论是否命中）
    // 此方法保留为空，避免基类和AI目标中重复播放
}

bool IronGolemEntity::canAttackType(const entity::EntityType& type) const
{
    // 玩家创建的铁傀儡不攻击玩家
    if (isPlayerCreated() && &type == entity::VanillaEntityTypeKeys::PLAYER) {
        return false;
    }

    // 铁傀儡不攻击苦力怕
    if (&type == entity::VanillaEntityTypeKeys::CREEPER) {
        return false;
    }

    // 其他情况由父类处理
    return MobEntity::canAttackType(type);
}

} // namespace mc
