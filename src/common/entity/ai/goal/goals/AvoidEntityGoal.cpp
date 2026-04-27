#include "AvoidEntityGoal.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../GoalConstants.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../util/RandomPositionGenerator.hpp"
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

    // MC 1.16.5: 寻找要避开的实体
    m_avoidTarget = findEntityToAvoid();
    if (!m_avoidTarget) {
        return false;
    }

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTargetBlockAwayFrom
    // 寻找远离目标的位置
    return findEscapePosition();
}

bool AvoidEntityGoal::shouldContinueExecuting() {
    if (!m_creature) return false;

    // MC 1.16.5: 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return true;
}

void AvoidEntityGoal::startExecuting() {
    if (m_creature) {
        // MC 1.16.5: 设置路径到逃跑位置
        if (auto* nav = m_creature->navigator()) {
            nav->moveTo(m_escapeX, m_escapeY, m_escapeZ, m_farSpeed);
        }
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
    f64 distSq = m_creature->distanceSqTo(*m_avoidTarget);

    if (auto* nav = m_creature->navigator()) {
        if (distSq < AVOID_NEAR_DISTANCE_SQ) {
            // 近距离使用近距速度（更快）
            nav->setSpeed(m_nearSpeed);
        } else {
            // 远距离使用远距速度
            nav->setSpeed(m_farSpeed);
        }
    }
}

LivingEntity* AvoidEntityGoal::findEntityToAvoid() {
    if (!m_creature || !m_creature->world()) return nullptr;

    // MC 1.16.5: 在避开距离内搜索，垂直扩展 3.0D
    // 使用 EntityPredicate 进行搜索
    f32 verticalExpand = 3.0f;

    return EntityUtils::findClosestEntity<LivingEntity>(
        m_creature->world(),
        m_creature->position(),
        m_avoidDistance + verticalExpand,  // 包含垂直扩展
        m_creature,
        m_predicate
    );
}

bool AvoidEntityGoal::findEscapePosition() {
    if (!m_creature || !m_avoidTarget) return false;

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTargetBlockAwayFrom(entity, 16, 7, avoidTarget.getPositionVec())
    Vector3 avoidPos(m_avoidTarget->x(), m_avoidTarget->y(), m_avoidTarget->z());
    Vector3 escapePos;

    if (util::RandomPositionGenerator::findRandomTargetBlockAwayFrom(
            m_creature,
            ESCAPE_HORIZONTAL_RANGE,  // 16
            ESCAPE_VERTICAL_RANGE,     // 7
            avoidPos,
            escapePos)) {
        m_escapeX = escapePos.x;
        m_escapeY = escapePos.y;
        m_escapeZ = escapePos.z;
        return true;
    }

    return false;
}

} // namespace mc::entity::ai::goal
