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

#include "MeleeAttackGoal.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../controller/LookController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include <algorithm>

namespace mc::entity::ai::goal {

using namespace constants;

MeleeAttackGoal::MeleeAttackGoal(CreatureEntity* creature, f64 speed, bool useLongMemory)
    : m_creature(creature)
    , m_speed(speed)
    , m_useLongMemory(useLongMemory)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool MeleeAttackGoal::shouldExecute()
{
    if (!m_creature) return false;

    // 游戏时间节流 - 每20 tick检查一次
    u32 ticksExisted = m_creature->ticksExisted();
    if (ticksExisted - m_lastCheckTime < constants::TARGET_CHECK_COOLDOWN) {
        return false;
    }
    m_lastCheckTime = ticksExisted;

    // 获取攻击目标
    LivingEntity* target = m_creature->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_attackTarget = target;

    // 尝试获取路径
    if (m_creature->navigator()) {
        if (m_creature->navigator()->moveTo(*target, 0)) {
            return true;
        }
    }

    // 如果路径失败，检查是否已经在攻击范围内
    f32 attackReachSq = getAttackReachSqr(target);
    f32 distSq = m_creature->distanceSqTo(*target);
    return distSq <= attackReachSq;
}

bool MeleeAttackGoal::shouldContinueExecuting()
{
    if (!m_creature || !m_attackTarget) return false;

    // 检查目标是否存活
    if (!m_attackTarget->isAlive()) {
        return false;
    }

    // 如果使用长期记忆，检查目标是否在家范围内
    if (m_useLongMemory) {
        // 只有 MobEntity 才有家范围概念
        MobEntity* mob = dynamic_cast<MobEntity*>(m_creature);
        if (mob && !mob->isWithinHomeDistanceFromPosition(BlockPos(m_attackTarget->position()))) {
            return false;
        }
    }

    // 玩家模式检查
    Player* player = dynamic_cast<Player*>(m_attackTarget);
    if (player && (player->isSpectator() || player->isCreative())) {
        return false;
    }

    // 如果不使用长期记忆且没有路径，停止
    auto* nav = m_creature->navigator();
    if (!m_useLongMemory && nav && nav->noPath()) {
        return false;
    }

    // 检查距离
    f32 distSq = m_creature->distanceSqTo(*m_attackTarget);

    if (distSq > MELEE_ATTACK_STOP_DISTANCE_SQ) {
        return false; // 目标太远，停止追踪
    }

    // 如果使用长期记忆，继续追踪
    if (m_useLongMemory) {
        return true;
    }

    return shouldExecute();
}

void MeleeAttackGoal::startExecuting()
{
    m_attackCooldown = 0;
    m_pathRecalculateTimer = 0;
    m_failedPathFindingPenalty = 0;
    m_targetX = 0.0;
    m_targetY = 0.0;
    m_targetZ = 0.0;

    if (m_creature) {
        // 设置激怒状态
        m_creature->setAggroed(true);

        if (auto* nav = m_creature->navigator()) {
            if (m_attackTarget) {
                static_cast<void>(nav->moveTo(*m_attackTarget, m_speed));
            }
        }
    }
}

void MeleeAttackGoal::resetTask()
{
    m_attackTarget = nullptr;
    m_failedPathFindingPenalty = 0;

    if (m_creature) {
        m_creature->clearNavigation();
        // 清除激怒状态，与 startExecuting 中的 setAggroed(true) 对称。
        // 对应 MC 1.21.11 MeleeAttackGoal.stop() 中调用 mob.setAggressive(false)。
        // setAggroed 内部调用 setAggressive，通过 DATA_MOB_FLAGS_PARAM 同步到客户端，
        // 驱动 ZombieModel 等模型放下举起的攻击手臂。
        m_creature->setAggroed(false);
    }
}

void MeleeAttackGoal::tick()
{
    if (!m_creature || !m_attackTarget) return;

    // 使用 LookController 看向目标
    if (auto* lookCtrl = m_creature->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_attackTarget, 30.0f, 30.0f);
    }

    f64 distSq = m_creature->distanceSqTo(*m_attackTarget);

    // 路径重算逻辑
    m_pathRecalculateTimer = std::max(m_pathRecalculateTimer - 1, 0);

    bool shouldRecalcPath = false;

    // 检查是否需要重新计算路径
    if ((m_useLongMemory || m_creature->canSee(*m_attackTarget)) && m_pathRecalculateTimer <= 0 &&
        ((m_targetX == 0.0 && m_targetY == 0.0 && m_targetZ == 0.0) ||
            m_attackTarget->distanceSqTo(m_targetX, m_targetY, m_targetZ) >= 1.0 ||
            m_creature->getRandom().nextFloat() < 0.05f)) {

        shouldRecalcPath = true;
    }

