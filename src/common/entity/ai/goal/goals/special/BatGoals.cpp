/**
 * @file BatGoals.cpp
 * @brief 蝙蝠专用的AI目标类实现
 *
 * 参考 MC 1.16.5: net.minecraft.entity.passive.BatEntity
 */

#include "BatGoals.hpp"
#include "../../entities/passive/ambient/BatEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "../../core/Entity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../world/World.hpp"

namespace mc::entity::ai::goal {

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
    selectNewTarget();
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

    math::Random rng = m_bat->getRandom();
    IWorld* world = m_bat->world();

    // 检查是否需要选择新目标
    bool needNewTarget = false;

    // 条件1：没有目标
    if (!m_hasTarget) {
        needNewTarget = true;
    }

    // 条件2：目标不可用（非空气或Y<1）
    if (m_hasTarget) {
        if (!isTargetValid(m_targetPos)) {
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
        f32 dx = static_cast<f32>(m_targetPos.x()) + 0.5f - currentPos.x;
        f32 dy = static_cast<f32>(m_targetPos.y()) + 0.1f - currentPos.y;
        f32 dz = static_cast<f32>(m_targetPos.z()) + 0.5f - currentPos.z;
        f32 distanceSq = dx * dx + dy * dy + dz * dz;
        if (distanceSq < 4.0f) { // 距离平方 < 2^2 = 4
            needNewTarget = true;
        }
    }

    // 选择新目标
    if (needNewTarget) {
        selectNewTarget();
    }

    // 如果有有效目标，执行飞行移动
    if (m_hasTarget) {
        // MC 1.16.5: BatEntity 第150-159行
        // 计算到目标的方向
        f32 dx = static_cast<f32>(m_targetPos.x()) + 0.5f - m_bat->position().x;
        f32 dy = static_cast<f32>(m_targetPos.y()) + 0.1f - m_bat->position().y;
        f32 dz = static_cast<f32>(m_targetPos.z()) + 0.5f - m_bat->position().z;

        math::Vector3 currentVel = m_bat->velocity();

        // 计算新的速度向量
        // signum(dx) * 0.5 - vel.x) * 0.1
        // Y轴调整更强 (0.7 而非 0.5)
        math::Vector3 newVel(
            static_cast<f32>((math::signum(static_cast<f64>(dx)) * 0.5 - currentVel.x) * 0.1),
            static_cast<f32>((math::signum(static_cast<f64>(dy)) * 0.7 - currentVel.y) * 0.1),
            static_cast<f32>((math::signum(static_cast<f64>(dz)) * 0.5 - currentVel.z) * 0.1)
        );

        m_bat->setVelocity(newVel);

        // 计算朝向
        f32 targetYaw = static_cast<f32>(math::toDegrees(std::atan2(newVel.z, newVel.x))) - 90.0f;
        f32 yawDiff = math::wrapDegrees(targetYaw - m_bat->yaw());
        m_bat->setRotation(m_bat->yaw() + yawDiff * 0.1f, m_bat->pitch());
    }
}

void BatRandomFlyGoal::selectNewTarget()
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        m_hasTarget = false;
        return;
    }

    math::Random rng = m_bat->getRandom();
    math::Vector3 currentPos = m_bat->position();

    // MC 1.16.5: BatEntity 第142-148行
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

        if (isTargetValid(candidatePos)) {
            m_targetPos = candidatePos;
            m_hasTarget = true;
            return;
        }

        ++attempts;
    }

    // 找不到有效目标，标记为无目标
    m_hasTarget = false;
}

bool BatRandomFlyGoal::isTargetValid(const BlockPos& pos) const
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return false;
    }

    IWorld* world = m_bat->world();

    // Y 必须 >= 1（世界最低高度限制）
    if (pos.y() < 1) {
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
    i64 timeOfDay = m_bat->world()->dayTime() % 24000;
    bool isDay = timeOfDay < 12000;

    // 白天且在飞行中且可以休息（上方有固体方块）时尝试休息
    if (!isDay) {
        return false; // 夜间不休息
    }

    if (m_bat->isResting()) {
        return false; // 已经在休息
    }

    // 1/100 概率尝试休息
    // MC 1.16.5: BatEntity 第160-162行
    math::Random rng = m_bat->getRandom();
    if (rng.nextInt(1, 100) != 1) {
        return false;
    }

    // 检查上方是否有固体方块
    return canRestAtCurrentPosition();
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
    return !shouldStopResting();
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
    // MC 1.16.5: BatEntity 第125-127行
    math::Random rng = m_bat->getRandom();
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

bool BatRestGoal::shouldStopResting() const
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return true;
    }

    // 条件1：夜间唤醒
    // MC 1.16.5: BatEntity 第129行
    i64 timeOfDay = m_bat->world()->dayTime() % 24000;
    bool isDay = timeOfDay < 12000;
    if (!isDay) {
        return true; // 夜间唤醒
    }

    // 条件2：玩家靠近（4格内）
    // MC 1.16.5: BatEntity 第131-133行
    // 需要检查附近是否有玩家
    // 这里使用简化检查：通过 world()->getClosestPlayer() 检查
    // 但该方法可能未实现，暂时跳过
    // TODO: 当 world()->getClosestPlayer() 实现后添加检查

    // 条件3：失去支撑（上方不再是固体方块）
    if (!canRestAtCurrentPosition()) {
        return true;
    }

    return false;
}

bool BatRestGoal::canRestAtCurrentPosition() const
{
    if (m_bat == nullptr || m_bat->world() == nullptr) {
        return false;
    }

    // 检查上方是否有固体方块
    math::Vector3 pos = m_bat->position();
    BlockPos abovePos(
        static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y + m_bat->height() + 0.1f)),
        static_cast<i32>(std::floor(pos.z))
    );

    const BlockState* state = m_bat->world()->getBlockState(abovePos);
    if (state == nullptr) {
        return false;
    }

    // 检查方块是否是固体的
    return state->getBlock().isSolid(*state);
}

} // namespace mc::entity::ai::goal
