/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PathNavigator.hpp"
#include "WalkNodeEvaluator.hpp"
#include "WorldRegion.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace mc::entity::ai::pathfinding {

namespace {

/// 建路径时世界区域的水平半径（原版在 `createPath` 里为地面导航硬编码 8）。
constexpr i32 GROUND_SEARCH_RANGE = 8;

/// 走到实体时的到达半径（原版 `EntityPathNavigation.createPath(Entity, int)` 传 16）。
constexpr i32 ENTITY_REACH_RANGE = 16;

/// 走不到目标时「尽力而为」路径的终点离目标至少要小于这个距离才算有效
constexpr f64 MIN_PROGRESS = 1.0e-5;

} // namespace

PathNavigator::PathNavigator(MobEntity* mob, std::unique_ptr<PathFinder> pathFinder)
    : m_mob(mob)
    , m_pathFinder(std::move(pathFinder))
{
    MC_ASSERT_RELEASE(mob != nullptr);
    MC_ASSERT_RELEASE(m_pathFinder != nullptr);
    // 原版在构造时按 `FOLLOW_RANGE * 16` 初始化访问节点上限，并在跟随范围变化时更新。
    m_pathFinder->setMaxVisitedNodes(static_cast<i32>(std::floor(16.0f * 16.0f)));
}

NodeEvaluator* PathNavigator::nodeEvaluator() noexcept
{
    return &m_pathFinder->evaluator();
}

// ===========================================================================
// 配置
// ===========================================================================

void PathNavigator::setRequiredPathLength(f32 length) noexcept
{
    m_requiredPathLength = length;
}

void PathNavigator::setCanFloat(bool canFloat)
{
    nodeEvaluator()->setCanFloat(canFloat);
}

bool PathNavigator::canFloat() const
{
    return const_cast<PathNavigator*>(this)->nodeEvaluator()->canFloat();
}

void PathNavigator::setCanPassDoors(bool canPassDoors)
{
    nodeEvaluator()->setCanPassDoors(canPassDoors);
}

bool PathNavigator::canPassDoors() const
{
    return const_cast<PathNavigator*>(this)->nodeEvaluator()->canPassDoors();
}

void PathNavigator::setCanOpenDoors(bool canOpenDoors)
{
    nodeEvaluator()->setCanOpenDoors(canOpenDoors);
}

bool PathNavigator::canOpenDoors() const
{
    return const_cast<PathNavigator*>(this)->nodeEvaluator()->canOpenDoors();
}

void PathNavigator::setCanWalkOverFences(bool canWalkOverFences)
{
    nodeEvaluator()->setCanWalkOverFences(canWalkOverFences);
}

// ===========================================================================
// 建路径
// ===========================================================================

std::optional<Path> PathNavigator::_createPath(const BlockPos& target, i32 reachRange, f32 maxPathLength)
{
    return _createPath(std::vector<BlockPos>{target}, GROUND_SEARCH_RANGE, false, reachRange, maxPathLength);
}

std::optional<Path> PathNavigator::_createPath(const std::vector<BlockPos>& targets,
    i32 searchRange,
    bool above,
    i32 reachRange,
    f32 maxPathLength)
{
    if (targets.empty()) {
        return std::nullopt;
    }

    const IWorld* world = m_mob->world();
    if (world == nullptr) {
        return std::nullopt;
    }
    // 目标集合里已经有当前目标、且路径还没走完：沿用旧路径，不做重复搜索。
    if (m_path.has_value() && !m_path->isDone() && m_targetPos.has_value()) {
        const bool stillTargeted =
            std::any_of(targets.begin(), targets.end(), [this](const BlockPos& pos) { return pos == *m_targetPos; });
        if (stillTargeted) {
            return m_path;
        }
    }

    MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Game.Entity, "PathNavigator::createPath");

    // 区域只需要覆盖实体周围：求值器的查询范围由行走距离上限与搜索半径共同决定，
    // 而世界读取本身不按区域裁剪，因此这里不再手工圈定半径。
    // 目标若远在区域之外，A* 仍会沿「行走距离」边界把搜索停住并返回「尽力而为」的路径。
    MC_UNUSED(searchRange);
    BlockPos origin(static_cast<i32>(std::floor(m_mob->x())),
        static_cast<i32>(std::floor(m_mob->y())),
        static_cast<i32>(std::floor(m_mob->z())));
    if (above) {
        origin = BlockPos(origin.x, origin.y + 1, origin.z);
    }
    WorldRegion region(*world);

    auto path = m_pathFinder->findPath(region,
        *m_mob,
        Vector3d(static_cast<f64>(m_mob->x()), static_cast<f64>(m_mob->y()), static_cast<f64>(m_mob->z())),
        targets,
        maxPathLength,
        reachRange,
        m_maxVisitedNodesMultiplier);
    if (path.has_value()) {
        m_targetPos = path->target();
        m_reachRange = reachRange;
        _resetStuckTimeout();
    }
    return path;
}

