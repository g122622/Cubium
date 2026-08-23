/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do, subject to the following conditions:
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

#include "PandaGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/passive/special/PandaEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"

#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// PandaRollGoal
// ============================================================================

PandaRollGoal::PandaRollGoal(PandaEntity* panda)
    : m_panda(panda)
{
    // 互斥标志: MOVE, LOOK, JUMP
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
}

bool PandaRollGoal::shouldExecute()
{
    if (m_panda == nullptr) {
        return false;
    }

    // 条件1：幼年或顽皮性格
    if (!m_panda->isChild() && !m_panda->isPlayful()) {
        return false;
    }

    // 必须在地面
    if (!m_panda->onGround()) {
        return false;
    }

    // 条件2：检查熊猫是否可以执行动作（不在打喷嚏、吃东西、躺着、打滚等状态）
    if (!m_panda->canPerformAction()) {
        return false;
    }

    // 条件3：检查前方是否有悬崖或概率触发
    math::Random& rng = m_panda->getRandom();
    if (_isCliffInFront()) {
        // 如果前方是悬崖，100% 触发
        return true;
    } else if (m_panda->isPlayful()) {
        // 顽皮性格：1/60 概率
        return rng.nextInt(PLAYFUL_ROLL_CHANCE) == 0;
    } else {
        // 幼年熊猫（非顽皮）：1/500 概率
        return rng.nextInt(NORMAL_ROLL_CHANCE) == 0;
    }
}

bool PandaRollGoal::shouldContinueExecuting()
{
    // 打滚是一次性动作，由 rollCounter 控制持续时间
    // Goal 只负责触发，PandaEntity::updateRoll() 负责物理更新
    return false;
}

void PandaRollGoal::startExecuting()
{
    // 设置打滚状态，由 tick() 中的 updateRoll() 处理物理
    m_panda->setRolling(true);
    // 初始设置为 0，由 updateRoll() 递增
    m_panda->setRollTimer(0);
}

bool PandaRollGoal::_isCliffInFront() const
{
    if (m_panda == nullptr || m_panda->world() == nullptr) {
        return false;
    }

    IWorld* world = m_panda->world();

    // 计算熊猫朝向方向的前方位置
    const f32 yaw = m_panda->yaw();
    const f32 yawRad = math::toRadians(yaw);
    const f32 sinYaw = std::sin(yawRad);
    const f32 cosYaw = std::cos(yawRad);

    // 计算前方一格的偏移（当偏移超过0.5时才移动）
    i32 offsetX = 0;
    i32 offsetZ = 0;
    if (std::abs(sinYaw) > 0.5) {
        offsetX = static_cast<i32>(sinYaw / std::abs(sinYaw));
    }
    if (std::abs(cosYaw) > 0.5) {
        offsetZ = static_cast<i32>(cosYaw / std::abs(cosYaw));
    }

    // 检查前方一格下方是否是空气
    const BlockPos pandaPos(m_panda->position());
    const BlockPos checkPos(pandaPos.x + offsetX, pandaPos.y - 1, pandaPos.z + offsetZ);

    const BlockState* state = world->getBlockState(checkPos);
    return state != nullptr && state->isAir();
}

// ============================================================================
// PandaSneezeGoal
// ============================================================================

PandaSneezeGoal::PandaSneezeGoal(PandaEntity* panda)
    : m_panda(panda)
{
    // vanilla PandaSneezeGoal 未设 mutexFlags（canContinueToUse=false 一次性触发，由 tick 计时驱动）。
}

bool PandaSneezeGoal::shouldExecute()
{
    // 对齐 vanilla 1.21.11 Panda.PandaSneezeGoal.canUse（Panda.java:1111-1118）：
    //   if (isBaby() && canPerformAction()) {
    //       return isWeak() && random.nextInt(reducedTickDelay(500)) == 1
    //            ? true
    //            : random.nextInt(reducedTickDelay(6000)) == 1;
    //   }
    //   return false;
    if (m_panda == nullptr) {
        return false;
    }

    // 仅幼年熊猫且无其他动作时才考虑打喷嚏
    if (!m_panda->isChild() || !m_panda->canPerformAction()) {
        return false;
    }

    math::Random& rng = m_panda->getRandom();
    // 虚弱性格：1/500 概率（nextInt(reducedTickDelay(500))==1）；否则 1/6000 概率。
    // reducedTickDelay 减半补偿 GoalSelector 每 2 tick 评估一次（对齐 vanilla）。
    // 注：vanilla 用 ==1（非 ==0），原样对齐——仅随机数恰为 1 时触发。
    if (m_panda->isWeak()) {
        return rng.nextInt(reducedTickDelay(WEAK_SNEEZE_CHANCE)) == 1;
    }
    return rng.nextInt(reducedTickDelay(NORMAL_SNEEZE_CHANCE)) == 1;
}

bool PandaSneezeGoal::shouldContinueExecuting()
{
    // 一次性触发：goal 只负责启动 sneeze，由 PandaEntity::tick 的 m_sneezeTimer 递减驱动后续。
    return false;
}

void PandaSneezeGoal::startExecuting()
{
    // 对齐 vanilla PandaSneezeGoal.start：panda.sneeze(true)。
    // sneeze(true) 设 m_sneezing=true + m_sneezeTimer=SNEEZE_DURATION，tick 递减驱动预喷嚏音效
    // （timer==19，对齐 vanilla sneezeCounter==1）与 _onSneezeComplete（timer 到 0，对齐 afterSneeze）。
    m_panda->sneeze(true);
}

} // namespace mc::entity::ai::goal
