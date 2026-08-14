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
#include "WalkNodeProcessor.hpp"
#include "WorldRegion.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/controller/MovementController.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/MobEntity.hpp"
#include "util/TimeUtils.hpp"
#include "util/math/MathUtils.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

namespace mc::entity::ai::pathfinding {

using namespace goal::constants;
using namespace mc::util;
using namespace mc::math;

PathNavigator::PathNavigator(std::unique_ptr<PathFinder> finder)
    : m_pathFinder(std::move(finder))
{}

PathNavigator::PathNavigator(MobEntity* mob)
    : m_pathFinder(nullptr)
    , m_entity(mob)
{
    // PathFinder 需要后续设置或使用默认
}

bool PathNavigator::moveTo(f64 x, f64 y, f64 z, f64 speed)
{
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_speed = speed;

    if (!m_pathFinder || !m_entity) {
        return false;
    }

    // 重置卡住检测
    m_stuckTimer = 0;
    m_isStuck = false;
    m_lastPosX = m_entity->x();
    m_lastPosY = m_entity->y();
    m_lastPosZ = m_entity->z();

    // 计算路径
    i32 startX = floorTo<i32>(m_entity->x());
    i32 startY = floorTo<i32>(m_entity->y());
    i32 startZ = floorTo<i32>(m_entity->z());
    i32 targetXi = floorTo<i32>(x);
    i32 targetYi = floorTo<i32>(y);
    i32 targetZi = floorTo<i32>(z);

    // 寻路前注入世界区域：Region 是寻路器访问方块的适配层，历史全仓无具体实现，
    // m_region 恒 nullptr 致 getNodeType 恒 Blocked、findPath 返回空。每次寻路用实体
    // 当前 world 栈上构造 WorldRegion 并 setRegion（委托 IWorld）。对应 vanilla
    // PathNavigation 在路径计算前刷新区域缓存。
    // 注意：region 作用域必须覆盖整个 findPath 调用（NodeProcessor 持 m_region 裸指针），
    // 早期版本误把 region 声明在 if 块内，致 findPath 时 region 已析构、m_region 悬垂，
    // 虚调用 isWalkable 解引用已释放栈内存段错误。
    IWorld* world = m_entity->world();
    if (world == nullptr) {
        return false;
    }
    WorldRegion region(*world);
    m_pathFinder->setRegion(&region);

    m_path = std::make_unique<Path>(m_pathFinder->findPath(
        startX, startY, startZ, targetXi, targetYi, targetZi, m_maxDistance, m_maxVisitedNodesMultiplier));

    _trimPath();

    // 寻路结束后立即清空 region 指针：region 是本函数栈对象，返回后即析构。
    // NodeProcessor::m_region 是裸指针且 setRegion 不做所有权管理，若不清空将悬垂。
    // 后续 MovementController::canWalkAt(Strafe 分支)会调 NodeProcessor::getNodeType
    // 解引用 m_region，悬垂虚调用(isLoaded)会跳到已释放栈内存的虚表槽致 SEGV。
    // 此处置空后，getNodeType 内的 !m_region 守卫会返回 Blocked，canWalkAt 走兜底。
    m_pathFinder->setRegion(nullptr);

    return hasPath();
}

bool PathNavigator::moveTo(const Entity& target, f64 speed)
{
    return moveTo(target.x(), target.y(), target.z(), speed);
}

bool PathNavigator::moveToRange(f64 x, f64 y, f64 z, f32 range, f64 speed)
{
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_speed = speed;

    if (!m_pathFinder || !m_entity) {
        return false;
    }

    i32 startX = floorTo<i32>(m_entity->x());
    i32 startY = floorTo<i32>(m_entity->y());
    i32 startZ = floorTo<i32>(m_entity->z());
    i32 targetXi = floorTo<i32>(x);
    i32 targetYi = floorTo<i32>(y);
    i32 targetZi = floorTo<i32>(z);

    // 寻路前注入世界区域（同 moveTo，详见其注释；region 作用域须覆盖 findPathToRange）。
    IWorld* world = m_entity->world();
    if (world == nullptr) {
        return false;
    }
    WorldRegion region(*world);
    m_pathFinder->setRegion(&region);

    m_path = std::make_unique<Path>(m_pathFinder->findPathToRange(
        startX, startY, startZ, targetXi, targetYi, targetZi, static_cast<i32>(range), m_maxVisitedNodesMultiplier));

    _trimPath();

    // 寻路结束后清空 region 指针避免悬垂（详见 moveTo 末尾同名注释）。
    m_pathFinder->setRegion(nullptr);

    return hasPath();
}

i32 PathNavigator::getCurrentIndex() const
{
    return m_path ? m_path->getCurrentIndex() : -1;
}

bool PathNavigator::recomputePath()
{
    if (!m_path || m_retryTimer > 0) {
        return false;
    }

    m_retryTimer = m_retryInterval;
    return moveTo(m_targetX, m_targetY, m_targetZ, m_speed);
}

void PathNavigator::tick()
{
    if (!hasPath() || !m_entity) {
        return;
    }

    // 增加总计时
    ++m_ticksSinceLastPath;

    // 更新重试计时器
    if (m_retryTimer > 0) {
        --m_retryTimer;
    }

    // 沿路径移动
    _followPath();

    // 卡住检测
    _checkForStuck();

    // 检查是否需要重新计算
    if (_shouldRecomputePath()) {
        (void)recomputePath();
    }
}

void PathNavigator::_followPath()
{
    if (!m_path || m_path->empty() || !m_entity) {
        return;
    }

    const PathPoint* waypoint = _getCurrentWaypoint();
    if (!waypoint) {
        return;
    }

    // 检查是否到达当前路径点
    if (_isAtCurrentWaypoint()) {
        _advanceToNextWaypoint();
        waypoint = _getCurrentWaypoint();

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
            moveCtrl->setMoveTo(waypoint->x() + 0.5, waypoint->y(), waypoint->z() + 0.5, m_speed);
        }
    }
}

