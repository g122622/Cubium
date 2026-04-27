#include "PathNavigator.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "../controller/MovementController.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

using namespace goal::constants;

PathNavigator::PathNavigator(std::unique_ptr<PathFinder> finder)
    : m_pathFinder(std::move(finder))
{
}

PathNavigator::PathNavigator(MobEntity* mob)
    : m_pathFinder(nullptr)
    , m_entity(mob)
{
    // PathFinder 需要后续设置或使用默认
}

bool PathNavigator::moveTo(f64 x, f64 y, f64 z, f64 speed) {
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_speed = speed;

    if (!m_pathFinder || !m_entity) {
        return false;
    }

    // MC 1.16.5: 重置卡住检测
    m_stuckTimer = 0;
    if (m_entity) {
        m_lastPosX = m_entity->x();
        m_lastPosY = m_entity->y();
        m_lastPosZ = m_entity->z();
    }

    // 计算路径
    i32 startX = static_cast<i32>(std::floor(m_entity->x()));
    i32 startY = static_cast<i32>(std::floor(m_entity->y()));
    i32 startZ = static_cast<i32>(std::floor(m_entity->z()));
    i32 targetXi = static_cast<i32>(std::floor(x));
    i32 targetYi = static_cast<i32>(std::floor(y));
    i32 targetZi = static_cast<i32>(std::floor(z));

    m_path = std::make_unique<Path>(m_pathFinder->findPath(
        startX, startY, startZ,
        targetXi, targetYi, targetZi,
        m_maxDistance
    ));

    return hasPath();
}

bool PathNavigator::moveTo(const Entity& target, f64 speed) {
    return moveTo(target.x(), target.y(), target.z(), speed);
}

bool PathNavigator::moveToRange(f64 x, f64 y, f64 z, f32 range, f64 speed) {
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_speed = speed;

    if (!m_pathFinder || !m_entity) {
        return false;
    }

    i32 startX = static_cast<i32>(std::floor(m_entity->x()));
    i32 startY = static_cast<i32>(std::floor(m_entity->y()));
    i32 startZ = static_cast<i32>(std::floor(m_entity->z()));
    i32 targetXi = static_cast<i32>(std::floor(x));
    i32 targetYi = static_cast<i32>(std::floor(y));
    i32 targetZi = static_cast<i32>(std::floor(z));

    m_path = std::make_unique<Path>(m_pathFinder->findPathToRange(
        startX, startY, startZ,
        targetXi, targetYi, targetZi,
        static_cast<i32>(range)
    ));

    return hasPath();
}

i32 PathNavigator::getCurrentIndex() const {
    return m_path ? m_path->getCurrentIndex() : -1;
}

bool PathNavigator::recomputePath() {
    if (!m_path || m_retryTimer > 0) {
        return false;
    }

    m_retryTimer = m_retryInterval;
    return moveTo(m_targetX, m_targetY, m_targetZ, m_speed);
}

void PathNavigator::tick() {
    if (!hasPath() || !m_entity) {
        return;
    }

    // 更新重试计时器
    if (m_retryTimer > 0) {
        --m_retryTimer;
    }

    // MC 1.16.5: 卡住检测 - 检查是否移动
    f64 dx = m_entity->x() - m_lastPosX;
    f64 dy = m_entity->y() - m_lastPosY;
    f64 dz = m_entity->z() - m_lastPosZ;
    f64 distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < PATH_STUCK_DISTANCE_THRESHOLD) {
        ++m_stuckTimer;
        if (m_stuckTimer >= PATH_STUCK_THRESHOLD) {
            // MC 1.16.5: 卡住超过阈值，清除路径
            clearPath();
            m_stuckTimer = 0;
            return;
        }
    } else {
        m_stuckTimer = 0;
    }

    // 更新上次位置
    m_lastPosX = m_entity->x();
    m_lastPosY = m_entity->y();
    m_lastPosZ = m_entity->z();

    // 沿路径移动
    followPath();

    // 检查是否需要重新计算
    if (shouldRecomputePath()) {
        (void)recomputePath();
    }

    // 增加计时器
    ++m_ticksSinceLastPath;
}

void PathNavigator::followPath() {
    if (!m_path || m_path->empty() || !m_entity) {
        return;
    }

    const PathPoint* waypoint = getCurrentWaypoint();
    if (!waypoint) {
        return;
    }

    // 检查是否到达当前路径点
    if (isAtCurrentWaypoint()) {
        advanceToNextWaypoint();
        waypoint = getCurrentWaypoint();

        if (!waypoint) {
            // 路径完成
            clearPath();
            return;
        }
    }

    // 移动向目标路径点
    // 通过 MovementController 控制移动
    // 需要将 LivingEntity 转换为 MobEntity 来访问 moveController
    if (auto* mob = dynamic_cast<MobEntity*>(m_entity)) {
        if (auto* moveCtrl = mob->moveController()) {
            moveCtrl->setMoveTo(
                waypoint->x() + 0.5,
                waypoint->y(),
                waypoint->z() + 0.5,
                m_speed
            );
        }
    }
}

bool PathNavigator::shouldRecomputePath() const {
    if (!m_path || m_path->empty()) {
        return false;
    }

    // 检查目标位置是否变化太多
    if (m_path->getEnd()) {
        f32 distSq = m_path->getEnd()->distanceToSq(
            PathPoint(
                static_cast<i32>(m_targetX),
                static_cast<i32>(m_targetY),
                static_cast<i32>(m_targetZ)
            )
        );
        if (distSq > 16.0f) { // 目标移动超过4格
            return true;
        }
    }

    return false;
}

bool PathNavigator::isAtCurrentWaypoint() const {
    const PathPoint* waypoint = getCurrentWaypoint();
    if (!waypoint || !m_entity) {
        return true;
    }

    // 检查水平距离是否足够近
    f64 dx = waypoint->x() + 0.5 - m_entity->x();
    f64 dz = waypoint->z() + 0.5 - m_entity->z();
    f64 distSq = dx * dx + dz * dz;

    // 到达阈值（约0.7格，允许一些误差）
    constexpr f64 threshold = 0.5;
    return distSq < threshold * threshold;
}

void PathNavigator::advanceToNextWaypoint() {
    if (m_path) {
        m_path->advance();
    }
}

f32 PathNavigator::getDistanceToTarget() const {
    if (!m_entity) {
        return std::numeric_limits<f32>::max();
    }

    f64 dx = m_targetX - m_entity->x();
    f64 dy = m_targetY - m_entity->y();
    f64 dz = m_targetZ - m_entity->z();

    return static_cast<f32>(std::sqrt(dx * dx + dy * dy + dz * dz));
}

const PathPoint* PathNavigator::getCurrentWaypoint() const {
    return m_path ? m_path->getCurrentTarget() : nullptr;
}

} // namespace mc::entity::ai::pathfinding
