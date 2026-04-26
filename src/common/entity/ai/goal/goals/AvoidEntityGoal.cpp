#include "AvoidEntityGoal.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../GoalConstants.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

AvoidEntityGoal::AvoidEntityGoal(CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed)
    : m_creature(creature)
    , m_avoidDistance(avoidDistance)
    , m_farSpeed(farSpeed)
    , m_nearSpeed(nearSpeed)
    , m_predicate(nullptr)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

AvoidEntityGoal::AvoidEntityGoal(CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed, EntityPredicate predicate)
    : m_creature(creature)
    , m_avoidDistance(avoidDistance)
    , m_farSpeed(farSpeed)
    , m_nearSpeed(nearSpeed)
    , m_predicate(std::move(predicate))
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool AvoidEntityGoal::shouldExecute() {
    if (!m_creature) return false;

    // 寻找要避开的实体
    m_avoidTarget = findEntityToAvoid();
    if (!m_avoidTarget) {
        return false;
    }

    // 寻找逃跑位置
    return findEscapePosition();
}

bool AvoidEntityGoal::shouldContinueExecuting() {
    if (!m_creature) return false;

    // 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return m_avoidTarget != nullptr;
}

void AvoidEntityGoal::startExecuting() {
    if (m_creature) {
        m_creature->tryMoveTo(m_escapeX, m_escapeY, m_escapeZ, m_farSpeed);
    }
}

void AvoidEntityGoal::resetTask() {
    m_avoidTarget = nullptr;
    if (m_creature) {
        m_creature->clearNavigation();
    }
}

void AvoidEntityGoal::tick() {
    if (!m_creature || !m_avoidTarget) return;

    // MC 1.16.5: 根据距离调整速度，阈值是 49.0D (7*7)
    f32 distSq = m_creature->distanceSqTo(*m_avoidTarget);

    f64 speed;
    if (distSq < AVOID_NEAR_DISTANCE_SQ) {
        speed = m_nearSpeed;
    } else {
        speed = m_farSpeed;
    }

    // MC 1.16.5: 直接调用 navigator.setSpeed() 而不是重新计算路径
    auto* nav = m_creature->navigator();
    if (nav) {
        nav->setSpeed(speed);
    }
}

LivingEntity* AvoidEntityGoal::findEntityToAvoid() {
    if (!m_creature || !m_creature->world()) return nullptr;

    return EntityUtils::findClosestEntity<LivingEntity>(
        m_creature->world(),
        m_creature->position(),
        m_avoidDistance,
        m_creature,
        m_predicate
    );
}

bool AvoidEntityGoal::findEscapePosition() {
    if (!m_creature || !m_avoidTarget) return false;

    math::Random rng = m_creature->getRandom();

    // 生成远离目标的位置
    // 简化实现：直接计算逃跑位置
    f32 distance = static_cast<f32>(ESCAPE_RANGE);

    // 计算从目标指向相反方向的位置
    f32 dirX = m_creature->x() - m_avoidTarget->x();
    f32 dirZ = m_creature->z() - m_avoidTarget->z();
    f32 len = std::sqrt(dirX * dirX + dirZ * dirZ);

    if (len > 0.01f) {
        dirX /= len;
        dirZ /= len;
    }

    // 添加随机偏移
    f32 randomAngle = rng.nextFloat() * math::TWO_PI;
    f32 randomDist = distance * rng.nextFloat();

    m_escapeX = m_creature->x() + dirX * distance + std::cos(randomAngle) * randomDist * 0.5f;
    m_escapeZ = m_creature->z() + dirZ * distance + std::sin(randomAngle) * randomDist * 0.5f;
    m_escapeY = m_creature->y();

    return true;
}

} // namespace mc::entity::ai::goal