void PathNavigator::_checkForStuck()
{
    if (!m_entity || !hasPath()) {
        return;
    }

    // 检测卡住
    if (m_ticksSinceLastPath > 100) {
        f64 dx = m_entity->x() - m_lastPosX;
        f64 dy = m_entity->y() - m_lastPosY;
        f64 dz = m_entity->z() - m_lastPosZ;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < 2.25) {
            m_isStuck = true;
            clearPath();
            _resetTimeout();
            return;
        }
        m_isStuck = false;

        // 更新上次位置
        m_lastPosX = m_entity->x();
        m_lastPosY = m_entity->y();
        m_lastPosZ = m_entity->z();
        m_ticksSinceLastPath = 0;
    }

    // 超时检测
    if (m_path && !m_path->isFinished()) {
        const PathPoint* waypoint = _getCurrentWaypoint();
        if (waypoint) {
            i32 nodeX = waypoint->x();
            i32 nodeY = waypoint->y();
            i32 nodeZ = waypoint->z();

            if (nodeX == m_timeoutCachedNodeX && nodeY == m_timeoutCachedNodeY && nodeZ == m_timeoutCachedNodeZ) {
                // 同一个节点，累加时间
                i64 currentTime = TimeUtils::getCurrentTimeMs();
                m_timeoutTimer += currentTime - m_lastTimeoutCheck;
                m_lastTimeoutCheck = currentTime;
            } else {
                // 新节点，重置计时器
                m_timeoutCachedNodeX = nodeX;
                m_timeoutCachedNodeY = nodeY;
                m_timeoutCachedNodeZ = nodeZ;

                // 计算超时限制
                f64 dx = m_entity->x() - (nodeX + 0.5);
                f64 dz = m_entity->z() - (nodeZ + 0.5);
                f64 dist = std::sqrt(dx * dx + dz * dz);

                f32 moveSpeed = 0.0f;
                if (auto* mob = dynamic_cast<MobEntity*>(m_entity)) {
                    if (auto* moveCtrl = mob->moveController()) {
                        moveSpeed = static_cast<f32>(moveCtrl->speed());
                    }
                }
                m_timeoutLimit = moveSpeed > 0.0f ? dist / moveSpeed * 1000.0 : 0.0;
                m_timeoutTimer = 0;
                m_lastTimeoutCheck = TimeUtils::getCurrentTimeMs();
            }

            // 如果超时超过限制的3倍，清除路径
            if (m_timeoutLimit > 0.0 && static_cast<f64>(m_timeoutTimer) > m_timeoutLimit * 3.0) {
                _resetTimeout();
                clearPath();
            }
        }
    }
}

