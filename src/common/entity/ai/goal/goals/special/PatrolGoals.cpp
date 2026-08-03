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

#include "PatrolGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/PatrollerEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <vector>

namespace mc::entity::ai::goal {

using namespace mc::math;
using PathNavigator = mc::entity::ai::pathfinding::PathNavigator;

PatrolGoal::PatrolGoal(PatrollerEntity* patroller, f64 memberSpeed, f64 leaderSpeed)
    : m_patroller(patroller)
    , m_memberSpeed(memberSpeed)
    , m_leaderSpeed(leaderSpeed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool PatrolGoal::shouldExecute()
{
    if (!m_patroller) return false;

    // 必须正在巡逻
    if (!m_patroller->isPatrolling()) return false;

    // 没有攻击目标
    LivingEntity* attackTarget = m_patroller->attackTarget();
    if (attackTarget != nullptr) return false;

    // 没有被骑乘
    if (m_patroller->isBeingRidden()) return false;

    // 有巡逻目标
    if (!m_patroller->hasPatrolTarget()) return false;

    // 不在冷却期
    IWorld* world = m_patroller->world();
    if (!world) return false;

    u64 gameTime = world->getGameTime();
    if (static_cast<i64>(gameTime) < m_cooldownTime) return false;

    return true;
}

bool PatrolGoal::shouldContinueExecuting()
{
    if (!m_patroller) return false;

    // 继续检查基本条件
    if (!m_patroller->isPatrolling()) return false;
    if (m_patroller->attackTarget() != nullptr) return false;
    if (m_patroller->isBeingRidden()) return false;

    // 检查导航器状态
    PathNavigator* nav = m_patroller->navigator();
    if (!nav) return false;

    // 有路径就继续
    return !nav->noPath();
}

void PatrolGoal::startExecuting()
{
    // 导航在 tick() 中处理
}

void PatrolGoal::tick()
{
    if (!m_patroller) return;

    PathNavigator* nav = m_patroller->navigator();
    if (!nav) return;

    IWorld* world = m_patroller->world();
    if (!world) return;

    // 只有在没有路径时才处理
    if (nav->noPath()) {
        // 获取附近巡逻队员
        std::vector<PatrollerEntity*> nearbyPatrollers = _getNearbyPatrollers();

        // 情况 1: 正在巡逻但没有队员 -> 停止巡逻
        if (m_patroller->isPatrolling() && nearbyPatrollers.empty()) {
            m_patroller->setPatrolling(false);
            return;
        }

        // 情况 2: 队长已到达巡逻目标 -> 重置新目标
        bool isLeader = m_patroller->isLeader();
        const BlockPos& patrolTarget = m_patroller->getPatrolTarget();

        // 计算到目标的距离（水平距离）
        f64 dx = static_cast<f64>(patrolTarget.x) + 0.5 - m_patroller->x();
        f64 dz = static_cast<f64>(patrolTarget.z) + 0.5 - m_patroller->z();
        f64 distSqXZ = dx * dx + dz * dz;

        if (isLeader && distSqXZ < ARRIVAL_THRESHOLD_SQ) {
            // 队长到达目标后重置巡逻目标
            m_patroller->resetPatrolTarget();
        } else {
            // 情况 3: 正常移动逻辑
            // 计算移动目标位置，使用巡逻目标中心点
            Vector3 targetCenter(static_cast<f32>(patrolTarget.x) + 0.5f,
                static_cast<f32>(patrolTarget.y) + 0.5f,
                static_cast<f32>(patrolTarget.z) + 0.5f);

            Vector3 currentPos = m_patroller->position();

            // 计算移动向量：向目标方向前进，但带有横向偏移
            // 这样可以让巡逻实体不完全走直线
            Vector3 toTarget = targetCenter - currentPos;
            Vector3 rotated = Vector3(-toTarget.z * 0.4f, // 旋转 90 度
                toTarget.y,
                toTarget.x * 0.4f);
            Vector3 offset = rotated + currentPos;
            Vector3 moveDirection = (offset - currentPos).normalized() * 10.0f;
            Vector3 moveTarget = currentPos + moveDirection;

            // 找到地面高度
            BlockPos targetBlockPos(floorTo<i32>(moveTarget.x), floorTo<i32>(moveTarget.y), floorTo<i32>(moveTarget.z));

            // 使用世界高度查询获取地面位置（不含树叶）
            i32 groundY = world->getHeight(targetBlockPos.x, targetBlockPos.z);
            BlockPos groundPos(targetBlockPos.x, groundY, targetBlockPos.z);

            // 选择移动速度
            f64 speed = isLeader ? m_leaderSpeed : m_memberSpeed;

            // 尝试移动
            bool moveSuccess = nav->moveTo(static_cast<f64>(groundPos.x) + 0.5,
                static_cast<f64>(groundPos.y),
                static_cast<f64>(groundPos.z) + 0.5,
                speed);

            if (!moveSuccess) {
                // 移动失败 -> 随机移动 + 设置冷却
                (void)_moveRandomly();
                m_cooldownTime = static_cast<i64>(world->getGameTime()) + COOLDOWN_TICKS;
            } else if (isLeader) {
                // 队长移动成功 -> 同步目标给队员
                for (PatrollerEntity* member : nearbyPatrollers) {
                    member->setPatrolTarget(groundPos);
                }
            }
        }
    }
}

std::vector<PatrollerEntity*> PatrolGoal::_getNearbyPatrollers() const
{
    std::vector<PatrollerEntity*> result;

    if (!m_patroller) return result;

    IWorld* world = m_patroller->world();
    if (!world) return result;

    // 获取碰撞箱扩展 16 格内的所有 PatrollerEntity
    auto entities = world->getEntitiesInAABB(m_patroller->boundingBox().grow(NEARBY_PATROLLER_RANGE));

    for (Entity* entity : entities) {
        // 检查是否是 PatrollerEntity
        PatrollerEntity* patroller = dynamic_cast<PatrollerEntity*>(entity);
        if (patroller == nullptr) continue;

        // 排除自己
        if (patroller->id() == m_patroller->id()) continue;

        // 检查是否可以加入巡逻
        if (patroller->canJoinPatrol()) {
            result.push_back(patroller);
        }
    }

    return result;
}

bool PatrolGoal::_moveRandomly()
{
    if (!m_patroller) return false;

    IWorld* world = m_patroller->world();
    if (!world) return false;

    Random& rng = m_patroller->getRandom();

    // 在当前位置 ±8 格范围内随机选择位置
    Vector3 currentPos = m_patroller->position();
    i32 offsetX = -RANDOM_MOVE_RANGE + rng.nextInt(RANDOM_MOVE_RANGE * 2);
    i32 offsetZ = -RANDOM_MOVE_RANGE + rng.nextInt(RANDOM_MOVE_RANGE * 2);

    BlockPos targetPos(floorTo<i32>(currentPos.x) + offsetX,
        0, // Y 稍后获取
        floorTo<i32>(currentPos.z) + offsetZ);

    // 获取地面高度
    i32 groundY = world->getHeight(targetPos.x, targetPos.z);

    PathNavigator* nav = m_patroller->navigator();
    if (!nav) return false;

    return nav->moveTo(static_cast<f64>(targetPos.x) + 0.5,
        static_cast<f64>(groundY),
        static_cast<f64>(targetPos.z) + 0.5,
        m_memberSpeed);
}

} // namespace mc::entity::ai::goal