bool PathNavigator::moveTo(f64 x, f64 y, f64 z, i32 reachRange, f64 speed)
{
    return moveTo(_createPath(BlockPos(static_cast<i32>(std::floor(x)), static_cast<i32>(std::floor(y)), static_cast<i32>(std::floor(z))), reachRange, 16.0f), speed);
}

bool PathNavigator::moveTo(f64 x, f64 y, f64 z, f64 speed)
{
    return moveTo(x, y, z, 1, speed);
}

bool PathNavigator::moveToRange(f64 x, f64 y, f64 z, i32 reachRange, f64 speed)
{
    return moveTo(x, y, z, reachRange, speed);
}

bool PathNavigator::moveTo(const LivingEntity& target, f64 speed)
{
    const BlockPos pos(static_cast<i32>(std::floor(target.x())),
        static_cast<i32>(std::floor(target.y())),
        static_cast<i32>(std::floor(target.z())));
    return moveTo(_createPath(pos, ENTITY_REACH_RANGE, 16.0f), speed);
}

bool PathNavigator::moveTo(std::optional<Path> path, f64 speed)
{
    if (!path.has_value()) {
        m_path.reset();
        return false;
    }
    if (!path->sameAs(m_path.has_value() ? &*m_path : nullptr)) {
        m_path = std::move(path);
    }
    if (isDone()) {
        return false;
    }
    _trimPath();
    if (m_path->nodeCount() == 0) {
        return false;
    }
    m_speed = speed;
    m_lastStuckCheck = static_cast<i32>(m_tick);
    m_lastStuckCheckPos = _getTempMobPosition();
    return true;
}

void PathNavigator::recomputePath()
{
    if (m_tick - m_timeLastRecompute > MAX_TIME_RECOMPUTE) {
        if (m_targetPos.has_value()) {
            m_path.reset();
            m_path = _createPath(*m_targetPos, m_reachRange, m_requiredPathLength);
            m_timeLastRecompute = m_tick;
            m_hasDelayedRecomputation = false;
        }
        return;
    }
    // 刚算过就又要重算：留到下一个 tick，避免每 tick 都跑一次 A*。
    m_hasDelayedRecomputation = true;
}

// ===========================================================================
// 跟随
// ===========================================================================

void PathNavigator::tick()
{
    ++m_tick;
    if (m_hasDelayedRecomputation) {
        recomputePath();
    }

    if (isDone()) {
        return;
    }

    if (_canUpdatePath()) {
        _followThePath();
    } else if (m_path.has_value() && !m_path->isDone()) {
        // 无法跟随（例如正在下落）时，如果实体已经在下一路径点的正上方，就直接跳过它，
        // 免得落地后回头再走一遍。
        const Vector3d position = _getTempMobPosition();
        const Vector3d next = m_path->nextEntityPos(m_mob->width());
        if (position.y > next.y && m_mob->onGround() && std::floor(position.x) == std::floor(next.x) &&
            std::floor(position.z) == std::floor(next.z)) {
            m_path->advance();
        }
    }

    if (isDone()) {
        return;
    }
    const Vector3d next = m_path->nextEntityPos(m_mob->width());
    m_mob->moveController()->setMoveTo(next.x, _getGroundY(next), next.z, m_speed);
}

void PathNavigator::_followThePath()
{
    const Vector3d position = _getTempMobPosition();
    // 到达当前路径点的判定半径随实体宽度变化：宽实体能有更宽的「到达」窗口。
    m_maxDistanceToWaypoint =
        m_mob->width() > 0.75f ? m_mob->width() / 2.0f : 0.75f - m_mob->width() / 2.0f;

    const BlockPos nextPos = m_path->nextNodePos();
    const f64 dx = std::abs(m_mob->x() - (static_cast<f64>(nextPos.x) + 0.5));
    const f64 dy = std::abs(m_mob->y() - static_cast<f64>(nextPos.y));
    const f64 dz = std::abs(m_mob->z() - (static_cast<f64>(nextPos.z) + 0.5));
    const bool arrived = dx < m_maxDistanceToWaypoint && dz < m_maxDistanceToWaypoint && dy < 1.0;

    if (arrived || (canCutCorner(m_path->nextNode().type()) && _shouldTargetNextNodeInDirection(position))) {
        m_path->advance();
    }

    _doStuckDetection(position);
}

