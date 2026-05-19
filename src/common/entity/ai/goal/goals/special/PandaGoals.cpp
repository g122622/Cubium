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
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../entities/passive/special/PandaEntity.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// PandaRollGoal
// ============================================================================

PandaRollGoal::PandaRollGoal(PandaEntity* panda)
    : m_panda(panda)
{
    // MC 1.16.5: setMutexFlags(EnumSet.of(Goal.Flag.MOVE, Goal.Flag.LOOK, Goal.Flag.JUMP))
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
}

bool PandaRollGoal::shouldExecute()
{
    if (m_panda == nullptr) {
        return false;
    }

    // MC 1.16.5: 条件1 - 幼年或顽皮性格
    // if ((this.panda.isChild() || this.panda.isPlayful()) && this.panda.onGround)
    if (!m_panda->isChild() && !m_panda->isPlayful()) {
        return false;
    }

    // 必须在地面
    if (!m_panda->onGround()) {
        return false;
    }

    // MC 1.16.5: 条件2 - canPerformAction()
    // 检查熊猫是否可以执行动作（不在打喷嚏、吃东西、躺着、打滚等状态）
    if (!m_panda->canPerformAction()) {
        return false;
    }

    // MC 1.16.5: 条件3 - 检查前方是否有悬崖或概率触发
    // MC 1.16.5: 概率检查
    math::Random rng = m_panda->getRandom();
    if (isCliffInFront()) {
        // MC 1.16.5: 如果前方是悬崖，100% 触发
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
    // MC 1.16.5: return false
    // 打滚是一次性动作，由 rollCounter 控制持续时间
    // Goal 只负责触发，PandaEntity::updateRoll() 负责物理更新
    return false;
}

void PandaRollGoal::startExecuting()
{
    // MC 1.16.5: this.panda.func_213576_v(true)
    // 设置打滚状态，由 tick() 中的 updateRoll() 处理物理
    m_panda->setRolling(true);
    // 初始设置为 0，由 updateRoll() 递增
    m_panda->setRollTimer(0);
}

bool PandaRollGoal::isCliffInFront() const
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

    // MC 1.16.5: 计算前方一格的偏移
    // if ((double)Math.abs(f1) > 0.5D) { i = (int)((float)i + f1 / Math.abs(f1)); }
    // if ((double)Math.abs(f2) > 0.5D) { j = (int)((float)j + f2 / Math.abs(f2)); }
    i32 offsetX = 0;
    i32 offsetZ = 0;
    if (std::abs(sinYaw) > 0.5) {
        offsetX = static_cast<i32>(sinYaw / std::abs(sinYaw));
    }
    if (std::abs(cosYaw) > 0.5) {
        offsetZ = static_cast<i32>(cosYaw / std::abs(cosYaw));
    }

    // MC 1.16.5: 检查前方一格下方是否是空气
    // this.panda.world.getBlockState(this.panda.getPosition().add(i, -1, j)).isAir()
    const BlockPos pandaPos(m_panda->position());
    const BlockPos checkPos(pandaPos.x + offsetX, pandaPos.y - 1, pandaPos.z + offsetZ);

    const BlockState* state = world->getBlockState(checkPos);
    return state != nullptr && state->isAir();
}

} // namespace mc::entity::ai::goal
