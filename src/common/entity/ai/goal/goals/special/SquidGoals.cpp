/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "SquidGoals.hpp"

#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/water/SquidEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// SquidMoveRandomGoal
// ============================================================================

SquidMoveRandomGoal::SquidMoveRandomGoal(SquidEntity* squid)
    : Goal() // 无互斥标志，允许与其他目标同时运行
    , m_squid(squid)
{
    MC_ASSERT_RELEASE(squid != nullptr);
}

bool SquidMoveRandomGoal::shouldExecute()
{
    // 始终可以执行
    return true;
}

void SquidMoveRandomGoal::tick()
{
    if (m_squid == nullptr) {
        return;
    }

    // 获取空闲时间
    i32 idleTime = m_squid->idleTime();

    if (idleTime > IDLE_THRESHOLD) {
        // 空闲超过阈值，停止移动
        m_squid->setMovementVector(0.0f, 0.0f, 0.0f);
    } else {
        // 检查是否需要生成新的移动向量
        // 条件：1/50概率 或 不在水中 或 没有移动向量
        math::Random& rng = m_squid->getRandom();
        bool needNewVector =
            (rng.nextInt(RANDOM_CHANCE) == 0) || (!m_squid->isInWater()) || (!m_squid->hasMovementVector());

        if (needNewVector) {
            // 生成随机角度 [0, 2π)
            f32 angle = rng.nextFloat() * math::TWO_PI;

            // 计算移动向量分量
            // X = cos(角度) * 0.2
            f32 vecX = std::cos(angle) * HORIZONTAL_SPEED;
            // Y = -0.1 + random * 0.2 (范围 [-0.1, 0.1])
            f32 vecY = VERTICAL_MIN + rng.nextFloat() * VERTICAL_RANGE;
            // Z = sin(角度) * 0.2
            f32 vecZ = std::sin(angle) * HORIZONTAL_SPEED;

            m_squid->setMovementVector(vecX, vecY, vecZ);
        }
    }
}

// ============================================================================
// SquidFleeGoal
// ============================================================================

SquidFleeGoal::SquidFleeGoal(SquidEntity* squid)
    : Goal() // 无互斥标志，允许与其他目标同时运行
    , m_squid(squid)
    , m_fleeTarget(nullptr)
    , m_tickCounter(0)
{
    MC_ASSERT_RELEASE(squid != nullptr);
}

bool SquidFleeGoal::shouldExecute()
{
    if (m_squid == nullptr) {
        return false;
    }

    // 检查是否在水中
    if (!m_squid->isInWater()) {
        return false;
    }

    // 获取复仇目标（攻击者）
    LivingEntity* revengeTarget = m_squid->getLastHurtBy();
    if (revengeTarget == nullptr) {
        return false;
    }

    // 检查距离
    f64 distSq = m_squid->distanceSqTo(*revengeTarget);
    if (distSq >= FLEE_DISTANCE_SQ) {
        return false;
    }

    m_fleeTarget = revengeTarget;
    return true;
}

void SquidFleeGoal::startExecuting()
{
    m_tickCounter = 0;
}

void SquidFleeGoal::tick()
{
    if (m_squid == nullptr || m_fleeTarget == nullptr) {
        return;
    }

    ++m_tickCounter;

    // 计算远离敌人的方向向量
    f64 dx = m_squid->x() - m_fleeTarget->x();
    f64 dy = m_squid->y() - m_fleeTarget->y();
    f64 dz = m_squid->z() - m_fleeTarget->z();

    // 检查逃跑目标位置的方块和流体状态
    // 只有当目标位置是水或空气时，鱿鱼才会向该方向逃跑
    IWorld* world = m_squid->world();
    if (world != nullptr) {
        BlockPos targetPos(Vector3(static_cast<f32>(m_squid->x() + dx),
            static_cast<f32>(m_squid->y() + dy),
            static_cast<f32>(m_squid->z() + dz)));

        const fluid::FluidState* fluidState = world->getFluidState(targetPos);
        const BlockState* blockState = world->getBlockState(targetPos);

        bool isWater =
            fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER());
        bool isAir = blockState == nullptr || blockState->isAir();

        if (!isWater && !isAir) {
            // 目标位置既不是水也不是空气，不设置移动向量
            return;
        }

        f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (distance > 0.0) {
            // 归一化方向向量
            dx /= distance;
            dy /= distance;
            dz /= distance;

            // 根据距离调整速度
            f32 speed = BASE_FLEE_SPEED;
            if (distance > DISTANCE_THRESHOLD) {
                speed =
                    static_cast<f32>(static_cast<f64>(speed) - (distance - DISTANCE_THRESHOLD) / DISTANCE_THRESHOLD);
            }

            if (speed > 0.0f) {
                dx *= speed;
                dy *= speed;
                dz *= speed;
            }

            // 如果目标是空气，移除 Y 分量避免跳出水面
            if (isAir) {
                dy = 0.0;
            }

            // 设置移动向量（除以 20 转换为每 tick 速度）
            m_squid->setMovementVector(static_cast<f32>(dx) / SPEED_SCALE,
                static_cast<f32>(dy) / SPEED_SCALE,
                static_cast<f32>(dz) / SPEED_SCALE);
        }
    }

    // 每 10 tick 的第 5 tick 产生气泡粒子
    if (m_tickCounter % BUBBLE_INTERVAL == BUBBLE_OFFSET) {
        IWorld* worldPtr = m_squid->world();
        if (worldPtr != nullptr) {
            using namespace mc::particle;
            worldPtr->addParticle(
                ParticleTypeId::Bubble, Vector3(m_squid->x(), m_squid->y(), m_squid->z()), Vector3(0.0f, 0.0f, 0.0f));
        }
    }
}

} // namespace mc::entity::ai::goal
