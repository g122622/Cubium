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

#include "ShulkerGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/components/MobFlagComponent.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/end/ShulkerEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// ShulkerAttackGoal 实现
// ============================================================================

ShulkerAttackGoal::ShulkerAttackGoal(ShulkerEntity* shulker)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_shulker(shulker)
{
    MC_ASSERT_RELEASE(shulker != nullptr);
}

bool ShulkerAttackGoal::shouldExecute()
{
    // 检查是否有攻击目标
    LivingEntity* target = m_shulker->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 检查目标是否在攻击范围内
    double distSq = m_shulker->position().distanceSquared(target->position());
    if (distSq > ATTACK_RANGE_SQ) {
        return false;
    }

    m_target = target;
    return true;
}

bool ShulkerAttackGoal::shouldContinueExecuting()
{
    if (m_target == nullptr || !m_target->isAlive()) {
        return false;
    }

    // 目标仍在范围内
    double distSq = m_shulker->position().distanceSquared(m_target->position());
    return distSq <= ATTACK_RANGE_SQ;
}

void ShulkerAttackGoal::startExecuting()
{
    // 开始攻击时打开贝壳
    m_shulker->openShell();
    m_shulker->setAttacking(true);
}

void ShulkerAttackGoal::resetTask()
{
    // 结束时关闭贝壳
    m_shulker->closeShell();
    m_shulker->setAttacking(false);
    m_target = nullptr;
}

void ShulkerAttackGoal::tick()
{
    if (m_target == nullptr) {
        return;
    }

    // 看向目标
    m_shulker->lookAt(*m_target);

    // 如果贝壳打开且攻击冷却完成，发射子弹
    if (m_shulker->isShellOpen() && m_shulker->getAttackCooldown() <= 0) {
        m_shulker->shootBullet();
    }
}

// ============================================================================
// ShulkerPeekGoal 实现
// ============================================================================

ShulkerPeekGoal::ShulkerPeekGoal(ShulkerEntity* shulker)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move})
    , m_shulker(shulker)
{
    MC_ASSERT_RELEASE(shulker != nullptr);
}

bool ShulkerPeekGoal::shouldExecute()
{
    // 只在没有攻击目标时执行
    LivingEntity* target = m_shulker->attackTarget();
    if (target != nullptr && target->isAlive()) {
        return false;
    }

    // 贝壳必须闭合
    if (!m_shulker->isShellClosed()) {
        return false;
    }

    // 随机概率触发
    IWorld* world = m_shulker->world();
    if (world == nullptr) {
        return false;
    }

    math::Random& rng = world->getRandom();
    return rng.nextFloat() < PEEK_CHANCE;
}

bool ShulkerPeekGoal::shouldContinueExecuting()
{
    // 有攻击目标时停止
    LivingEntity* target = m_shulker->attackTarget();
    if (target != nullptr && target->isAlive()) {
        return false;
    }

    // 张望时间未结束
    return m_peekTime < m_totalPeekTime;
}

void ShulkerPeekGoal::startExecuting()
{
    // 开始张望
    m_shulker->openShell();

    // 随机张望时间
    IWorld* world = m_shulker->world();
    if (world != nullptr) {
        math::Random& rng = world->getRandom();
        m_totalPeekTime = MIN_PEEK_TIME + rng.nextInt(MAX_PEEK_TIME - MIN_PEEK_TIME + 1);
    } else {
        m_totalPeekTime = MIN_PEEK_TIME;
    }
    m_peekTime = 0;
}

void ShulkerPeekGoal::resetTask()
{
    // 结束时关闭贝壳
    m_shulker->closeShell();
    m_peekTime = 0;
    m_totalPeekTime = 0;
}

void ShulkerPeekGoal::tick()
{
    m_peekTime++;

    // 每秒随机看向不同方向
    if (m_peekTime % 20 == 0) {
        IWorld* world = m_shulker->world();
        math::Random& rng = world->getRandom();
        // 随机旋转视角 [-180, 180)
        f32 yaw = rng.nextFloat() * 360.0f - 180.0f;
        m_shulker->setRotation(yaw, m_shulker->pitch());
    }
}

// ============================================================================
// ShulkerNearestAttackGoal 实现
// ============================================================================

ShulkerNearestAttackGoal::ShulkerNearestAttackGoal(ShulkerEntity* shulker)
    : NearestAttackableTargetGoal<Player>(shulker, true)
{}

bool ShulkerNearestAttackGoal::shouldExecute()
{
    // 和平难度下不攻击玩家
    IWorld* world = m_mob->world();
    if (world != nullptr && world->difficulty() == Difficulty::Peaceful) {
        return false;
    }
    return NearestAttackableTargetGoal<Player>::shouldExecute();
}

// ============================================================================
// ShulkerDefenseAttackGoal 实现
// ============================================================================

ShulkerDefenseAttackGoal::ShulkerDefenseAttackGoal(ShulkerEntity* shulker)
    : NearestAttackableTargetGoal<LivingEntity>(shulker,
          true, // checkSight
          10,   // chance: 每10tick检查一次
          [](const LivingEntity* entity) -> bool {
              // 只攻击敌对生物（MobFlagComponent 标记组件，IMob 接口的 tag 层）
              return entity->hasComponent<ecs::MobFlagComponent>();
          })
{}

bool ShulkerDefenseAttackGoal::shouldExecute()
{
    // 只有当潜影贝处于队伍中时才执行防御攻击
    // 未分配队伍的潜影贝不会主动攻击其他怪物
    if (m_mob->getTeam() == nullptr) {
        return false;
    }
    return NearestAttackableTargetGoal<LivingEntity>::shouldExecute();
}

} // namespace entity::ai::goal
} // namespace mc