bool PathNavigator::canCutCorner(PathType type) const
{
    // 危险地形与「需要开门的门格」不允许抄近路：抄近路意味着实体不等走到格子中心就拐弯。
    return type != PathType::DangerFire && type != PathType::DangerOther && type != PathType::WalkableDoor;
}

bool PathNavigator::_shouldTargetNextNodeInDirection(const Vector3d& position) const
{
    if (m_path->nextNodeIndex() + 1 >= static_cast<i32>(m_path->nodeCount())) {
        return false;
    }
    const BlockPos next = m_path->nextNodePos();
    const Vector3d nextCentre(static_cast<f64>(next.x) + 0.5, static_cast<f64>(next.y), static_cast<f64>(next.z) + 0.5);
    if (position.distanceSquared(nextCentre) >= 4.0) {
        return false;
    }
    if (_canMoveDirectly(position, m_path->nextEntityPos(m_mob->width()))) {
        return true;
    }

    // 不能直走时，比较「朝当前路径点」和「朝再下一个路径点」两个方向的夹角：
    // 如果再下一个路径点其实更靠前（夹角超过 90°），说明当前路径点已经绕远，直接跳过。
    const BlockPos afterNext = m_path->nodePos(static_cast<size_t>(m_path->nextNodeIndex() + 1));
    const Vector3d afterCentre(
        static_cast<f64>(afterNext.x) + 0.5, static_cast<f64>(afterNext.y), static_cast<f64>(afterNext.z) + 0.5);
    const Vector3d toNext = nextCentre - position;
    const Vector3d toAfter = afterCentre - position;
    const f64 nextLengthSqr = toNext.lengthSquared();
    const f64 afterLengthSqr = toAfter.lengthSquared();
    const bool afterIsCloser = afterLengthSqr < nextLengthSqr;
    const bool nextIsNearby = nextLengthSqr < 0.5;
    if (!afterIsCloser && !nextIsNearby) {
        return false;
    }
    return toAfter.normalized().dot(toNext.normalized()) < 0.0;
}

void PathNavigator::_doStuckDetection(const Vector3d& position)
{
    const i32 tick = static_cast<i32>(m_tick);
    if (tick - m_lastStuckCheck > STUCK_CHECK_INTERVAL) {
        const f32 speed = m_mob->aiMoveSpeed();
        // 移动慢的实体平方衰减：慢速生物在 100 tick 里本来就走不了多远。
        const f32 effective = speed >= 1.0f ? speed : speed * speed;
        const f32 threshold = effective * static_cast<f32>(STUCK_CHECK_INTERVAL) * STUCK_THRESHOLD_DISTANCE_FACTOR;
        if (position.distanceSquared(m_lastStuckCheckPos) < static_cast<f64>(threshold) * threshold) {
            m_isStuck = true;
            stop();
        } else {
            m_isStuck = false;
        }
        m_lastStuckCheck = tick;
        m_lastStuckCheckPos = position;
    }

    if (m_path.has_value() && !m_path->isDone()) {
        const BlockPos next = m_path->nextNodePos();
        if (next == m_timeoutCachedNode) {
            m_timeoutTimer += m_tick - m_lastTimeoutCheck;
        } else {
            m_timeoutCachedNode = next;
            // 预计耗时：按当前速度走到该路径点需要多少 tick。
            const Vector3d centre(
                static_cast<f64>(next.x) + 0.5, static_cast<f64>(next.y), static_cast<f64>(next.z) + 0.5);
            const f64 speed = m_mob->aiMoveSpeed();
            m_timeoutLimit =
                speed > 0.0 ? position.distance(centre) / speed * static_cast<f64>(20.0) : 0.0;
        }

        // 在同一个路径点上徘徊超过预计耗时的 3 倍：判定为走不到，放弃当前路径。
        if (m_timeoutLimit > 0.0 && static_cast<f64>(m_timeoutTimer) > m_timeoutLimit * 3.0) {
            _resetStuckTimeout();
            stop();
        }
        m_lastTimeoutCheck = m_tick;
    }
}

void PathNavigator::_resetStuckTimeout()
{
    m_timeoutCachedNode = BlockPos(0, 0, 0);
    m_timeoutTimer = 0;
    m_timeoutLimit = 0.0;
    m_isStuck = false;
}

// ===========================================================================
// 位置与地面高度
// ===========================================================================

bool PathNavigator::_canUpdatePath() const
{
    return m_mob->onGround() || m_mob->isInWater() || m_mob->hasPassengers();
}

Vector3d PathNavigator::_getTempMobPosition() const
{
    return Vector3d(m_mob->x(), _getGroundY(Vector3d(m_mob->x(), m_mob->y(), m_mob->z())), m_mob->z());
}

