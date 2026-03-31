#include "EffectEntities.hpp"
#include "../../../world/IWorld.hpp"
#include "../player/Player.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../core/Types.hpp"
#include <cmath>
#include <algorithm>

namespace mc {
namespace entity {

// ==================== EnderCrystalEntity ====================

EnderCrystalEntity::EnderCrystalEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
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

bool EnderCrystalEntity::hasBeamTarget() const {
    return m_beamTarget.x != 0 || m_beamTarget.y != 0 || m_beamTarget.z != 0;
}

void EnderCrystalEntity::setBeamTarget(BlockPos pos) {
    m_beamTarget = pos;
}

void EnderCrystalEntity::healDragon() {
    // TODO: 找到末影龙并治愈
}

void EnderCrystalEntity::explode() {
    // TODO: 创建爆炸
    remove();
}

// ==================== LightningBoltEntity ====================

LightningBoltEntity::LightningBoltEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
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
}

void LightningBoltEntity::spawnFire() {
    if (m_effectOnly) return;
    // TODO: 在闪电击中位置生成火焰
}

void LightningBoltEntity::triggerLightningEffect() {
    // TODO: 触发闪电事件
}

// ==================== AreaEffectCloudEntity ====================

AreaEffectCloudEntity::AreaEffectCloudEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
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
    if (m_durationOnUse > 0) {
        m_duration = std::max(0, m_duration - m_durationOnUse);
    }
}

void AreaEffectCloudEntity::updateRadius() {
    m_radius += RADIUS_GROWTH;
    m_radius = std::max(0.5f, m_radius);
}

// 注意: ExperienceOrbEntity 已移动到独立的 orb/ 目录
// 文件位置: src/common/entity/entities/orb/ExperienceOrbEntity.cpp

// ==================== ArmorStandEntity ====================

ArmorStandEntity::ArmorStandEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

void ArmorStandEntity::tick() {
    Entity::tick();

    // 如果不是标记模式，应用重力
    if (!m_marker && m_hasGravity) {
        Vector3 vel = velocity();
        vel.y -= 0.04f; // 重力
        move(vel.x, vel.y, vel.z);

        // 减速
        vel.x *= 0.98f;
        vel.y *= 0.98f;
        vel.z *= 0.98f;
        setVelocity(vel);
    }
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
