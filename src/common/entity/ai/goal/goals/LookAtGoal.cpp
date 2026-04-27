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

    // MC 1.16.5: 检查概率
    math::Random rng = m_mob->getRandom();
    if (rng.nextFloat() >= m_chance) {
        return false;
    }

    // MC 1.16.5: 首先检查攻击目标
    if (m_mob->attackTarget() != nullptr) {
        m_lookTarget = m_mob->attackTarget();
        return true;
    }

    // MC 1.16.5: 寻找最近的实体
    // 使用 getBoundingBox().grow(maxDistance, 3.0D, maxDistance) 扩展范围
    m_lookTarget = findTarget();
    return m_lookTarget != nullptr;
}

bool LookAtGoal::shouldContinueExecuting() {
    if (!m_lookTarget) return false;

    // MC 1.16.5: 检查目标是否存活
    if (!m_lookTarget->isAlive()) return false;

    // MC 1.16.5: 使用距离平方比较
    f64 distSq = m_mob->distanceSqTo(*m_lookTarget);
    f64 maxDistSq = static_cast<f64>(m_maxDistance) * static_cast<f64>(m_maxDistance);

    if (distSq > maxDistSq) {
        return false;
    }

    // MC 1.16.5: 检查剩余时间
    return m_lookTime > 0;
}

void LookAtGoal::startExecuting() {
    // MC 1.16.5: 设置看向时间 (40 + random.nextInt(40))
    math::Random rng = m_mob->getRandom();
    m_lookTime = LOOK_AT_MIN_TIME + rng.nextInt(LOOK_AT_MAX_TIME - LOOK_AT_MIN_TIME);
}

void LookAtGoal::resetTask() {
    m_lookTarget = nullptr;
}

void LookAtGoal::tick() {
    if (!m_mob || !m_lookTarget) return;

    // MC 1.16.5: 使用 LookController 看向目标眼睛位置
    // getLookController().setLookPosition(posX, getPosYEye(), posZ)
    if (auto* lookCtrl = m_mob->lookController()) {
        f64 eyeY = m_lookTarget->y() + m_lookTarget->eyeHeight();
        lookCtrl->setLookPosition(m_lookTarget->x(), eyeY, m_lookTarget->z());
    }

    m_lookTime--;
}

LivingEntity* LookAtGoal::findTarget() {
    if (!m_mob) return nullptr;

    // MC 1.16.5: 使用 world.getClosestPlayer 或 getEntitiesWithinAABB
    // 当前使用 EntityUtils 实现
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
    // MC 1.16.5: LookRandomlyGoal 使用 MOVE 和 LOOK 标志
    // 原版: EnumSet.of(Goal.Flag.MOVE, Goal.Flag.LOOK)
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool LookRandomlyGoal::shouldExecute() {
    if (!m_mob) return false;

    // MC 1.16.5: 默认概率执行 (0.02f)
    math::Random rng = m_mob->getRandom();
    return rng.nextFloat() < RANDOM_LOOK_CHANCE;
}

bool LookRandomlyGoal::shouldContinueExecuting() {
    return m_lookTime > 0;
}

void LookRandomlyGoal::startExecuting() {
    if (!m_mob) return;

    // MC 1.16.5: 设置随机看往方向
    math::Random rng = m_mob->getRandom();

    // MC 1.16.5: 随机偏航角
    m_targetYaw = rng.nextFloat() * 360.0f - 180.0f;

    // MC 1.16.5: 随机俯仰角
    m_targetPitch = rng.nextFloat() * 60.0f - 30.0f;

    // MC 1.16.5: 看往时间 (20 + random.nextInt(20))
    m_lookTime = RANDOM_LOOK_MIN_TIME + rng.nextInt(RANDOM_LOOK_MAX_TIME - RANDOM_LOOK_MIN_TIME);
}

void LookRandomlyGoal::resetTask() {
    m_lookTime = 0;
}

void LookRandomlyGoal::tick() {
    if (!m_mob) return;

    // MC 1.16.5: 使用视线控制器看向往方向
    // getLookController().setLookPosition(eyeX + dirX, eyeY + dirY, eyeZ + dirZ)
    if (auto* lookCtrl = m_mob->lookController()) {
        // 计算方向向量
        f64 yawRad = static_cast<f64>(m_targetYaw) * math::DEG_TO_RAD;
        f64 pitchRad = static_cast<f64>(m_targetPitch) * math::DEG_TO_RAD;

        f64 dx = std::cos(pitchRad) * std::cos(yawRad);
        f64 dy = -std::sin(pitchRad);
        f64 dz = std::cos(pitchRad) * std::sin(yawRad);

        // 计算目标位置（实体眼睛位置 + 方向向量 * 10）
        f64 eyeX = m_mob->x();
        f64 eyeY = m_mob->y() + m_mob->eyeHeight();
        f64 eyeZ = m_mob->z();

        f64 lookX = eyeX + dx * 10.0;
        f64 lookY = eyeY + dy * 10.0;
        f64 lookZ = eyeZ + dz * 10.0;

        lookCtrl->setLookPosition(lookX, lookY, lookZ);
    }

    m_lookTime--;
}

} // namespace mc::entity::ai::goal
