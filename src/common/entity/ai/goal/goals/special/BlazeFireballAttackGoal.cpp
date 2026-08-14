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

#include "BlazeFireballAttackGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/nether/BlazeEntity.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include <cmath>
#include <memory>
#include <utility>

namespace mc::entity::ai::goal {

BlazeFireballAttackGoal::BlazeFireballAttackGoal(BlazeEntity* blaze)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_blaze(blaze)
{
    MC_ASSERT_RELEASE(blaze != nullptr);
}

bool BlazeFireballAttackGoal::shouldExecute()
{
    if (m_blaze == nullptr) {
        return false;
    }

    // 获取攻击目标
    LivingEntity* target = m_blaze->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 检查目标是否可以被攻击（存活、在同一世界等）
    if (!_isTargetValid(target)) {
        return false;
    }

    m_target = target;
    return true;
}

bool BlazeFireballAttackGoal::shouldContinueExecuting()
{
    if (m_blaze == nullptr || m_target == nullptr) {
        return false;
    }

    // 检查目标是否仍然有效
    if (!_isTargetValid(m_target)) {
        return false;
    }

    return true;
}

void BlazeFireballAttackGoal::startExecuting()
{
    // 重置攻击状态
    m_attackStep = 0;
    m_attackTime = 0;
    m_unseenTime = 0;
}

void BlazeFireballAttackGoal::resetTask()
{
    // 清除燃烧状态
    m_target = nullptr;
    m_attackStep = 0;
    m_attackTime = 0;
    m_unseenTime = 0;

    if (m_blaze != nullptr) {
        m_blaze->setCharging(false);
    }
}

void BlazeFireballAttackGoal::tick()
{
    if (m_blaze == nullptr || m_target == nullptr) {
        return;
    }

    // 递减攻击计时器
    if (m_attackTime > 0) {
        m_attackTime--;
    }

    // 检查视线
    bool canSee = m_blaze->canSee(*m_target);
    if (canSee) {
        m_unseenTime = 0;
    } else {
        m_unseenTime++;
    }

    // 计算到目标的距离平方
    f64 distSq = m_blaze->distanceSqTo(m_target->x(), m_target->y(), m_target->z());

    // 近战范围
    if (distSq < MELEE_RANGE_SQ) {
        // 太近了，如果看不到目标则不攻击
        if (!canSee) {
            return;
        }

        // 近战攻击
        if (m_attackTime <= 0) {
            m_attackTime = 20;
            // 执行近战攻击
            m_blaze->attackEntityAsMob(*m_target);
        }

        // 移动到目标位置
        auto* moveCtrl = m_blaze->moveController();
        if (moveCtrl) {
            moveCtrl->setMoveTo(m_target->x(), m_target->y(), m_target->z(), 1.0);
        }
    } else if (distSq < _getFollowDistance() * _getFollowDistance() && canSee) {
        // 在火球攻击范围内且能看到目标
        _performFireballAttack(m_target, distSq);
    } else if (m_unseenTime < 5) {
        // 看不到目标但时间不长，继续追踪
        auto* moveCtrl = m_blaze->moveController();
        if (moveCtrl) {
            moveCtrl->setMoveTo(m_target->x(), m_target->y(), m_target->z(), 1.0);
        }
    }

    // 看向目标
    auto* lookCtrl = m_blaze->lookController();
    if (lookCtrl) {
        lookCtrl->setLookPositionWithEntity(*m_target, 10.0f, 10.0f);
    }
}

bool BlazeFireballAttackGoal::_isTargetValid(LivingEntity* target) const
{
    if (target == nullptr) {
        return false;
    }

    // 检查目标是否存活
    if (!target->isAlive()) {
        return false;
    }

    // 检查目标是否在同一世界
    if (target->world() != m_blaze->world()) {
        return false;
    }

    return true;
}

f64 BlazeFireballAttackGoal::_getFollowDistance() const
{
    if (m_blaze == nullptr) {
        return 16.0; // 默认追踪范围
    }

    // 从属性获取追踪范围
    return m_blaze->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 48.0);
}