void PathNavigator::_trimPath()
{
    // 处理炼药锅等特殊方块的路径
    // 当实体在炼药锅中时会调整路径点
    if (!m_path || m_path->empty() || !m_entity) {
        return;
    }

    // 获取世界
    IWorld* world = m_entity->world();
    if (world == nullptr) {
        return;
    }

    // 遍历路径中的每个点
    for (size_t i = 0; i < m_path->length(); ++i) {
        const PathPoint* point = m_path->getPoint(i);
        if (point == nullptr) {
            continue;
        }

        // 获取下一个路径点（如果存在）
        const PathPoint* nextPoint = m_path->getPoint(i + 1);

        // 获取当前位置的方块状态
        BlockPos pos(point->x(), point->y(), point->z());
        const BlockState* state = world->getBlockState(pos);

        // 检查是否为炼药锅（空炼药锅、水炼药锅、岩浆炼药锅均需路径上移）
        if (state != nullptr &&
            (state->is(VanillaBlocks::CAULDRON) || state->is(VanillaBlocks::WATER_CAULDRON) ||
                state->is(VanillaBlocks::LAVA_CAULDRON))) {
            // 炼药锅是一个凹陷的方块，实体在里面时需要将路径点上移
            PathPoint newPoint = point->cloneMove(point->x(), point->y() + 1, point->z());
            m_path->setPoint(i, newPoint);

            // 如果下一个路径点的 Y 坐标不高于当前点（修正后的坐标），
            // 也需要将下一个点上移
            if (nextPoint != nullptr && nextPoint->y() <= point->y()) {
                PathPoint newNextPoint = nextPoint->cloneMove(nextPoint->x(), point->y() + 1, nextPoint->z());
                m_path->setPoint(i + 1, newNextPoint);
            }
        }
    }

    // 阳光避让路径截断
    // 当 m_avoidSun 为 true 时，遍历路径节点，如果在阴影中的实体
    // 发现路径经过阳光直射区域，则在该节点处截断路径。
    // 这对应 MC Java 版的 GroundPathNavigation.trimPath() 逻辑。
    if (m_avoidSun) {
        // 如果实体当前已在阳光下，保留完整路径——实体需要移动来逃离阳光
        if (world->canSeeSky(BlockPos(
                floorTo<i32>(m_entity->x()), floorTo<i32>(m_entity->y() + 0.5), floorTo<i32>(m_entity->z())))) {
            return;
        }

        // 遍历路径节点，找到第一个暴露在阳光下的节点并截断
        for (i32 i = 0; i < static_cast<i32>(m_path->length()); ++i) {
            const PathPoint* point = m_path->getPoint(static_cast<size_t>(i));
            if (point == nullptr) {
                continue;
            }

            if (world->canSeeSky(BlockPos(point->x(), point->y(), point->z()))) {
                // 在第一个阳光暴露节点处截断路径
                m_path->truncateNodes(i);
                break;
            }
        }
    }
}

void PathNavigator::_resetTimeout()
{
    m_timeoutCachedNodeX = 0;
    m_timeoutCachedNodeY = 0;
    m_timeoutCachedNodeZ = 0;
    m_timeoutTimer = 0;
    m_lastTimeoutCheck = 0;
    m_timeoutLimit = 0.0;
    m_isStuck = false;
}

