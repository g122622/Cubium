#include "MiscEntities.hpp"
#include "../../world/IWorld.hpp"
#include "../../entities/player/Player.hpp"
#include "../../core/LivingEntity.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ==================== FallingBlockEntity ====================

FallingBlockEntity::FallingBlockEntity()
    : Entity()
{
    setSize(0.98f, 0.98f);
    setMaxHealth(1.0f);
}

FallingBlockEntity::FallingBlockEntity(const BlockState& block)
    : Entity()
    , m_block(std::make_unique<BlockState>(block))
{
    setSize(0.98f, 0.98f);
    setMaxHealth(1.0f);
    m_fallStartY = y();
}

void FallingBlockEntity::tick() {
    Entity::tick();

    m_fallTime++;

    // 应用重力
    m_velocityY -= 0.04;

    // 移动
    move(m_velocityX, m_velocityY, m_velocityZ);

    // 减速
    m_velocityX *= 0.98;
    m_velocityY *= 0.98;
    m_velocityZ *= 0.98;

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

void FallingBlockEntity::setBlockState(const BlockState& block) {
    m_block = std::make_unique<BlockState>(block);
}

void FallingBlockEntity::handleLanding() {
    if (!m_block) {
        remove();
        return;
    }

    // 检查是否应该伤害实体
    if (m_hurtEntities) {
        f64 fallDistance = m_fallStartY - y();
        if (fallDistance > 0) {
            // 伤害下方的实体
            // auto entities = world->getEntitiesAt(position());
            // for (auto* entity : entities) {
            //     if (auto* living = dynamic_cast<LivingEntity*>(entity)) {
            //         i32 damage = std::min(static_cast<i32>(fallDistance), MAX_HURT_AMOUNT);
            //         living->hurt(DamageSource::fallingBlock(this), damage * HURT_AMOUNT);
            //     }
            // }
        }
    }

    // 放置方块或掉落物品
    if (m_placeBlock) {
        // TODO: 尝试放置方块
        // if (world->setBlockState(position(), *m_block)) {
        //     remove();
        //     return;
        // }
    }

    // 无法放置，掉落物品
    // TODO: 生成物品
    remove();
}

// ==================== TNTEntity ====================

TNTEntity::TNTEntity()
    : Entity()
{
    setSize(0.98f, 0.98f);
    setMaxHealth(1.0f);
}

TNTEntity::TNTEntity(f64 x, f64 y, f64 z)
    : Entity()
{
    setPosition(x, y, z);
    setSize(0.98f, 0.98f);
    setMaxHealth(1.0f);
}

void TNTEntity::tick() {
    Entity::tick();

    if (m_fuse > 0) {
        m_fuse--;

        // 视觉效果
        // TODO: 生成烟雾粒子

        if (m_fuse <= 0 && !m_exploded) {
            explode();
        }
    }

    // 重力
    m_velocityY -= 0.04;

    // 移动
    move(m_velocityX, m_velocityY, m_velocityZ);

    // 减速
    m_velocityX *= 0.98;
    m_velocityY *= 0.98;
    m_velocityZ *= 0.98;
}

void TNTEntity::ignite() {
    m_fuse = DEFAULT_FUSE;
}

void TNTEntity::explode() {
    if (m_exploded) return;
    m_exploded = true;

    // TODO: 创建爆炸
    // if (world) {
    //     world->createExplosion(position(), m_explosionRadius, true, true);
    // }

    remove();
}

// ==================== EyeOfEnderEntity ====================

EyeOfEnderEntity::EyeOfEnderEntity()
    : Entity()
{
    setSize(0.25f, 0.25f);
    setMaxHealth(1.0f);
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
            // 飞行
            f32 speed = 0.3f;
            m_velocityX = (dx / dist) * speed;
            m_velocityY = (dy / dist) * speed;
            m_velocityZ = (dz / dist) * speed;
        } else {
            // 到达目标
            if (m_shatter && (rand() % 5) == 0) {
                // 碎裂掉落
                // TODO: 生成末影之眼物品
            }
            remove();
        }
    }

    // 最大生存时间
    if (m_lifeTime >= MAX_LIFE) {
        if (m_shatter) {
            // TODO: 生成末影之眼物品
        }
        remove();
    }

    // 移动
    move(m_velocityX, m_velocityY, m_velocityZ);
}

void EyeOfEnderEntity::setTargetPos(f64 x, f64 y, f64 z) {
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
}

// ==================== ConduitEntity ====================

ConduitEntity::ConduitEntity()
    : Entity()
{
    setSize(2.0f, 2.0f);
    setMaxHealth(1.0f);
}

void ConduitEntity::tick() {
    Entity::tick();

    if (!m_active) return;

    // 应用效果
    m_effectCooldown--;
    if (m_effectCooldown <= 0) {
        applyEffects();
        m_effectCooldown = EFFECT_INTERVAL;
    }

    // 攻击目标
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
    // auto players = world->getPlayersInRange(position(), EFFECT_RADIUS);
    // for (auto* player : players) {
    //     player->addEffect(Effects::CONDUIT_POWER, 260, 0, false, true);
    // }
}

void ConduitEntity::attackTarget() {
    if (!m_target || !m_target->isAlive()) {
        m_target = nullptr;
        return;
    }

    // 检查距离
    f32 dist = static_cast<f32>(distanceTo(*m_target));
    if (dist > ATTACK_RADIUS) {
        m_target = nullptr;
        return;
    }

    // 伤害目标
    // m_target->hurt(DamageSource::magic(this), 4.0f);
}

// ==================== WardenWarningEffect ====================

void WardenWarningEffect::tick() {
    if (m_cooldown > 0) {
        m_cooldown--;
    } else {
        // 逐渐降低警告级别
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
    : Entity()
{
    setSize(0.5f, 0.8f);
    setMaxHealth(1.0f);
}

EvokerFangsEntity::EvokerFangsEntity(f64 x, f64 y, f64 z, f32 yaw, i32 delay)
    : Entity()
    , m_delay(delay)
{
    setPosition(x, y, z);
    setRotation(yaw, 0.0f);
    setSize(0.5f, 0.8f);
    setMaxHealth(1.0f);
}

void EvokerFangsEntity::tick() {
    Entity::tick();

    m_ticksLived++;

    if (m_ticksLived < m_delay) {
        return;
    }

    // 尖牙冒出动画
    if (m_ticksLived == m_delay + WARMUP_DELAY) {
        // 开始攻击
        attackEntities();
        m_hasAttacked = true;
    }

    // 生命周期结束
    if (m_ticksLived >= m_delay + LIFETIME) {
        remove();
    }
}

void EvokerFangsEntity::attackEntities() {
    if (m_hasAttacked) return;

    // 伤害范围内的实体
    // auto entities = world->getEntitiesInRange(position(), 0.5f);
    // for (auto* entity : entities) {
    //     if (auto* living = dynamic_cast<LivingEntity*>(entity)) {
    //         if (living != m_owner) {
    //             living->hurt(DamageSource::magic(this), m_damage);
    //         }
    //     }
    // }
}

} // namespace entity
} // namespace mc