void BlazeFireballAttackGoal::_performFireballAttack(LivingEntity* target, f64 distanceToTargetSq)
{
    if (m_blaze == nullptr || target == nullptr) {
        return;
    }

    // 计算发射方向（对齐 MC 1.21.11 Blaze.BlazeAttackGoal.tick：d2 = target.getY(0.5) - blaze.getY(0.5)）。
    // 用实体几何中心（height*0.5）而非 eyeHeight：vanilla 烈焰人火球瞄准目标身体中部，
    // 用 eyeHeight 会使火球朝目标眼部偏上方飞，对矮小或站立目标的命中率偏离原版。
    f64 dx = target->x() - m_blaze->x();
    f64 dy = target->getY(0.5) - m_blaze->getY(0.5);
    f64 dz = target->z() - m_blaze->z();

    if (m_attackTime <= 0) {
        m_attackStep++;

        if (m_attackStep == 1) {
            // 阶段1：开始充能
            m_attackTime = CHARGE_TIME; // 60 ticks
            m_blaze->setCharging(true);
        } else if (m_attackStep <= MAX_FIREBALLS + 1) {
            // 阶段2-4：发射火球（最多3个）
            m_attackTime = FIREBALL_INTERVAL; // 6 ticks

            // 计算散布
            f32 spread = std::sqrt(static_cast<f32>(std::sqrt(distanceToTargetSq))) * 0.5f;

            math::Random& rng = m_blaze->world()->getRandom();

            // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
            auto* registry = m_blaze->world()->entityRegistry();
            if (registry == nullptr) {
                return;
            }

            // 创建并发射小火球
            auto fireball = std::make_unique<SmallFireballEntity>(EntityInstanceId(0), *registry);
            fireball->setTypeId(::mc::entity::EntityTypeKeys::SMALL_FIREBALL); // 工厂绕过补救：直接构造缺 typeId

            // 设置世界
            fireball->setWorld(m_blaze->world());

            // 设置位置：从烈焰人几何中心上方 0.5 发射（对齐 MC 1.21.11 Blaze：
            // smallfireball.setPos(x, this.blaze.getY(0.5) + 0.5, z)）。
            f32 fireballX = static_cast<f32>(m_blaze->x());
            f32 fireballY = static_cast<f32>(m_blaze->getY(0.5) + 0.5);
            f32 fireballZ = static_cast<f32>(m_blaze->z());
            fireball->setPosition(fireballX, fireballY, fireballZ);

            // 设置发射者
            fireball->setShooter(m_blaze);

            // 计算发射方向（带散布）
            f64 accelX = dx + rng.nextGaussian() * static_cast<f64>(spread);
            f64 accelY = dy;
            f64 accelZ = dz + rng.nextGaussian() * static_cast<f64>(spread);

            // 小火球使用加速度而非速度，加速度 = 方向 * 0.1
            fireball->setAcceleration(
                static_cast<f32>(accelX * 0.1), static_cast<f32>(accelY * 0.1), static_cast<f32>(accelZ * 0.1));

            // 在世界中生成火球
            if (m_blaze->world()) {
                // 播放发射音效
                m_blaze->world()->playEvent(world::WorldEvents::BLAZE_SHOOT_SOUND,
                    BlockPos(
                        static_cast<i32>(m_blaze->x()), static_cast<i32>(m_blaze->y()), static_cast<i32>(m_blaze->z())),
                    0);
                m_blaze->world()->spawnEntity(std::move(fireball));
            }
        } else {
            // 阶段5：冷却
            m_attackTime = COOLDOWN_TIME; // 100 ticks
            m_attackStep = 0;
            m_blaze->setCharging(false);
        }
    }
}

} // namespace mc::entity::ai::goal
