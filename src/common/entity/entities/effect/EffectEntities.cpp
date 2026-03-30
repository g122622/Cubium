#include "EffectEntities.hpp"
#include "../../world/IWorld.hpp"
#include "../../entities/player/Player.hpp"
#include "../../core/LivingEntity.hpp"
#include <cmath>
#include <algorithm>

namespace mc {
namespace entity {

// ==================== EnderCrystalEntity ====================

EnderCrystalEntity::EnderCrystalEntity()
    : Entity()
{
    setSize(2.0f, 2.0f);
    setMaxHealth(1.0f);
}

void EnderCrystalEntity::tick() {
    Entity::tick();

    // 治愈末影龙冷却
    if (m_healCooldown > 0) {
        m_healCooldown--;
    }

    // 生成光束粒子效果
    // TODO: 生成粒子
}

void EnderCrystalEntity::onEntityCollision(Entity& other) {
    // 碰撞时爆炸
    if (other.isAlive()) {
        explode();
    }
}

void EnderCrystalEntity::setBeamTarget(BlockPos pos) {
    m_beamTarget = pos;
}

void EnderCrystalEntity::healDragon() {
    // TODO: 找到末影龙并治愈
    // EnderDragonEntity* dragon = findNearestDragon();
    // if (dragon && dragon->getHealth() < dragon->getMaxHealth()) {
    //     dragon->heal(1.0f);
    //     m_healCooldown = HEAL_COOLDOWN;
    // }
}

void EnderCrystalEntity::explode() {
    // TODO: 创建爆炸
    // if (world) {
    //     world->createExplosion(position(), EXPLOSION_RADIUS, true, false);
    // }
    remove();
}

// ==================== LightningBoltEntity ====================

LightningBoltEntity::LightningBoltEntity()
    : Entity()
{
    setSize(0.0f, 0.0f);
    setMaxHealth(1.0f);
}

void LightningBoltEntity::tick() {
    Entity::tick();

    m_ticksLived++;

    // 等待一帧后再造成伤害
    if (m_ticksLived == 1 && !m_effectOnly) {
        damageEntities();
        spawnFire();
    }

    // 闪电视觉效果
    if (m_ticksLived < 2) {
        // 闪光效果
        m_flashCount++;
    }

    // 闪电在30tick后消失
    if (m_ticksLived >= LIFETIME) {
        remove();
    }
}

void LightningBoltEntity::damageEntities() {
    if (m_effectOnly) return;

    // TODO: 伤害周围实体
    // auto entities = world->getEntitiesInRange(position(), DAMAGE_RADIUS);
    // for (auto* entity : entities) {
    //     if (auto* living = dynamic_cast<LivingEntity*>(entity)) {
    //         living->hurt(this, DAMAGE_AMOUNT);
    //         // 雷击可能产生火焰
    //         living->setFire(8);
    //     }
    // }
}

void LightningBoltEntity::spawnFire() {
    if (m_effectOnly) return;

    // TODO: 在闪电击中位置生成火焰
    // world->setBlockState(position(), Blocks::FIRE.getDefaultState());
}

void LightningBoltEntity::triggerLightningEffect() {
    // TODO: 触发闪电事件（用于成就等）
    // world->onLightningStrike(this);
}

// ==================== AreaEffectCloudEntity ====================

AreaEffectCloudEntity::AreaEffectCloudEntity()
    : Entity()
{
    setSize(6.0f, 0.5f);
    setMaxHealth(1.0f);
}

void AreaEffectCloudEntity::tick() {
    Entity::tick();

    m_ticksLived++;

    // 等待时间结束后开始应用效果
    if (m_ticksLived > m_waitTime) {
        // 每隔一段时间应用效果
        if (m_ticksLived % m_reapplicationDelay == 0) {
            applyEffects();
        }

        // 更新半径
        updateRadius();

        // 检查是否过期
        if (m_ticksLived >= m_duration) {
            remove();
        }
    }
}

void AreaEffectCloudEntity::applyEffects() {
    // TODO: 应用效果到范围内的实体
    // auto entities = world->getEntitiesInRange(position(), m_radius);
    // for (auto* entity : entities) {
    //     if (auto* living = dynamic_cast<LivingEntity*>(entity)) {
    //         for (const auto& effect : m_effects) {
    //             living->addEffect(effect);
    //         }
    //     }
    // }

    // 缩短持续时间
    if (m_durationOnUse > 0) {
        m_duration = std::max(0, m_duration - m_durationOnUse);
    }
}

void AreaEffectCloudEntity::updateRadius() {
    // 半径随时间减小
    m_radius += RADIUS_GROWTH;
    m_radius = std::max(0.5f, m_radius);

    // 更新碰撞箱
    setSize(m_radius * 2.0f, 0.5f);
}

// ==================== ExperienceOrbEntity ====================

ExperienceOrbEntity::ExperienceOrbEntity(i32 xpValue)
    : Entity()
    , m_xpValue(xpValue)
{
    setSize(0.5f, 0.5f);
    setMaxHealth(1.0f);
}

void ExperienceOrbEntity::tick() {
    Entity::tick();

    // 减少收集延迟
    if (m_collectDelay > 0) {
        m_collectDelay--;
    }

    // 寻找附近的玩家
    // TODO: 获取附近玩家
    // Player* nearestPlayer = findNearestPlayer(FOLLOW_RANGE);
    // if (nearestPlayer && m_collectDelay <= 0) {
    //     m_trackingPlayer = nearestPlayer;
    // }

    // 追踪玩家
    if (m_trackingPlayer && m_trackingPlayer->isAlive()) {
        followPlayer(m_trackingPlayer);
    }

    // 自然消失
    m_despawnDelay--;
    if (m_despawnDelay <= 0) {
        remove();
    }

    // 物理运动
    m_velocityY -= 0.03; // 重力
    move(m_velocityX, m_velocityY, m_velocityZ);

    // 减速
    m_velocityX *= 0.98;
    m_velocityY *= 0.98;
    m_velocityZ *= 0.98;
}

void ExperienceOrbEntity::onEntityCollision(Entity& other) {
    Player* player = dynamic_cast<Player*>(&other);
    if (player && m_collectDelay <= 0) {
        // 玩家拾取经验球
        // player->giveExperience(m_xpValue);
        player->onEntityCollision(*this);
        remove();
    }
}

ExperienceOrbEntity* ExperienceOrbEntity::split() {
    if (m_xpValue <= 1) return nullptr;

    // 分割成更小的经验球
    i32 splitValue = m_xpValue / 2;
    m_xpValue -= splitValue;

    // TODO: 创建新的经验球
    // return new ExperienceOrbEntity(splitValue);
    return nullptr;
}

u32 ExperienceOrbEntity::getExperienceColor() const {
    // 根据经验值返回不同颜色
    if (m_xpValue <= 5) return 0xFFAA00FF;      // 小经验：黄色
    if (m_xpValue <= 20) return 0xFF55FFFF;     // 中等经验：绿色
    if (m_xpValue <= 100) return 0xFF55FF55;    // 大经验：青色
    return 0xFFFF5555;                          // 超大经验：红色
}

void ExperienceOrbEntity::followPlayer(Player* player) {
    if (!player) return;

    // 计算方向
    f64 dx = player->x() - x();
    f64 dy = player->y() - y();
    f64 dz = player->z() - z();
    f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (dist > 0.0 && dist < FOLLOW_RANGE) {
        f32 speed = FOLLOW_SPEED * (1.0f - static_cast<f32>(dist / FOLLOW_RANGE));
        m_velocityX += (dx / dist) * speed;
        m_velocityY += (dy / dist) * speed;
        m_velocityZ += (dz / dist) * speed;
    }
}

// ==================== ArmorStandEntity ====================

ArmorStandEntity::ArmorStandEntity()
    : Entity()
{
    setSize(0.5f, 1.975f);
    setMaxHealth(1.0f);
}

void ArmorStandEntity::tick() {
    Entity::tick();

    // 如果不是标记模式，应用重力
    if (!m_marker && m_hasGravity) {
        m_velocityY -= 0.04; // 重力
        move(m_velocityX, m_velocityY, m_velocityZ);

        // 减速
        m_velocityX *= 0.98;
        m_velocityY *= 0.98;
        m_velocityZ *= 0.98;
    }
}

void ArmorStandEntity::onEntityCollision(Entity& other) {
    // 盔甲架不能被推动
    // 但是可以被玩家交互
}

void ArmorStandEntity::setHeadRotation(f32 x, f32 y, f32 z) {
    m_head = {x, y, z};
}

void ArmorStandEntity::setBodyRotation(f32 x, f32 y, f32 z) {
    m_body = {x, y, z};
}

void ArmorStandEntity::setLeftArmRotation(f32 x, f32 y, f32 z) {
    m_leftArm = {x, y, z};
}

void ArmorStandEntity::setRightArmRotation(f32 x, f32 y, f32 z) {
    m_rightArm = {x, y, z};
}

void ArmorStandEntity::setLeftLegRotation(f32 x, f32 y, f32 z) {
    m_leftLeg = {x, y, z};
}

void ArmorStandEntity::setRightLegRotation(f32 x, f32 y, f32 z) {
    m_rightLeg = {x, y, z};
}

} // namespace entity
} // namespace mc