    if (shouldRecalcPath) {
        // 更新目标位置
        m_targetX = m_attackTarget->x();
        m_targetY = m_attackTarget->y();
        m_targetZ = m_attackTarget->z();

        // 随机重算间隔 (4-10)
        math::Random& rng = m_creature->getRandom();
        m_pathRecalculateTimer = PATH_RECALCULATE_BASE + rng.nextInt(PATH_RECALCULATE_RANDOM);

        // 添加路径失败惩罚
        if (m_canPenalize) {
            m_pathRecalculateTimer += m_failedPathFindingPenalty;
            m_failedPathFindingPenalty += PATH_FAILURE_PENALTY;
        }

        // 根据距离调整重算间隔
        if (distSq > DISTANCE_FAR_THRESHOLD) { // > 32格距离
            m_pathRecalculateTimer += PATH_RECALC_FAR_BONUS;
        } else if (distSq > DISTANCE_MEDIUM_THRESHOLD) { // > 16格距离
            m_pathRecalculateTimer += PATH_RECALC_MEDIUM_BONUS;
        }

        // 移动到目标
        if (m_creature->navigator()) {
            if (!m_creature->navigator()->moveTo(*m_attackTarget, m_speed)) {
                m_pathRecalculateTimer += 15; // 路径失败惩罚
            }
        }

        // 对齐 vanilla MeleeAttackGoal.tick：所有 bonus 累加后整体 adjustedTickDelay 减半，
        // 补偿半 tick 评估（m_pathRecalculateTimer 在 tick 每 tick 递减）。
        m_pathRecalculateTimer = adjustedTickDelay(m_pathRecalculateTimer);
    }

    // 减少攻击冷却
    m_attackCooldown = std::max(m_attackCooldown - 1, 0);

    // 检查是否可以攻击
    checkAndPerformAttack(m_attackTarget, distSq);
}

bool MeleeAttackGoal::_canAttack(LivingEntity* target) const
{
    if (!m_creature || !target) return false;

    // 检查是否在攻击范围内
    f32 attackReachSq = getAttackReachSqr(target);
    f32 distSq = m_creature->distanceSqTo(*target);

    return distSq <= attackReachSq;
}

void MeleeAttackGoal::checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr)
{
    if (!m_creature || !target) return;

    f32 attackReachSq = getAttackReachSqr(target);

    if (distToEnemySqr <= static_cast<f64>(attackReachSq) && m_attackCooldown <= 0) {
        // 重置攻击冷却。对齐 vanilla MeleeAttackGoal.resetAttackCooldown：adjustedTickDelay(20)。
        m_attackCooldown = adjustedTickDelay(ATTACK_COOLDOWN_TICKS);

        // 执行攻击
        _attackTarget(target);
    }
}

void MeleeAttackGoal::_attackTarget(LivingEntity* target)
{
    if (!m_creature || !target) return;

    // 挥动手臂动画（对齐 vanilla MeleeAttackGoal.checkAndPerformAttack：先 swing(MAIN_HAND)）。
    m_creature->swingArm();

    // 委托实体本身的 attackEntityAsMob 执行攻击（对齐 vanilla checkAndPerformAttack 调
    // mob.doHurtTarget(serverLevel, target)）。attackEntityAsMob 是 MobEntity 的虚函数，子类
    // override 注入攻击附加效果：
    //   - HuskEntity::attackEntityAsMob：施加饥饿效果（Husk.java:57-65 doHurtTarget）
    //   - WitherSkeletonEntity::attackEntityAsMob：施加凋零效果
    //   - CaveSpiderEntity::attackEntityAsMob：施加中毒效果
    //   - ZombieEntity::attackEntityAsMob：燃烧传递
    //   - PolarBearEntity/RavagerEntity/OcelotEntity 等 override
    // MobEntity::attackEntityAsMob 基类实现完整攻击链（伤害属性+附魔加成+火焰附加+击退+
    // setLastHurtBy+音效），此前 MeleeAttackGoal 自行 target->hurt 绕过虚派发，致上述 override
    // 全部失效（husk 不施加饥饿、凋零骷髅不施加凋零等）。改为委托后攻击逻辑统一走基类，
    // 子类 override 自动生效，且补齐附魔加成/火焰附加等之前缺失的逻辑。
    m_creature->attackEntityAsMob(*target);
}

f32 MeleeAttackGoal::getAttackReachSqr(LivingEntity* target) const
{
    // 攻击距离公式: (width * 2)^2 + targetWidth
    f32 attackerWidth = m_creature->width();
    f32 targetWidth = target->width();
    f32 reachWidth = attackerWidth * 2.0f;
    return reachWidth * reachWidth + targetWidth;
}

} // namespace mc::entity::ai::goal
