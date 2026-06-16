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

#include "IllusionerEntity.hpp"

#include "../../../../item/Items.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "../../../ai/goal/goals/special/IllusionerGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../entities/passive/golem/IronGolemEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entities/villager/AbstractVillagerEntity.hpp"
#include <cmath>

namespace mc {

IllusionerEntity::IllusionerEntity(EntityId id)
    : SpellcastingIllagerEntity(id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> IllusionerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<IllusionerEntity>(EntityId(0));
}

void IllusionerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    if (target == nullptr || world() == nullptr) {
        return;
    }

    // 创建箭矢实体
    auto arrow = entity::ArrowEntity::createFromShooter(*this, world());
    if (arrow == nullptr) {
        return;
    }

    // 计算发射方向
    f64 dx = target->x() - x();
    f64 dy = (target->y() + target->height() * 0.333) - (arrow->y());
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 计算不精确度
    i32 difficulty = static_cast<i32>(world()->difficulty());
    f32 inaccuracy = static_cast<f32>(14 - difficulty * 4);

    // 使用生物箭矢伤害公式设置基础伤害
    arrow->setBaseDamageFromMob(charge);

    // 发射箭矢
    arrow->shoot(static_cast<f32>(dx),
        static_cast<f32>(dy + horizontalDist * 0.2),
        static_cast<f32>(dz),
        ARROW_VELOCITY, // 1.6F
        inaccuracy);

    // 播放射箭音效
    math::Random rng = getRandom();
    f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 0.8f);
    playSound(SoundEvents::ENTITY_SKELETON_SHOOT, 1.0f, pitch);

    // 生成箭矢实体
    world()->spawnEntity(std::move(arrow));
}

void IllusionerEntity::tick()
{
    SpellcastingIllagerEntity::tick();

    // 更新冷却时间
    if (m_blindnessCooldown > 0) {
        --m_blindnessCooldown;
    }
    if (m_mirrorCooldown > 0) {
        --m_mirrorCooldown;
    }
}

void IllusionerEntity::registerGoals()
{
    // 调用父类方法
    SpellcastingIllagerEntity::registerGoals();

    // 行为目标选择器 (goalSelector)
    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 施法时看向目标（父类已注册 CastingSpellGoal）

    // 优先级 4: 镜像法术（隐身）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::IllusionerMirrorSpellGoal>(this));

    // 优先级 5: 失明法术
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::IllusionerBlindnessSpellGoal>(this));

    // 优先级 6: 弓箭远程攻击
    // 参数：移动速度 0.5，攻击间隔 20 ticks
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::RangedBowAttackGoal>(this, 0.5, 20, 20));

    // 优先级 8: 随机行走
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 0.6, 1));

    // 优先级 9: 看向玩家
    m_goalSelector.addGoal(
        9, std::make_unique<entity::ai::goal::LookAtGoal>(this, 3.0f, 1.0f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            return entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));

    // 优先级 10: 看向生物
    m_goalSelector.addGoal(
        10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            // 看向所有 MobEntity
            return entity->typeId() != entity::EntityTypeIdNumber::PLAYER;
        }));

    // 目标选择器 (targetSelector)
    // 优先级 1: 被攻击后反击并呼叫支援
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));

    // 优先级 2: 攻击玩家（300 ticks 未见记忆）
    m_targetSelector.addGoal(
        2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 300));

    // 优先级 3: 攻击村民（300 ticks 未见记忆）
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(
            this, false, 300));

    // 优先级 3: 攻击铁傀儡（300 ticks 未见记忆）
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, false, 300));
}

void IllusionerEntity::registerAttributes()
{
    SpellcastingIllagerEntity::registerAttributes();

    // 幻术师属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 32.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 18.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

} // namespace mc