f64 PathNavigator::_getGroundY(const Vector3d& position) const
{
    const IWorld* world = m_mob->world();
    MC_ASSERT_RELEASE(world != nullptr);
    WorldRegion region(*world);
    return groundYAt(region, position);
}

f64 PathNavigator::groundYAt(const Region& region, const Vector3d& position)
{
    const i32 x = static_cast<i32>(std::floor(position.x));
    const i32 y = static_cast<i32>(std::floor(position.y));
    const i32 z = static_cast<i32>(std::floor(position.z));
    if (region.getBlockState(x, y - 1, z)->isAir()) {
        return position.y;
    }
    return WalkNodeEvaluator::getFloorLevel(region, x, y, z);
}

void PathNavigator::_trimPath()
{
    if (!m_path.has_value()) {
        return;
    }
    const IWorld* world = m_mob->world();
    MC_ASSERT_RELEASE(world != nullptr);
    WorldRegion region(*world);
    // 锅（炼药锅 / 细雪锅）的碰撞形状不满足寻路「可站立」判定，路径点会落在锅里；
    // 把落在锅里的路径点整体抬高一层，实体才能走过去。
    for (size_t index = 0; index < m_path->nodeCount(); ++index) {
        const PathPoint& node = m_path->node(index);
        const PathPoint* after = index + 1 < m_path->nodeCount() ? &m_path->node(index + 1) : nullptr;
        if (!BlockTags::CAULDRONS().contains(&region.getBlockState(node.x(), node.y(), node.z())->getBlock())) {
            continue;
        }
        PathPoint raised(node.x(), node.y() + 1, node.z());
        m_path->replaceNode(index, raised);
        if (after != nullptr && node.y() >= after->y()) {
            m_path->replaceNode(index + 1, PathPoint(after->x(), node.y() + 1, after->z()));
        }
    }
}

void PathNavigator::_trimPathSun()
{
    if (!m_avoidSun || !m_path.has_value()) {
        return;
    }
    const IWorld* world = m_mob->world();
    MC_ASSERT_RELEASE(world != nullptr);
    // 实体自己已经暴露在阳光下：不截断，否则它会连逃离的路径都没有。
    const i32 mobX = static_cast<i32>(std::floor(m_mob->x()));
    const i32 mobZ = static_cast<i32>(std::floor(m_mob->z()));
    if (world->canSeeSky(BlockPos(mobX, static_cast<i32>(std::floor(m_mob->y() + 0.5)), mobZ))) {
        return;
    }
    for (size_t index = 0; index < m_path->nodeCount(); ++index) {
        const PathPoint& node = m_path->node(index);
        if (world->canSeeSky(BlockPos(node.x(), node.y(), node.z()))) {
            m_path->truncateNodes(static_cast<i32>(index));
            return;
        }
    }
}

bool PathNavigator::shouldRecomputePath(const BlockPos& pos) const
{
    if (m_hasDelayedRecomputation) {
        return false;
    }
    if (!m_path.has_value() || m_path->isDone() || m_path->nodeCount() == 0) {
        return false;
    }
    const PathPoint* end = m_path->endNode();
    const Vector3d centre((static_cast<f64>(end->x()) + m_mob->x()) / 2.0,
        (static_cast<f64>(end->y()) + m_mob->y()) / 2.0,
        (static_cast<f64>(end->z()) + m_mob->z()) / 2.0);
    const f64 blockCentreX = static_cast<f64>(pos.x) + 0.5;
    const f64 blockCentreY = static_cast<f64>(pos.y) + 0.5;
    const f64 blockCentreZ = static_cast<f64>(pos.z) + 0.5;
    const f64 dx = blockCentreX - centre.x;
    const f64 dy = blockCentreY - centre.y;
    const f64 dz = blockCentreZ - centre.z;
    const f64 allowed = static_cast<f64>(m_path->nodeCount() - m_path->nextNodeIndex());
    return dx * dx + dy * dy + dz * dz < allowed * allowed;
}

bool PathNavigator::isStableDestination(const BlockPos& pos) const
{
    const IWorld* world = m_mob->world();
    MC_ASSERT_RELEASE(world != nullptr);
    return world->getBlockState(pos.x, pos.y - 1, pos.z)->isSolidRender();
}

bool PathNavigator::_canMoveDirectly(const Vector3d& from, const Vector3d& to) const
{
    // 地面实体不能「直线穿越」：必须沿路径点走，所以默认返回 false。
    MC_UNUSED(from);
    MC_UNUSED(to);
    return false;
}

} // namespace mc::entity::ai::pathfinding