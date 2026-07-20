/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation to the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "BlazeEntity.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/special/BlazeFireballAttackGoal.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../player/Player.hpp"
#include "common/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {

BlazeEntity::BlazeEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 烈焰人不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 经验值
    setExperienceValue(10);
}

std::unique_ptr<Entity> BlazeEntity::create(IWorld* /*world*/)
{
    return std::make_unique<BlazeEntity>(EntityInstanceId(0));
}

std::optional<ResourceLocation> BlazeEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_BLAZE_AMBIENT;
}

std::optional<ResourceLocation> BlazeEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_BLAZE_HURT;
}

std::optional<ResourceLocation> BlazeEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_BLAZE_DEATH;
}

void BlazeEntity::attackEntityWithRangedAttack(LivingEntity* /*target*/, f32 /*charge*/)
{
    // IRangedAttackMob 纯虚接口的空实现。
    // 烈焰人使用专用的 BlazeFireballAttackGoal 管理火球攻击，
    // 而非通用的 RangedAttackGoal，因此此方法不会被外部调用。
}

void BlazeEntity::tick()
{
    // ========== 空中缓降 ==========
    // 对齐 MC 1.21.11 Blaze.aiStep():
    // 当不在地面且Y轴速度向下时，将Y速度乘以0.6实现缓降
    if (!onGround() && velocityY() < 0.0f) {
        setVelocity(velocityX(), velocityY() * FALL_DAMPING, velocityZ());
    }

    // ========== 水伤害 ==========
    // 对齐 MC 1.21.11 LivingEntity.baseTick():
    // if (isSensitiveToWater() && isInWaterOrRain())
    //     hurtServer(damageSources().drown(), 1.0F);
    // 烈焰人 isSensitiveToWater() 返回 true，
    // 在水中或雨中每 tick 受 1 点 drown 伤害
    // 注：伤害源为 drown（非 onFire），影响死亡消息和火焰保护附魔交互
    if (isWaterSensitive() && isWet()) {
        auto damageSource = DamageSources::drown();
        hurt(damageSource, WATER_DAMAGE_AMOUNT);
    }

    // ========== 客户端粒子效果和音效 ==========
    if (world() != nullptr && world()->isClientSide()) {
        math::Random& random = world()->getRandom();

        // 随机播放燃烧音效（24分之1概率）
        if (random.nextInt(24) == 0 && !isSilent()) {
            world()->playSound(SoundEvents::ENTITY_BLAZE_BURN,
                sound::SoundCategory::Hostile,
                m_position,
                1.0f + random.nextFloat() * 0.3f, // 音量
                random.nextFloat() * 0.7f + 0.3f  // 音调
            );
        }

        // 生成烟雾粒子
        using namespace particle;
        for (i32 i = 0; i < 2; ++i) {
            f32 px = static_cast<f32>(x()) + (random.nextFloat() - 0.5f) * width();
            f32 py = static_cast<f32>(y()) + random.nextFloat() * height();
            f32 pz = static_cast<f32>(z()) + (random.nextFloat() - 0.5f) * width();
            world()->addParticle(ParticleTypeId::LargeSmoke, Vector3(px, py, pz), Vector3(0.0, 0.0, 0.0));
        }
    }

    MonsterEntity::tick();
}

void BlazeEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 对齐 MC 1.21.11 Blaze.registerGoals():
    // 优先级 4: BlazeAttackGoal（火球攻击，包含充能/发射/近战/追击）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::BlazeFireballAttackGoal>(this));

    // 优先级 5: MoveTowardsRestrictionGoal（向限制点移动，如下界堡垒区域）
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::MoveTowardsRestrictionGoal>(this, 1.0));

    // 优先级 7: WaterAvoidingRandomWalkingGoal（避水随机行走）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0, 0.0f));

    // 优先级 8: LookAtGoal（看向玩家）
    m_goalSelector.addGoal(
        8, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return dynamic_cast<const Player*>(entity) != nullptr;
        }));

    // 优先级 8: LookRandomlyGoal（随机看向）
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器：
    // 优先级 1: HurtByTargetGoal（被攻击反击，呼唤同伴）
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));

    // 优先级 2: NearestAttackableTargetGoal<Player>（攻击玩家）
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));
}

void BlazeEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 烈焰人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 48.0);
}

void BlazeEntity::updateAITasks()
{
    // ========== 悬浮高度偏移随机化 ==========
    // 对齐 MC 1.21.11 Blaze.customServerAiStep():
    // 每 100 tick 通过三角分布重新随机化 allowedHeightOffset
    --m_nextHeightOffsetChangeTick;
    if (m_nextHeightOffsetChangeTick <= 0) {
        m_nextHeightOffsetChangeTick = HEIGHT_OFFSET_CHANGE_INTERVAL;
        // MC 原版: this.allowedHeightOffset = (float)this.random.triangle(0.5, 6.891);
        // triangle(mode, deviation) = mode + (nextFloat() - nextFloat()) * deviation
        math::Random& rng = getRandom();
        m_allowedHeightOffset = HEIGHT_OFFSET_MODE + (rng.nextFloat() - rng.nextFloat()) * HEIGHT_OFFSET_DEVIATION;
    }

    // ========== 上升推力 ==========
    // 对齐 MC 1.21.11 Blaze.customServerAiStep():
    // 当攻击目标的眼高 > 烈焰人眼高 + allowedHeightOffset 时，施加上升推力
    LivingEntity* target = attackTarget();
    if (target != nullptr && target->isAlive()) {
        f64 targetEyeY = target->y() + static_cast<f64>(target->eyeHeight());
        f64 blazeEyeY = y() + static_cast<f64>(eyeHeight());

        if (targetEyeY > blazeEyeY + static_cast<f64>(m_allowedHeightOffset)) {
            // PD 控制器式的上升推力：向 ASCEND_TARGET_SPEED 收敛
            // MC 原版: this.setDeltaMovement(this.getDeltaMovement().add(0.0, (0.3F - vec3.y) * 0.3F, 0.0));
            f32 currentVelY = velocityY();
            f32 ascendForce = (ASCEND_TARGET_SPEED - currentVelY) * ASCEND_ACCELERATION;
            setVelocity(velocityX(), currentVelY + ascendForce, velocityZ());
        }
    }

    MonsterEntity::updateAITasks();
}

} // namespace mc
