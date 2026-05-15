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

#include "EndermanEntity.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/damage/DamageSource.hpp"
#include <cmath>

namespace mc {

EndermanEntity::EndermanEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 末影人不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> EndermanEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EndermanEntity>(LegacyEntityType::Unknown, 0);
}

std::optional<ResourceLocation> EndermanEntity::getAmbientSound() const
{
    // MC 1.16.5: 愤怒时返回 ambient，被注视时返回 scream
    if (m_screaming) {
        return makeSoundEventId("scream");
    }
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> EndermanEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.enderman.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> EndermanEntity::getDeathSound() const
{
    // MC 1.16.5: entity.enderman.death
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> EndermanEntity::getStareSound() const
{
    // MC 1.16.5: entity.enderman.stare
    return makeSoundEventId("stare");
}

std::optional<ResourceLocation> EndermanEntity::getTeleportSound() const
{
    // MC 1.16.5: entity.enderman.teleport
    return makeSoundEventId("teleport");
}

void EndermanEntity::setRevengeTarget(LivingEntity* target)
{
    m_attackTarget = target;
    if (target != nullptr) {
        setAngry(true);
        m_angerTime = ANGER_DURATION;
        m_revengeTargetId = target->id();
        m_revengeTimer = ANGER_DURATION;
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* EndermanEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void EndermanEntity::setAngry(bool angry)
{
    m_angry = angry;
    if (!angry) {
        m_angerTime = 0;
        m_attackTarget = nullptr;
        m_screaming = false;
    }
}

void EndermanEntity::setHeldBlockState(const BlockState* state)
{
    m_heldBlockState = state;
    m_holdingBlock = (state != nullptr);
}

bool EndermanEntity::teleport()
{
    // MC 1.16.5 EndermanEntity.teleport()
    if (m_teleportCooldown > 0) {
        return false;
    }

    // 末影人瞬移范围：64 格
    // 参考 MC 1.16.5 EndermanEntity.teleportRandomly()
    bool success = randomTeleport(TELEPORT_RANGE, true, true);

    if (success) {
        m_teleportCooldown = TELEPORT_COOLDOWN;

        // 播放瞬移音效
        auto teleportSound = getTeleportSound();
        if (teleportSound) {
            playSound(*teleportSound, 1.0f, 1.0f);
        }
    }

    return success;
}

bool EndermanEntity::teleportToTarget()
{
    // MC 1.16.5 EndermanEntity.teleportTowards()
    if (m_attackTarget == nullptr || m_teleportCooldown > 0) {
        return false;
    }

    // 计算远离目标的方向向量
    Vector3 direction(m_position.x - m_attackTarget->position().x, 0.0, m_position.z - m_attackTarget->position().z);

    // 归一化方向向量
    f32 length = direction.length();
    if (length > 0.001f) {
        direction.x /= length;
        direction.z /= length;
    } else {
        // 如果长度太小，随机选择方向
        math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted));
        f32 angle = rng.nextFloat() * 6.28318530718f;
        direction.x = std::cos(angle);
        direction.z = std::sin(angle);
    }

    // 目标位置：远离目标 16 格
    math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted));
    f32 targetX = m_position.x + (rng.nextDouble() - 0.5) * 8.0 - direction.x * 16.0;
    f32 targetY = m_position.y + static_cast<f32>(rng.nextInt(16) - 8);
    f32 targetZ = m_position.z + (rng.nextDouble() - 0.5) * 8.0 - direction.z * 16.0;

    // 尝试瞬移
    bool success = attemptTeleport(targetX, targetY, targetZ, true);

    if (success) {
        m_teleportCooldown = TELEPORT_COOLDOWN;
    }

    return success;
}

bool EndermanEntity::teleportAwayFromWater()
{
    // MC 1.16.5: 瞬移避开水
    // 尝试多次瞬移，直到找到一个不在水中的位置
    for (i32 i = 0; i < 10; ++i) {
        if (teleport()) {
            // 检查是否还在水中
            if (!isInWater() && !isInLava()) {
                return true;
            }
        }
    }
    return false;
}

void EndermanEntity::placeHeldBlock()
{
    // MC 1.16.5 EndermanEntity.placeBlock()
    if (!m_holdingBlock || m_heldBlockState == nullptr) {
        return;
    }

    // TODO: 放置方块
    // 1. 找到合适的放置位置
    // 2. 检查是否可以放置
    // 3. 放置方块

    m_holdingBlock = false;
    m_heldBlockState = nullptr;
}

void EndermanEntity::pickUpBlock()
{
    // MC 1.16.5 EndermanEntity.takeBlock()
    // TODO: 拾取方块
    // 1. 找到可拾取的方块
    // 2. 检查方块是否在可拾取列表中
    // 3. 移除方块并设置 heldBlockState
}

bool EndermanEntity::isInWaterOrRain() const
{
    // MC 1.16.5: 检查是否在水中或雨中
    // TODO: 实现
    return isInWater();
}

void EndermanEntity::tick()
{
    // MC 1.16.5 EndermanEntity.tick()
    MonsterEntity::tick();

    // 更新瞬移冷却
    if (m_teleportCooldown > 0) {
        m_teleportCooldown--;
    }

    // 更新复仇计时器
    if (m_revengeTimer > 0) {
        m_revengeTimer--;
        if (m_revengeTimer <= 0) {
            m_revengeTargetId = std::nullopt;
        }
    }

    // 更新愤怒时间
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
            m_screaming = false;
        }
    }

    // 检查水/雨伤害
    // MC 1.16.5: 在水中或雨中受到伤害并瞬移
    if (isInWaterOrRain()) {
        // MC 1.16.5: 每tick在水中受到1.0伤害
        auto damageSource = DamageSources::drown();
        hurt(damageSource, WATER_DAMAGE);
        teleportAwayFromWater();
    }

    // 被注视时更新状态
    // TODO: 检查玩家是否正在看末影人的眼睛
}

bool EndermanEntity::hurt(DamageSource& source, f32 amount)
{
    // MC 1.16.5 EndermanEntity.attackEntityFrom()
    if (!MonsterEntity::hurt(source, amount)) {
        return false;
    }

    // 受伤后有概率瞬移
    // TODO: 如果伤害来自投射物，瞬移避让
    // TODO: 否则 50% 概率瞬移

    return true;
}

void EndermanEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // MC 1.16.5 EndermanEntity.registerGoals()
    // 优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: PanicGoal (不注册 - 末影人不会惊慌)
    // 2: MeleeAttackGoal (攻击目标)
    // 5: WaterAvoidingRandomWalkingGoal (避水随机行走)
    // 7: LookAtGoal (看向玩家，但会激怒末影人)
    // 8: LookRandomlyGoal (随机看向)
    //
    // 目标选择器：
    // 1: HurtByTargetGoal (被攻击反击)
    // 2: NearestAttackableTargetGoal<Player> (攻击注视玩家，有特殊条件)
    // 3: NearestAttackableTargetGoal<EndermiteEntity> (攻击末影螨)

    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 优先级 7: 看向玩家（会激怒末影人）
    m_goalSelector.addGoal(
        7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* /*entity*/) -> bool {
            // 只看向玩家
            // TODO: 检查是否是玩家
            return true;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));
}

void EndermanEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // MC 1.16.5 EndermanEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

} // namespace mc
