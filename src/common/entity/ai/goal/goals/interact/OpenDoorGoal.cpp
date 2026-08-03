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

#include "OpenDoorGoal.hpp"

#include "common/entity/ai/goal/goals/interact/DoorInteractGoal.hpp"
#include "common/entity/core/MobEntity.hpp"

namespace mc::entity::ai::goal {

OpenDoorGoal::OpenDoorGoal(MobEntity* mob, bool closeDoor)
    : DoorInteractGoal(mob)
    , m_closeDoor(closeDoor)
{}

bool OpenDoorGoal::shouldContinueExecuting()
{
    // 只有关门模式下才需要继续执行（等待忘记时间耗尽或穿过门）
    // 不关门模式下，开门后立即结束（shouldContinueExecuting 返回 false）
    return m_closeDoor && m_forgetTime > 0 && DoorInteractGoal::shouldContinueExecuting();
}

void OpenDoorGoal::startExecuting()
{
    DoorInteractGoal::startExecuting();

    // 设置忘记时间（关门倒计时），20 tick 后目标结束并关门
    m_forgetTime = 20;

    // 打开门
    _setDoorOpen(true);
}

void OpenDoorGoal::resetTask()
{
    DoorInteractGoal::resetTask();

    // 关闭门（无论是关门模式还是不关门模式，目标结束时都关门）
    // 不关门模式下，由于 shouldContinueExecuting 返回 false，
    // 目标在 startExecuting 之后的第一个 tick 就会结束并调用 resetTask
    _setDoorOpen(false);
}

void OpenDoorGoal::tick()
{
    // 递减忘记时间
    m_forgetTime--;

    // 调用父类 tick 检测实体是否穿过门
    DoorInteractGoal::tick();
}

} // namespace mc::entity::ai::goal
