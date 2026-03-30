#include "LookController.hpp"
#include "../../core/MobEntity.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

LookController::LookController(MobEntity* mob)
    : m_mob(mob)
{}

void LookController::setLookPosition(f64 x, f64 y, f64 z) {
    // 默认旋转速度
    setLookPosition(x, y, z, 10.0f, 10.0f);
}

void LookController::setLookPosition(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch) {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    m_deltaLookYaw = deltaYaw;
    m_deltaLookPitch = deltaPitch;
    m_isLooking = true;
}

void LookController::tick() {
    if (!m_mob) return;

    // TODO: 重置俯仰角（如果需要）
    // if (shouldResetPitch()) {
    //     m_mob->setPitch(0.0f);
    // }

    if (m_isLooking) {
        m_isLooking = false;

        // 计算目标角度并限制旋转速度
        f32 targetYaw = getTargetYaw();
        f32 targetPitch = getTargetPitch();

        f32 currentYaw = m_mob->yaw();
        f32 currentPitch = m_mob->pitch();

        f32 newYaw = clampedRotate(currentYaw, targetYaw, m_deltaLookYaw);
        f32 newPitch = clampedRotate(currentPitch, targetPitch, m_deltaLookPitch);

        m_mob->setRotation(newYaw, newPitch);
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

f32 LookController::clampedRotate(f32 from, f32 to, f32 maxDelta) {
    return math::clampAngle(from, to, maxDelta);
}

} // namespace mc::entity::ai::controller
