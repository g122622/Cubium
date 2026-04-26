#include "LookController.hpp"
#include "../../core/MobEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../core/Entity.hpp"
#include "../pathfinding/PathNavigator.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

LookController::LookController(MobEntity* mob)
    : m_mob(mob)
{}

void LookController::setLookPosition(f64 x, f64 y, f64 z) {
    // 从实体获取默认旋转速度
    if (m_mob) {
        setLookPosition(x, y, z, m_mob->getHorizontalFaceSpeed(), m_mob->getVerticalFaceSpeed());
    } else {
        setLookPosition(x, y, z, 10.0f, 10.0f);
    }
}

void LookController::setLookPosition(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch) {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    m_deltaLookYaw = deltaYaw;
    m_deltaLookPitch = deltaPitch;
    m_isLooking = true;
}

void LookController::setLookPositionWithEntity(const Entity& entity, f32 deltaYaw, f32 deltaPitch) {
    // MC 的 getEyePosition 实现：LivingEntity 使用 getPosYEye()，其他实体使用碰撞盒中心
    f64 eyeY = entity.y() + entity.eyeHeight();
    setLookPosition(entity.x(), eyeY, entity.z(), deltaYaw, deltaPitch);
}

void LookController::tick() {
    if (!m_mob) return;

    // 1. 首先处理俯仰角重置（MC在tick开头处理）
    if (shouldResetPitch()) {
        m_mob->setRotationPitch(0.0f);
    }

    // 2. 处理观看逻辑
    if (m_isLooking) {
        m_isLooking = false;

        // 计算目标角度并限制旋转速度
        f32 targetYaw = getTargetYaw();
        f32 targetPitch = getTargetPitch();

        // MC 使用 rotationYawHead 而非 yaw
        f32 currentYaw = m_mob->rotationYawHead();
        f32 currentPitch = m_mob->pitch();

        f32 newYaw = math::clampedRotate(currentYaw, targetYaw, m_deltaLookYaw);
        f32 newPitch = math::clampedRotate(currentPitch, targetPitch, m_deltaLookPitch);

        // MC 分别设置头部旋转和俯仰角
        m_mob->setRotationYawHead(newYaw);
        m_mob->setRotationPitch(newPitch);
    } else {
        // 3. 空闲时让头部朝向身体朝向
        f32 currentYaw = m_mob->rotationYawHead();
        f32 bodyYaw = m_mob->renderYawOffset();
        f32 newYaw = math::clampedRotate(currentYaw, bodyYaw, 10.0f);
        m_mob->setRotationYawHead(newYaw);
    }

    // 4. MC 1.16.5: 如果有导航路径，限制头部与身体的角度差
    auto* navigator = m_mob->navigator();
    if (navigator && !navigator->noPath()) {
        f32 currentYaw = m_mob->rotationYawHead();
        f32 bodyYaw = m_mob->renderYawOffset();
        f32 maxRotate = m_mob->getHorizontalFaceSpeed();
        f32 newYaw = math::clampedRotate(currentYaw, bodyYaw, maxRotate);
        m_mob->setRotationYawHead(newYaw);
    }
}

f32 LookController::getTargetYaw() const {
    if (!m_mob) return 0.0f;

    f64 dx = m_posX - m_mob->x();
    f64 dz = m_posZ - m_mob->z();

    // atan2(dz, dx) 返回弧度，转换为度数
    // MC 使用 atan2(dz, dx) * 180/PI - 90
    f32 yaw = static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);

    return yaw;
}

f32 LookController::getTargetPitch() const {
    if (!m_mob) return 0.0f;

    f64 dx = m_posX - m_mob->x();
    f64 dy = m_posY - (m_mob->y() + m_mob->eyeHeight());  // 眼睛高度
    f64 dz = m_posZ - m_mob->z();

    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 俯仰角为负值表示向上看
    f32 pitch = static_cast<f32>(-(std::atan2(dy, horizontalDist) * math::RAD_TO_DEG));

    return pitch;
}

} // namespace mc::entity::ai::controller
