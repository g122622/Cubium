#include "MiscEntities.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
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
    checkOnGround();

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
    setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
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
    checkOnGround();

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

    IWorld* worldPtr = world();
    if (worldPtr) {
        // TNT 爆炸半径 4.0，模式 BREAK（破坏方块但不掉落物品）
        // 爆炸位置在 TNT 底部（Y 偏移 0.0625）
        worldPtr->createExplosion(
            Vector3(x(), y() + 0.0625f, z()),
            m_explosionRadius,
            world::explosion::ExplosionMode::Break,
            false,  // 不生成火焰
            this    // 爆炸源实体
        );
    }

    remove();
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

} // namespace entity
} // namespace mc
