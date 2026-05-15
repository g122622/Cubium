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

#include "VexGoals.hpp"

#include "../../../../entities/monster/illager/VexEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/block/Block.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../controller/MovementController.hpp"
#include "../../GoalFlag.hpp"
#include "../../GoalConstants.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/AxisAlignedBB.hpp"

namespace mc::entity::ai::goal {

// ==================== VexChargeAttackGoal ====================

VexChargeAttackGoal::VexChargeAttackGoal(VexEntity* vex)
    : m_vex(vex)
{
    // MC 1.16.5: 只占用 MOVE 标志
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool VexChargeAttackGoal::shouldExecute()
{
    if (!m_vex) return false;

    // MC 1.16.5: 条件检查
    // 1. 有攻击目标
    // 2. 移动控制器未更新
    // 3. 1/7 概率
    // 4. 距离 > 2格（4.0 = 2^2）
    LivingEntity* attackTarget = m_vex->attackTarget();
    if (!attackTarget || !attackTarget->isAlive()) {
        return false;
    }

    // MC 1.16.5: 检查移动控制器是否未更新
    auto* moveController = m_vex->moveController();
    if (moveController && moveController->isUpdating()) {
        return false;
    }

    // MC 1.16.5: 1/7 概率
    math::Random rng = m_vex->getRandom();
    if (rng.nextInt(CHARGE_PROBABILITY) != 0) {
        return false;
    }

    // MC 1.16.5: 检查距离 > 2格
    f64 distSq = m_vex->distanceSqTo(*attackTarget);
    return distSq > MIN_CHARGE_DISTANCE_SQ;
}

bool VexChargeAttackGoal::shouldContinueExecuting()
{
    if (!m_vex) return false;

    auto* moveController = m_vex->moveController();
    LivingEntity* attackTarget = m_vex->attackTarget();

    // MC 1.16.5: 继续条件
    // 1. 移动控制器正在更新
    // 2. 正在充电
    // 3. 有攻击目标且存活
    return moveController && moveController->isUpdating()
        && m_vex->isCharging()
        && attackTarget && attackTarget->isAlive();
}

void VexChargeAttackGoal::startExecuting()
{
    if (!m_vex) return;

    LivingEntity* attackTarget = m_vex->attackTarget();
    if (!attackTarget) return;

    // MC 1.16.5: 获取目标的眼睛位置
    Vector3 targetEyePos(
        static_cast<f64>(attackTarget->x()),
        static_cast<f64>(attackTarget->y() + attackTarget->eyeHeight()),
        static_cast<f64>(attackTarget->z()));

    // MC 1.16.5: 移动到目标眼睛位置，速度为 1.0（全速）
    auto* moveController = m_vex->moveController();
    if (moveController) {
        moveController->setMoveTo(targetEyePos.x, targetEyePos.y, targetEyePos.z, 1.0);
    }

    // MC 1.16.5: 设置充电状态
    m_vex->setCharging(true);

    // MC 1.16.5: 播放充电音效
    // world.playSound(null, posX, posY, posZ, SoundEvents.ENTITY_VEX_CHARGE, SoundCategory.HOSTILE, 1.0F, 1.0F);
    // TODO: 当音效系统完善后播放音效
}

void VexChargeAttackGoal::resetTask()
{
    if (m_vex) {
        m_vex->setCharging(false);
    }
    m_attackCooldown = 0;
}

void VexChargeAttackGoal::tick()
{
    if (!m_vex) return;

    LivingEntity* attackTarget = m_vex->attackTarget();
    if (!attackTarget || !attackTarget->isAlive()) {
        return;
    }

    // MC 1.16.5: 使用 LookController 看向目标
    auto* lookController = m_vex->lookController();
    if (lookController) {
        lookController->setLookPositionWithEntity(*attackTarget, 30.0f, 30.0f);
    }

    f64 distSq = m_vex->distanceSqTo(*attackTarget);

    // 减少攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // MC 1.16.5: 检查碰撞箱是否相交
    // 如果相交，攻击目标
    AxisAlignedBB vexBox = m_vex->boundingBox();
    AxisAlignedBB targetBox = attackTarget->boundingBox();
    if (vexBox.intersects(targetBox)) {
        // 执行攻击
        checkAndPerformAttack(attackTarget, distSq);
        // MC 1.16.5: 攻击后停止充电
        m_vex->setCharging(false);
    } else if (distSq < STOP_CHASE_DISTANCE_SQ) {
        // MC 1.16.5: 如果距离 < 3格，继续追击目标的眼睛位置
        Vector3 targetEyePos(
            static_cast<f64>(attackTarget->x()),
            static_cast<f64>(attackTarget->y() + attackTarget->eyeHeight()),
            static_cast<f64>(attackTarget->z()));
        auto* moveController = m_vex->moveController();
        if (moveController) {
            moveController->setMoveTo(targetEyePos.x, targetEyePos.y, targetEyePos.z, 1.0);
        }
    }
}

void VexChargeAttackGoal::checkAndPerformAttack(LivingEntity* target, f64 /*distSq*/)
{
    if (!m_vex || !target) return;

    // 检查攻击冷却
    if (m_attackCooldown > 0) return;

    // MC 1.16.5: 执行攻击
    m_vex->attackEntityAsMob(*target);

    // 设置攻击冷却
    m_attackCooldown = ATTACK_COOLDOWN_TICKS;
}

// ==================== VexMoveRandomGoal ====================

VexMoveRandomGoal::VexMoveRandomGoal(VexEntity* vex)
    : m_vex(vex)
{
    // MC 1.16.5: 只占用 MOVE 标志
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool VexMoveRandomGoal::shouldExecute()
{
    if (!m_vex) return false;

    // MC 1.16.5: 条件检查
    // 1. 移动控制器未更新
    // 2. 1/7 概率
    auto* moveController = m_vex->moveController();
    if (moveController && moveController->isUpdating()) {
        return false;
    }

    math::Random rng = m_vex->getRandom();
    return rng.nextInt(RANDOM_PROBABILITY) == 0;
}

bool VexMoveRandomGoal::shouldContinueExecuting()
{
    // MC 1.16.5: 单次执行
    return false;
}

void VexMoveRandomGoal::tick()
{
    if (!m_vex) return;

    IWorld* world = m_vex->world();
    if (!world) return;

    // MC 1.16.5: 获取绑定原点（主人的位置或当前位置）
    BlockPos origin(m_vex->position());

    // 尝试找到随机位置
    math::Random rng = m_vex->getRandom();

    for (i32 i = 0; i < 3; ++i) {
        // MC 1.16.5: 在原点周围随机选择位置
        // 范围：X: -7~7, Y: -5~5, Z: -7~7
        i32 offsetX = rng.nextInt(WANDER_RANGE_X * 2 + 1) - WANDER_RANGE_X;
        i32 offsetY = rng.nextInt(WANDER_RANGE_Y * 2 + 1) - WANDER_RANGE_Y;
        i32 offsetZ = rng.nextInt(WANDER_RANGE_Z * 2 + 1) - WANDER_RANGE_Z;

        BlockPos targetPos(origin.x + offsetX, origin.y + offsetY, origin.z + offsetZ);

        // MC 1.16.5: 只移动到空气方块位置
        if (const BlockState* state = world->getBlockState(targetPos)) {
            if (state->isAir()) {
                // 找到空气位置，移动到那里
                auto* moveController = m_vex->moveController();
                if (moveController) {
                    moveController->setMoveTo(
                        static_cast<f64>(targetPos.x) + 0.5,
                        static_cast<f64>(targetPos.y) + 0.5,
                        static_cast<f64>(targetPos.z) + 0.5,
                        WANDER_SPEED);
                }

                // MC 1.16.5: 如果没有攻击目标，看向目标位置
                if (m_vex->attackTarget() == nullptr) {
                    auto* lookController = m_vex->lookController();
                    if (lookController) {
                        lookController->setLookPosition(
                            static_cast<f64>(targetPos.x) + 0.5,
                            static_cast<f64>(targetPos.y) + 0.5,
                            static_cast<f64>(targetPos.z) + 0.5,
                            180.0f, 20.0f);
                    }
                }
                break;
            }
        }
    }
}

// ==================== VexCopyOwnerTargetGoal ====================

VexCopyOwnerTargetGoal::VexCopyOwnerTargetGoal(VexEntity* vex)
    : TargetGoal(vex, false)  // MC 1.16.5: checkSight = false，在shouldExecute中手动检查
    , m_vex(vex)
{
}

bool VexCopyOwnerTargetGoal::shouldExecute()
{
    if (!m_vex) return false;

    // MC 1.16.5: 检查主人是否存在且有攻击目标
    LivingEntity* owner = m_vex->getOwner();
    if (!owner) return false;

    // MC 1.16.5: 主人必须是 MobEntity 才有 attackTarget
    MobEntity* ownerMob = dynamic_cast<MobEntity*>(owner);
    if (!ownerMob) return false;

    LivingEntity* ownerTarget = ownerMob->attackTarget();
    if (!ownerTarget || !ownerTarget->isAlive()) {
        return false;
    }

    // MC 1.16.5: 检查目标是否适合攻击（使用TargetGoal的isSuitableTarget）
    if (!isSuitableTarget(ownerTarget)) {
        return false;
    }

    // MC 1.16.5: 检查视线（手动检查）
    if (!m_vex->canSee(*ownerTarget)) {
        return false;
    }

    m_target = ownerTarget;
    return true;
}

void VexCopyOwnerTargetGoal::startExecuting()
{
    // MC 1.16.5: 设置攻击目标为主人的攻击目标
    TargetGoal::startExecuting();
}

} // namespace mc::entity::ai::goal
