#include "MiscEntities.hpp"
#include "../../../world/IWorld.hpp"
#include "../player/Player.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../core/Types.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ==================== FallingBlockEntity ====================

FallingBlockEntity::FallingBlockEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

void FallingBlockEntity::tick() {
    Entity::tick();

    m_fallTime++;

    // 应用重力
    Vector3 vel = velocity();
    vel.y -= 0.04f;

    // 移动
    move(vel.x, vel.y, vel.z);

    // 减速
    vel.x *= 0.98f;
    vel.y *= 0.98f;
    vel.z *= 0.98f;
    setVelocity(vel);

    // 检查是否落地
    if (onGround()) {
        handleLanding();
    }

    // 超过一定时间后自动放置
    if (m_fallTime > 600) {
        m_placeBlock = true;
        handleLanding();
    }
}

void FallingBlockEntity::handleLanding() {
    // 检查是否应该伤害实体
    if (m_hurtEntities) {
        f64 fallDistance = m_fallStartY - y();
        if (fallDistance > 0) {
            // 伤害下方的实体
        }
    }

    // 放置方块或掉落物品
    if (m_placeBlock) {
        // TODO: 尝试放置方块
    }

    // 无法放置，掉落物品
    remove();
}

// ==================== TNTEntity ====================

TNTEntity::TNTEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

TNTEntity::TNTEntity(f64 x, f64 y, f64 z)
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
    setPosition(x, y, z);
}

void TNTEntity::tick() {
    Entity::tick();

    if (m_fuse > 0) {
        m_fuse--;

        if (m_fuse <= 0 && !m_exploded) {
            explode();
        }
    }

    // 重力
    Vector3 vel = velocity();
    vel.y -= 0.04f;

    // 移动
    move(vel.x, vel.y, vel.z);

    // 减速
    vel.x *= 0.98f;
    vel.y *= 0.98f;
    vel.z *= 0.98f;
    setVelocity(vel);
}

void TNTEntity::ignite() {
    m_fuse = DEFAULT_FUSE;
}

void TNTEntity::explode() {
    if (m_exploded) return;
    m_exploded = true;
    // TODO: 创建爆炸
    remove();
}

// ==================== EyeOfEnderEntity ====================

EyeOfEnderEntity::EyeOfEnderEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

void EyeOfEnderEntity::tick() {
    Entity::tick();

    m_lifeTime++;

    // 飞向目标
    if (m_targetX != 0.0 || m_targetY != 0.0 || m_targetZ != 0.0) {
        f64 dx = m_targetX - x();
        f64 dy = m_targetY - y();
        f64 dz = m_targetZ - z();
        f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist > 1.0) {
            f32 speed = 0.3f;
            setVelocity(
                static_cast<f32>((dx / dist) * speed),
                static_cast<f32>((dy / dist) * speed),
                static_cast<f32>((dz / dist) * speed)
            );
        } else {
            if (m_shatter && (rand() % 5) == 0) {
                // 碎裂掉落
            }
            remove();
        }
    }

    // 最大生存时间
    if (m_lifeTime >= MAX_LIFE) {
        if (m_shatter) {
            // 生成末影之眼物品
        }
        remove();
    }

    // 移动
    Vector3 vel = velocity();
    move(vel.x, vel.y, vel.z);
}

void EyeOfEnderEntity::setTargetPos(f64 x, f64 y, f64 z) {
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
}

// ==================== ConduitEntity ====================

ConduitEntity::ConduitEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

void ConduitEntity::tick() {
    Entity::tick();

    if (!m_active) return;

    m_effectCooldown--;
    if (m_effectCooldown <= 0) {
        applyEffects();
        m_effectCooldown = EFFECT_INTERVAL;
    }

    if (m_target) {
        m_attackCooldown--;
        if (m_attackCooldown <= 0) {
            attackTarget();
            m_attackCooldown = ATTACK_INTERVAL;
        }
    }
}

void ConduitEntity::setTarget(LivingEntity* target) {
    m_target = target;
}

void ConduitEntity::applyEffects() {
    // TODO: 给附近玩家潮涌能量效果
}

void ConduitEntity::attackTarget() {
    if (!m_target || !m_target->isAlive()) {
        m_target = nullptr;
        return;
    }

    f32 dist = static_cast<f32>(distanceTo(*m_target));
    if (dist > ATTACK_RADIUS) {
        m_target = nullptr;
        return;
    }
}

// ==================== WardenWarningEffect ====================

void WardenWarningEffect::tick() {
    if (m_cooldown > 0) {
        m_cooldown--;
    } else {
        if (m_warningLevel > 0) {
            m_warningLevel--;
            m_cooldown = DECREASE_INTERVAL;
        }
    }
}

void WardenWarningEffect::increaseWarning() {
    if (m_warningLevel < MAX_WARNING) {
        m_warningLevel++;
        m_cooldown = DECREASE_INTERVAL;
    }
}

void WardenWarningEffect::decreaseWarning() {
    if (m_warningLevel > 0) {
        m_warningLevel--;
    }
}

// ==================== EvokerFangsEntity ====================

EvokerFangsEntity::EvokerFangsEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

EvokerFangsEntity::EvokerFangsEntity(f64 x, f64 y, f64 z, f32 yaw, i32 delay)
    : Entity(LegacyEntityType::Unknown, EntityId(0))
    , m_delay(delay)
{
    setPosition(x, y, z);
    setRotation(yaw, 0.0f);
}

void EvokerFangsEntity::tick() {
    Entity::tick();

    m_ticksLived++;

    if (m_ticksLived < m_delay) {
        return;
    }

    if (m_ticksLived == m_delay + WARMUP_DELAY) {
        attackEntities();
        m_hasAttacked = true;
    }

    if (m_ticksLived >= m_delay + LIFETIME) {
        remove();
    }
}

void EvokerFangsEntity::attackEntities() {
    if (m_hasAttacked) return;
    // 伤害范围内的实体
}

} // namespace entity
} // namespace mc
