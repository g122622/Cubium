#include "LookAtGoal.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../GoalConstants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../controller/LookController.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

// ==================== LookAtGoal ====================

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance)
    : LookAtGoal(mob, maxDistance, DEFAULT_LOOK_CHANCE)
{
}

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance)
    : m_mob(mob)
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookAtGoal::shouldExecute() {
    if (!m_mob) return false;

    // 检查概率
    math::Random rng = m_mob->getRandom();
    if (rng.nextFloat() >= m_chance) {
        return false;
    }

    // 寻找目标
    m_lookTarget = findTarget();
    return m_lookTarget != nullptr;
}

bool LookAtGoal::shouldContinueExecuting() {
    if (!m_lookTarget) return false;

    // 检查目标是否存活
    if (!m_lookTarget->isAlive()) return false;

    // 检查距离（水平距离）
    f32 distSq = m_mob->distanceHorizontalSqTo(m_lookTarget->x(), m_lookTarget->z());
    f32 maxDistSq = m_maxDistance * m_maxDistance;

    if (distSq > maxDistSq) {
        return false;
    }

    // 检查剩余时间
    return m_lookTime > 0;
}

void LookAtGoal::startExecuting() {
    // 设置看向时间
    math::Random rng = m_mob->getRandom();
    m_lookTime = LOOK_AT_MIN_TIME + rng.nextInt(LOOK_AT_MAX_TIME - LOOK_AT_MIN_TIME);
}

void LookAtGoal::resetTask() {
    m_lookTarget = nullptr;
}

void LookAtGoal::tick() {
    if (!m_mob || !m_lookTarget) return;

    // 使用视线控制器看向目标
    m_mob->lookAt(*m_lookTarget);

    m_lookTime--;
}

LivingEntity* LookAtGoal::findTarget() {
    if (!m_mob) return nullptr;

    return EntityUtils::findClosestEntity<LivingEntity>(
        m_mob->world(),
        m_mob->position(),
        m_maxDistance,
        m_mob
    );
}

// ==================== LookRandomlyGoal ====================

LookRandomlyGoal::LookRandomlyGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookRandomlyGoal::shouldExecute() {
    if (!m_mob) return false;

    // 默认概率执行
    math::Random rng = m_mob->getRandom();
    return rng.nextFloat() < RANDOM_LOOK_CHANCE;
}

bool LookRandomlyGoal::shouldContinueExecuting() {
    return m_lookTime > 0;
}

void LookRandomlyGoal::startExecuting() {
    if (!m_mob) return;

    // 设置随机看往方向
    math::Random rng = m_mob->getRandom();

    // 随机偏航角（-180 到 180）
    m_targetYaw = rng.nextFloat() * 360.0f - 180.0f;

    // 随机俯仰角（-30 到 30）
    m_targetPitch = rng.nextFloat() * 60.0f - 30.0f;

    // 看往时间
    m_lookTime = RANDOM_LOOK_MIN_TIME + rng.nextInt(RANDOM_LOOK_MAX_TIME - RANDOM_LOOK_MIN_TIME);
}

void LookRandomlyGoal::resetTask() {
    m_lookTime = 0;
}

void LookRandomlyGoal::tick() {
    if (!m_mob) return;

    // 使用视线控制器看向往方向
    if (auto* lookCtrl = m_mob->lookController()) {
        // 计算目标位置（使用当前坐标加方向向量）
        f32 yawRad = m_targetYaw * math::DEG_TO_RAD;
        f32 pitchRad = m_targetPitch * math::DEG_TO_RAD;

        f32 dx = std::cos(pitchRad) * std::cos(yawRad);
        f32 dy = -std::sin(pitchRad);
        f32 dz = std::cos(pitchRad) * std::sin(yawRad);

        f64 lookX = m_mob->x() + dx * 10.0;
        f64 lookY = m_mob->y() + m_mob->eyeHeight() + dy * 10.0;
        f64 lookZ = m_mob->z() + dz * 10.0;

        lookCtrl->setLookPosition(lookX, lookY, lookZ);
    }

    m_lookTime--;
}

} // namespace mc::entity::ai::goal
