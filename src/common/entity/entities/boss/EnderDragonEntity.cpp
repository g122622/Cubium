#include "EnderDragonEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// BossEntity
// ============================================================================

BossEntity::BossEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
}

// ============================================================================
// EnderDragonEntity
// ============================================================================

std::unique_ptr<Entity> EnderDragonEntity::create(IWorld* /*world*/) {
    return std::make_unique<EnderDragonEntity>(LegacyEntityType::Unknown, 0);
}

EnderDragonEntity::EnderDragonEntity(LegacyEntityType type, EntityId id)
    : BossEntity(type, id)
{
    // 初始化路径点
    // 末影龙围绕末地中心盘旋
    for (int i = 0; i < 8; ++i) {
        f32 angle = i * (MathUtils::PI * 2.0f / 8.0f);
        m_pathPoints.emplace_back(
            std::cos(angle) * 64.0f,
            64.0f,
            std::sin(angle) * 64.0f
        );
    }

    registerAttributes();
    registerGoals();
}

void EnderDragonEntity::tick() {
    BossEntity::tick();

    // 更新阶段
    m_phaseTime++;

    switch (m_phase) {
        case Phase::HoldingPattern:
            updateHoldingPattern();
            break;

        case Phase::StrafePlayer:
            // 突袭玩家
            break;

        case Phase::LandingApproach:
            // 准备降落
            break;

        case Phase::Landing:
            // 降落
            break;

        case Phase::Takeoff:
            // 起飞
            break;

        case Phase::Sitting:
            // 坐在传送门上，龙息攻击
            if (m_breathCooldown <= 0) {
                breathAttack();
                m_breathCooldown = 200;  // 10秒冷却
            } else {
                m_breathCooldown--;
            }
            break;

        case Phase::ChargingPlayer:
            chargeAttack();
            break;

        case Phase::Dying:
            updateDying();
            break;

        case Phase::Hover:
            // 悬停
            break;
    }

    // 冲撞冷却
    if (m_chargeTime > 0) {
        m_chargeTime--;
    }

    // 减少龙息冷却
    if (m_breathCooldown > 0) {
        m_breathCooldown--;
    }
}

Vector3 EnderDragonEntity::breathOrigin() const {
    // 龙息从嘴部发出
    f32 yaw = m_yaw * (MathUtils::PI / 180.0f);
    return Vector3(
        m_position.x + std::sin(yaw) * 5.0f,
        m_position.y + 2.0f,
        m_position.z + std::cos(yaw) * 5.0f
    );
}

void EnderDragonEntity::breathAttack() {
    // TODO: 生成龙息区域效果云
    // DragonFireballEntity 或 AreaEffectCloudEntity
}

void EnderDragonEntity::chargeAttack() {
    if (!m_attackTarget) {
        m_phase = Phase::HoldingPattern;
        return;
    }

    // 冲向目标
    Vector3 dir(
        m_attackTarget->x() - m_position.x,
        0.0f,
        m_attackTarget->z() - m_position.z
    );

    f32 dist = dir.length();
    if (dist > 0.0f) {
        dir = dir.normalized();
        m_velocity.x = dir.x * 2.0f;
        m_velocity.z = dir.z * 2.0f;
    }

    // 冲撞完成后返回盘旋
    if (m_phaseTime > 100) {
        m_phase = Phase::HoldingPattern;
        m_phaseTime = 0;
    }
}

void EnderDragonEntity::dragonFireballAttack() {
    if (!m_attackTarget || m_chargeTime > 0) {
        return;
    }

    // TODO: 发射龙火球
    // auto fireball = std::make_unique<DragonFireballEntity>(...);
    // fireball->shootToTarget(m_attackTarget);
    // world->spawnEntity(std::move(fireball));

    m_chargeTime = 40;  // 2秒冷却
}

void EnderDragonEntity::updatePhase() {
    // 根据生命值切换阶段
    f32 healthPercent = health() / maxHealth();

    if (healthPercent <= 0.0f) {
        m_phase = Phase::Dying;
    }
    // 更多阶段切换逻辑...
}

void EnderDragonEntity::updateHoldingPattern() {
    // 围绕末地中心盘旋
    if (m_pathPoints.empty()) {
        return;
    }

    Vector3& target = m_pathPoints[m_currentPathPoint];
    Vector3 dir = target - m_position;

    f32 dist = dir.length();
    if (dist < 10.0f) {
        // 到达当前路径点，切换到下一个
        m_currentPathPoint = (m_currentPathPoint + 1) % m_pathPoints.size();
    }

    if (dist > 0.0f) {
        dir = dir.normalized();
        m_velocity.x = dir.x * 0.5f;
        m_velocity.y = dir.y * 0.2f;
        m_velocity.z = dir.z * 0.5f;
    }

    // 随机攻击玩家
    if (m_attackTarget && m_phaseTime > 600) {  // 30秒后可能攻击
        // 有概率切换到冲撞阶段
        m_phase = Phase::ChargingPlayer;
        m_phaseTime = 0;
    }
}

void EnderDragonEntity::updateDying() {
    m_deathTime++;

    // 死亡动画
    m_velocity.y = -0.1f;  // 缓慢下降

    // 生成爆炸粒子
    if (m_deathTime % 10 == 0) {
        // TODO: 生成爆炸粒子
    }

    // 死亡后生成经验和龙蛋
    if (m_deathTime > 200) {  // 10秒死亡动画
        // TODO: 生成经验球和龙蛋
        // TODO: 生成传送门

        remove();
    }
}

void EnderDragonEntity::respawnDragon(IWorld* world, BlockCoord portalPos) {
    // TODO: 实现末影龙重生逻辑
    // 1. 放置末影水晶
    // 2. 等待重生动画
    // 3. 生成末影龙
}

void EnderDragonEntity::registerGoals() {
    MobEntity::registerGoals();

    // 末影龙使用特殊阶段系统，不使用普通AI目标
}

void EnderDragonEntity::registerAttributes() {
    MobEntity::registerAttributes();

    // 末影龙属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 200.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 256.0);
}

// ============================================================================
// EnderDragonPartEntity
// ============================================================================

EnderDragonPartEntity::EnderDragonPartEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
}

void EnderDragonPartEntity::tick() {
    Entity::tick();

    // 部件位置由父龙更新
}

void EnderDragonPartEntity::updatePosition(f32 offsetX, f32 offsetY, f32 offsetZ, f32 size) {
    if (!m_parent) {
        return;
    }

    // 根据父龙的旋转计算实际位置
    f32 yaw = m_parent->yaw() * (MathUtils::PI / 180.0f);

    m_position.x = m_parent->x() + std::cos(yaw) * offsetX - std::sin(yaw) * offsetZ;
    m_position.y = m_parent->y() + offsetY;
    m_position.z = m_parent->z() + std::sin(yaw) * offsetX + std::cos(yaw) * offsetZ;
}

} // namespace entity
} // namespace mc