bool PathNavigator::_shouldRecomputePath() const
{
    if (!m_path || m_path->empty()) {
        return false;
    }

    // 检查目标位置是否变化太多
    if (m_path->getEnd()) {
        // 使用 distanceToSq(x, y, z) 重载避免创建临时 PathPoint 对象
        f32 distSq = m_path->getEnd()->distanceToSq(
            static_cast<i32>(m_targetX), static_cast<i32>(m_targetY), static_cast<i32>(m_targetZ));
        if (distSq > 16.0f) { // 目标移动超过4格
            return true;
        }
    }

    return false;
}

bool PathNavigator::_isAtCurrentWaypoint() const
{
    const PathPoint* waypoint = _getCurrentWaypoint();
    if (!waypoint || !m_entity) {
        return true;
    }

    // 检查水平和垂直距离
    f64 dx = waypoint->x() + 0.5 - m_entity->x();
    f64 dz = waypoint->z() + 0.5 - m_entity->z();
    f64 distSq = dx * dx + dz * dz;

    // maxDistanceToWaypoint 根据实体宽度调整
    f32 width = m_entity->width();
    f32 maxDist = width > 0.75f ? width / 2.0f : 0.75f - width / 2.0f;

    // 检查水平距离和垂直距离
    f64 dy = std::abs(m_entity->y() - waypoint->y());

    return distSq < static_cast<f64>(maxDist * maxDist) && dy < 1.0;
}

void PathNavigator::_advanceToNextWaypoint()
{
    if (m_path) {
        m_path->advance();
    }
}

f32 PathNavigator::_getDistanceToTarget() const
{
    if (!m_entity) {
        return std::numeric_limits<f32>::max();
    }

    f64 dx = m_targetX - m_entity->x();
    f64 dy = m_targetY - m_entity->y();
    f64 dz = m_targetZ - m_entity->z();

    return static_cast<f32>(std::sqrt(dx * dx + dy * dy + dz * dz));
}

const PathPoint* PathNavigator::_getCurrentWaypoint() const
{
    return m_path ? m_path->getCurrentTarget() : nullptr;
}

void PathNavigator::setAvoidSunPathing(bool avoidSun)
{
    m_avoidSun = avoidSun;

    if (m_pathFinder) {
        auto* nodeProcessor = m_pathFinder->getNodeProcessor();
        if (nodeProcessor != nullptr) {
            auto* walkNodeProcessor = dynamic_cast<WalkNodeProcessor*>(nodeProcessor);
            if (walkNodeProcessor != nullptr) {
                walkNodeProcessor->setAvoidSun(avoidSun);
            }
        }
    }
}

void PathNavigator::setCanOpenDoors(bool canOpenDoors)
{
    m_canOpenDoors = canOpenDoors;

    if (m_pathFinder) {
        auto* nodeProcessor = m_pathFinder->getNodeProcessor();
        if (nodeProcessor != nullptr) {
            auto* walkNodeProcessor = dynamic_cast<WalkNodeProcessor*>(nodeProcessor);
            if (walkNodeProcessor != nullptr) {
                walkNodeProcessor->setCanOpenDoors(canOpenDoors);
            }
        }
    }
}

void PathNavigator::setCanEnterDoors(bool canEnterDoors)
{
    m_canEnterDoors = canEnterDoors;

    if (m_pathFinder) {
        auto* nodeProcessor = m_pathFinder->getNodeProcessor();
        if (nodeProcessor != nullptr) {
            auto* walkNodeProcessor = dynamic_cast<WalkNodeProcessor*>(nodeProcessor);
            if (walkNodeProcessor != nullptr) {
                walkNodeProcessor->setCanEnterDoors(canEnterDoors);
            }
        }
    }
}

} // namespace mc::entity::ai::pathfinding
