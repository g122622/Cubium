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

#include "CamelSitGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/entities/passive/special/CamelEntity.hpp"

namespace mc::entity::ai::goal {

CamelSitGoal::CamelSitGoal(CamelEntity* camel)
    : m_camel(camel)
{
    // 坐下/站起不应被漫步目标打断，占用 Move。
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool CamelSitGoal::shouldExecute()
{
    if (m_camel == nullptr || !m_camel->isAlive()) {
        return false;
    }
    // 对齐 vanilla RandomSitting.checkExtraStartConditions。
    if (m_camel->isInWater()) {
        return false;
    }
    // IDLE_MINIMAL_DURATION_TICKS = 80 tick = 4 秒（vanilla RandomSitting(20) 的入参为秒）。
    if (m_camel->getPoseTime() < CamelEntity::IDLE_MINIMAL_DURATION_TICKS) {
        return false;
    }
    if (m_camel->isLeashed()) {
        return false;
    }
    if (!m_camel->onGround()) {
        return false;
    }
    if (m_camel->hasControllingPassenger()) {
        return false;
    }
    return true;
}

void CamelSitGoal::startExecuting()
{
    if (m_camel == nullptr) {
        return;
    }
    // 对齐 vanilla RandomSitting.start：坐着则站起；否则（不在恐慌中）坐下。
    if (m_camel->isCamelSitting()) {
        m_camel->standUp();
    } else if (!m_camel->isPanicking()) {
        m_camel->sitDown();
    }
}

void CamelSitGoal::resetTask() {}

} // namespace mc::entity::ai::goal
