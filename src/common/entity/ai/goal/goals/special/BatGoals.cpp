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

#include "BatGoals.hpp"

#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/Block.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../entities/passive/ambient/BatEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

namespace {
// 符号函数：返回 x 的符号（-1, 0, 1）
f64 signum(f64 x)
{
    if (x > 0.0) return 1.0;
    if (x < 0.0) return -1.0;
    return 0.0;
}
} // namespace

// ============================================================================
// BatRandomFlyGoal
// ============================================================================

BatRandomFlyGoal::BatRandomFlyGoal(BatEntity* bat)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_bat(bat)
    , m_targetPos(0, 0, 0)
    , m_hasTarget(false)
    , m_cooldown(0)
{
    MC_ASSERT_RELEASE(bat != nullptr);
}

bool BatRandomFlyGoal::shouldExecute()
{
    if (m_bat == nullptr) {
        return false;
    }

    // 蝙蝠不在休息状态时执行飞行
    return !m_bat->isResting();
}

bool BatRandomFlyGoal::shouldContinueExecuting()
{
    if (m_bat == nullptr) {
        return false;
    }

    // 蝙蝠不在休息状态时继续飞行
    return !m_bat->isResting();
}

void BatRandomFlyGoal::startExecuting()
{
    // 选择初始目标点
    _selectNewTarget();
    m_cooldown = 0;
}

void BatRandomFlyGoal::resetTask()
{
    m_hasTarget = false;
    m_cooldown = 0;
}

void BatRandomFlyGoal::tick()
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return;
    }

    // 冷却计时器
    if (m_cooldown > 0) {
        --m_cooldown;
        return;
    }

    math::Random& rng = m_bat->getRandom();
    IWorld* world = m_bat->world();

    // 检查是否需要选择新目标
    bool needNewTarget = false;

    // 条件1：没有目标
    if (!m_hasTarget) {
        needNewTarget = true;
    }

    // 条件2：目标不可用（非空气或低于最低建筑高度）
    if (m_hasTarget) {
        if (!_isTargetValid(m_targetPos)) {
            needNewTarget = true;
        }
    }

    // 条件3：1/30概率随机更换目标
    if (!needNewTarget && m_hasTarget) {
        if (rng.nextInt(30) == 0) {
            needNewTarget = true;
        }
    }

    // 条件4：到达目标点（距离<2）
    if (!needNewTarget && m_hasTarget) {
        math::Vector3 currentPos = m_bat->position();
        f32 dx = static_cast<f32>(m_targetPos.x) + 0.5f - currentPos.x;
        f32 dy = static_cast<f32>(m_targetPos.y) + 0.1f - currentPos.y;
        f32 dz = static_cast<f32>(m_targetPos.z) + 0.5f - currentPos.z;
        f32 distanceSq = dx * dx + dy * dy + dz * dz;
        if (distanceSq < 4.0f) { // 距离平方 < 2^2 = 4
            needNewTarget = true;
        }
    }

    // 选择新目标
    if (needNewTarget) {
        _selectNewTarget();
    }

    // 如果有有效目标，执行飞行移动
    if (m_hasTarget) {
        // 计算到目标的方向
        f32 dx = static_cast<f32>(m_targetPos.x) + 0.5f - m_bat->position().x;
        f32 dy = static_cast<f32>(m_targetPos.y) + 0.1f - m_bat->position().y;
        f32 dz = static_cast<f32>(m_targetPos.z) + 0.5f - m_bat->position().z;

        math::Vector3 currentVel = m_bat->velocity();

        // 计算新的速度向量
        // signum(dx) * 0.5 - vel.x) * 0.1
        // Y轴调整更强 (0.7 而非 0.5)
        math::Vector3 newVel(static_cast<f32>((signum(static_cast<f64>(dx)) * 0.5 - currentVel.x) * 0.1),
            static_cast<f32>((signum(static_cast<f64>(dy)) * 0.7 - currentVel.y) * 0.1),
            static_cast<f32>((signum(static_cast<f64>(dz)) * 0.5 - currentVel.z) * 0.1));

        m_bat->setVelocity(newVel);

        // 计算朝向
        f32 targetYaw = static_cast<f32>(math::toDegrees(std::atan2(newVel.z, newVel.x))) - 90.0f;
        f32 yawDiff = math::wrapDegrees(targetYaw - m_bat->yaw());
        m_bat->setRotation(m_bat->yaw() + yawDiff * 0.1f, m_bat->pitch());
    }
}

void BatRandomFlyGoal::_selectNewTarget()
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        m_hasTarget = false;
        return;
    }

    math::Random& rng = m_bat->getRandom();
    math::Vector3 currentPos = m_bat->position();

    // 目标点范围：
    // X: 当前位置 ±7 格
    // Y: 当前位置 -2 到 +4 格
    // Z: 当前位置 ±7 格
    i32 attempts = 0;
    constexpr i32 MAX_ATTEMPTS = 20;

    while (attempts < MAX_ATTEMPTS) {
        i32 targetX = static_cast<i32>(currentPos.x) + rng.nextInt(-7, 7);
        i32 targetY = static_cast<i32>(currentPos.y) + rng.nextInt(-2, 4);
        i32 targetZ = static_cast<i32>(currentPos.z) + rng.nextInt(-7, 7);

        BlockPos candidatePos(targetX, targetY, targetZ);

        if (_isTargetValid(candidatePos)) {
            m_targetPos = candidatePos;
            m_hasTarget = true;
            return;
        }

        ++attempts;
    }

    // 找不到有效目标，标记为无目标
    m_hasTarget = false;
}

