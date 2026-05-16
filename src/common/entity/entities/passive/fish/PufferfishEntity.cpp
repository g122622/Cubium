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

#include "PufferfishEntity.hpp"

#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/special/SpecialGoals.hpp"

namespace mc {

PufferfishEntity::PufferfishEntity(LegacyEntityType type, EntityId id)
    : AbstractFishEntity(type, id)
{
}

std::unique_ptr<Entity> PufferfishEntity::create(IWorld* /*world*/)
{
    return std::make_unique<PufferfishEntity>(LegacyEntityType::Unknown, 0);
}

f32 PufferfishEntity::getPuffSize() const
{
    // MC 1.16.5: getPuffSize(puffState)
    // 返回碰撞箱缩放因子，基础尺寸为 0.7 x 0.7
    switch (m_puffState) {
        case PuffState::Deflated:
            return 0.5f;  // 0.7 * 0.5 = 0.35
        case PuffState::SemiPuffed:
            return 0.7f;  // 0.7 * 0.7 = 0.49
        case PuffState::FullyPuffed:
            return 1.0f;  // 0.7 * 1.0 = 0.7
        default:
            return 0.5f;
    }
}

entity::EntitySize PufferfishEntity::getDimensions(EntityPose /*pose*/) const
{
    // MC 1.16.5: 根据膨胀状态动态计算尺寸
    // 基础尺寸 0.7 x 0.7，乘以 getPuffSize() 缩放因子
    f32 scale = getPuffSize();
    return entity::EntitySize::flexible(0.7f * scale, 0.7f * scale);
}

void PufferfishEntity::registerGoals()
{
    AbstractFishEntity::registerGoals();

    // MC 1.16.5: 注册 PuffGoal
    // 优先级 1，检测附近敌人并触发膨胀
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PuffGoal>(this));
}

void PufferfishEntity::tick()
{
    AbstractFishEntity::tick();

    // MC 1.16.5: 只有在服务端世界且存活时才处理膨胀逻辑
    // 由于我们没有 isRemote 检查，直接在 tick 中处理

    // MC 1.16.5 PufferfishEntity.livingTick():
    // if (!this.world.isRemote && this.isAlive() && this.isServerWorld()) {
    //     if (this.puffTimer > 0) {
    //         // 膨胀逻辑
    //     } else if (this.getPuffState() != 0) {
    //         // 收缩逻辑
    //     }
    // }

    if (m_puffTimer > 0) {
        // MC 1.16.5: 膨胀逻辑
        // 当 puffTimer == 1 时，从状态 0 变为状态 1
        if (m_puffState == PuffState::Deflated && m_puffTimer == 1) {
            setPuffState(PuffState::SemiPuffed);
        }
        // 当 puffTimer > 40 且状态为 1 时，从状态 1 变为状态 2
        else if (m_puffTimer > PUFF_SEMI_THRESHOLD && m_puffState == PuffState::SemiPuffed) {
            setPuffState(PuffState::FullyPuffed);
        }

        ++m_puffTimer;
    } else if (m_puffState != PuffState::Deflated) {
        // MC 1.16.5: 收缩逻辑
        ++m_deflateTimer;

        if (m_deflateTimer > DEFLATE_FULL_TO_SEMI && m_puffState == PuffState::FullyPuffed) {
            setPuffState(PuffState::SemiPuffed);
            m_deflateTimer = 0;
        } else if (m_deflateTimer > DEFLATE_SEMI_TO_DEFLATE && m_puffState == PuffState::SemiPuffed) {
            setPuffState(PuffState::Deflated);
            m_deflateTimer = 0;
        }
    }

    // MC 1.16.5: 在膨胀状态时攻击附近敌人
    if (m_puffState != PuffState::Deflated) {
        attackNearbyEnemies();
    }
}

void PufferfishEntity::setPuffState(PuffState state)
{
    if (state == m_puffState) {
        return;
    }

    PuffState oldState = m_puffState;
    m_puffState = state;

    // MC 1.16.5: 膨胀时播放 BLOW_UP 音效
    if (static_cast<i32>(state) > static_cast<i32>(oldState)) {
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_UP, 1.0f, 1.0f);
    }
    // MC 1.16.5: 收缩时播放 BLOW_OUT 音效
    else {
        playSound(SoundEvents::ENTITY_PUFFER_FISH_BLOW_OUT, 1.0f, 1.0f);
    }

    // MC 1.16.5: 刷新碰撞箱尺寸
    refreshDimensions();
}