bool BatRandomFlyGoal::_isTargetValid(const BlockPos& pos) const
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return false;
    }

    IWorld* world = m_bat->world();

    // Y 必须在有效建筑高度范围内
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 目标位置必须是空气
    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 检查是否是空气方块
    return state->getBlock().isAir(*state);
}

// ============================================================================
// BatRestGoal
// ============================================================================

BatRestGoal::BatRestGoal(BatEntity* bat)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_bat(bat)
    , m_turnTimer(0)
    , m_targetYaw(0.0f)
{
    MC_ASSERT_RELEASE(bat != nullptr);
}

bool BatRestGoal::shouldExecute()
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return false;
    }

    // 检查是否是白天
    i64 timeOfDay = m_bat->world()->dayTimeOfDay();
    bool isDay = timeOfDay < 12000;

    // 白天且在飞行中且可以休息（上方有固体方块）时尝试休息
    if (!isDay) {
        return false; // 夜间不休息
    }

    if (m_bat->isResting()) {
        return false; // 已经在休息
    }

    // 1/100 概率尝试休息
    math::Random& rng = m_bat->getRandom();
    if (rng.nextInt(1, 100) != 1) {
        return false;
    }

    // 检查上方是否有固体方块
    return _canRestAtCurrentPosition();
}

bool BatRestGoal::shouldContinueExecuting()
{
    if (m_bat == nullptr) {
        return false;
    }

    // 如果已经不在休息状态，停止
    if (!m_bat->isResting()) {
        return false;
    }

    // 检查是否应该停止休息
    return !_shouldStopResting();
}

void BatRestGoal::startExecuting()
{
    if (m_bat == nullptr) {
        return;
    }

    // 进入休息状态
    m_bat->setResting(true);
    m_bat->setFlying(false);

    // 清除速度
    m_bat->setVelocity(math::Vector3(0.0f, 0.0f, 0.0f));

    // 对齐到方块位置（挂在方块下方）
    math::Vector3 pos = m_bat->position();
    i32 blockY = static_cast<i32>(std::floor(pos.y + 1.0));
    m_bat->setPosition(pos.x, static_cast<f32>(blockY) - m_bat->height() + 0.1f, pos.z);

    // 初始化转头计时器
    m_turnTimer = 0;
    m_targetYaw = m_bat->yaw();
}

void BatRestGoal::resetTask()
{
    if (m_bat == nullptr) {
        return;
    }

    // 退出休息状态
    m_bat->setResting(false);
    m_bat->setFlying(true);

    // 重置转头计时器
    m_turnTimer = 0;
}

void BatRestGoal::tick()
{
    if (m_bat == nullptr) {
        return;
    }

    // 休息时偶尔转头
    math::Random& rng = m_bat->getRandom();
    if (rng.nextInt(200) == 0) {
        // 随机选择新的转头角度
        m_targetYaw = static_cast<f32>(rng.nextInt(360));
    }

    // 平滑转向目标角度
    f32 yawDiff = math::wrapDegrees(m_targetYaw - m_bat->yaw());
    m_bat->setRotation(m_bat->yaw() + yawDiff * 0.1f, m_bat->pitch());

    // 保持静止
    m_bat->setVelocity(math::Vector3(0.0f, 0.0f, 0.0f));
}

bool BatRestGoal::_shouldStopResting() const
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return true;
    }

    // 条件1：夜间唤醒
    i64 timeOfDay = m_bat->world()->dayTimeOfDay();
    bool isDay = timeOfDay < 12000;
    if (!isDay) {
        return true; // 夜间唤醒
    }

    // 条件2：玩家靠近（4格内）
    // 检查附近是否有玩家
    constexpr f32 PLAYER_WAKE_DISTANCE = 4.0f;
    Player* closestPlayer = m_bat->world()->getClosestPlayer(m_bat->position(), PLAYER_WAKE_DISTANCE);
    if (closestPlayer != nullptr) {
        return true; // 玩家靠近，唤醒
    }

    // 条件3：失去支撑（上方不再是固体方块）
    if (!_canRestAtCurrentPosition()) {
        return true;
    }

    return false;
}

bool BatRestGoal::_canRestAtCurrentPosition() const
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return false;
    }

    // 检查上方是否有固体方块
    math::Vector3 pos = m_bat->position();
    BlockPos abovePos(static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y + m_bat->height() + 0.1f)),
        static_cast<i32>(std::floor(pos.z)));

    const BlockState* state = m_bat->world()->getBlockState(abovePos);
    if (state == nullptr) {
        return false;
    }

    // 检查方块是否是固体的
    return state->getBlock().isSolid(*state);
}

} // namespace mc::entity::ai::goal