void PufferfishEntity::startPuffTimer()
{
    // MC 1.16.5 PuffGoal.startExecuting():
    // this.fish.puffTimer = 1;
    // this.fish.deflateTimer = 0;
    m_puffTimer = 1;
    m_deflateTimer = 0;
}

void PufferfishEntity::resetPuffTimer()
{
    // MC 1.16.5 PuffGoal.resetTask():
    // this.fish.puffTimer = 0;
    m_puffTimer = 0;
}

void PufferfishEntity::attackNearbyEnemies()
{
    // MC 1.16.5 PufferfishEntity.livingTick():
    // if (this.isAlive() && this.getPuffState() > 0) {
    //     for(MobEntity mobentity : this.world.getEntitiesWithinAABB(
    //             MobEntity.class, this.getBoundingBox().grow(0.3D), ENEMY_MATCHER)) {
    //         if (mobentity.isAlive()) {
    //             this.attack(mobentity);
    //         }
    //     }
    // }

    if (!isAlive() || !world()) return;

    // 检测碰撞箱扩展 0.3 格范围内的敌人
    AxisAlignedBB searchBox = boundingBox().grow(0.3);

    std::vector<Entity*> entities = world()->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : entities) {
        if (!entity || !entity->isAlive()) continue;

        // 只检测 MobEntity
        MobEntity* mob = dynamic_cast<MobEntity*>(entity);
        if (!mob) continue;

        // 检查是否为敌人（非水生生物）
        // MC 1.16.5: ENEMY_MATCHER 检查 getCreatureAttribute() != WATER
        // 对于玩家，PuffGoal 已经在检测时处理，这里主要处理怪物

        // 攻击敌人
        // MC 1.16.5: attack(MobEntity)
        // 伤害 = 1 + puffState
        // 中毒持续时间 = 60 * puffState ticks

        // 创建伤害来源
        EntityDamageSource damageSource = DamageSources::mobAttack(this);
        i32 damage = 1 + static_cast<i32>(m_puffState);

        if (mob->hurt(damageSource, static_cast<f32>(damage))) {
            // MC 1.16.5: 添加中毒效果
            // mobentity.addPotionEffect(new EffectInstance(Effects.POISON, 60 * i, 0));
            i32 poisonDuration = 60 * static_cast<i32>(m_puffState);
            mob->addEffect(entity::effect::EffectInstance(
                entity::effect::EffectType::Poison,
                poisonDuration,
                0,  // amplifier (0 = Poison I)
                false,  // ambient
                true    // visible
            ));

            // 播放刺击音效
            playSound(SoundEvents::ENTITY_PUFFER_FISH_STING, 1.0f, 1.0f);
        }
    }
}

std::optional<ResourceLocation> PufferfishEntity::getAmbientSound() const
{
    if (!isInWater()) {
        return SoundEvents::ENTITY_PUFFER_FISH_FLOP;
    }
    return SoundEvents::ENTITY_PUFFER_FISH_AMBIENT;
}

std::optional<ResourceLocation> PufferfishEntity::getFlopSound() const
{
    // MC 1.16.5: SoundEvents.ENTITY_PUFFER_FISH_FLOP
    return SoundEvents::ENTITY_PUFFER_FISH_FLOP;
}

std::optional<ResourceLocation> PufferfishEntity::getDeathSound() const
{
    // MC 1.16.5: SoundEvents.ENTITY_PUFFER_FISH_DEATH
    return SoundEvents::ENTITY_PUFFER_FISH_DEATH;
}

std::optional<ResourceLocation> PufferfishEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: SoundEvents.ENTITY_PUFFER_FISH_HURT
    return SoundEvents::ENTITY_PUFFER_FISH_HURT;
}

void PufferfishEntity::registerAttributes()
{
    // 调用父类方法
    AbstractFishEntity::registerAttributes();

    // 河豚的属性
    // 参考 MC 1.16.5 河豚属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
