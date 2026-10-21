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

#include "SnifferGoals.hpp"

#include "common/entity/ai/goal/goals/special/MoveToBlockGoal.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc::entity::ai::goal {

SnifferDigGoal::SnifferDigGoal(SnifferEntity* sniffer, f64 speed)
    : MoveToBlockGoal(sniffer, speed, SEARCH_LENGTH)
    , m_sniffer(sniffer)
{}

bool SnifferDigGoal::shouldExecute()
{
    // 对齐 vanilla Sniffer.canDig：恐慌/被引诱/幼年/在水中/空中/被骑乘时都不挖掘。
    if (m_sniffer == nullptr || m_sniffer->isChild() || m_sniffer->isInWater() || !m_sniffer->onGround() ||
        m_sniffer->getVehicle() != 0) {
        return false;
    }
    // 嗅探兽空闲时才寻找挖掘点：正在闻气味/嗅探/搜索/挖掘/抬头期间不打断。
    if (m_sniffer->getState() != SnifferEntity::State::Idling &&
        m_sniffer->getState() != SnifferEntity::State::Searching) {
        return false;
    }
    return MoveToBlockGoal::shouldExecute();
}

void SnifferDigGoal::startExecuting()
{
    MoveToBlockGoal::startExecuting();
    m_digTicks = 0;
    m_digPos = BlockPos();
}

bool SnifferDigGoal::shouldContinueExecuting()
{
    // 到达目标后原地挖掘 DIG_TICKS，期间不因寻路结束而中断；掉种由实体 tick 负责。
    if (m_isAboveDestination) {
        return m_digTicks < DIG_TICKS;
    }
    return MoveToBlockGoal::shouldContinueExecuting();
}

void SnifferDigGoal::tick()
{
    if (m_sniffer == nullptr) {
        return;
    }

    MoveToBlockGoal::tick();
    if (!m_isAboveDestination) {
        return;
    }

    // 首次到达：把目标方块（其上方为站立方块）记为目标并进入 Digging 状态，
    // 由 SnifferEntity::transitionTo(Digging) 设置掉种 tick（tickCount + 120）。
    const BlockPos targetPos = m_destinationBlock;
    if (m_digTicks == 0) {
        m_digPos = targetPos;
        m_sniffer->transitionTo(SnifferEntity::State::Digging);
    }
    ++m_digTicks;

    if (m_digTicks >= DIG_TICKS) {
        // 挖掘结束：抬头回到空闲，等待下一轮嗅探。
        m_sniffer->transitionTo(SnifferEntity::State::Rising);
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
        m_digTicks = 0;
        m_runDelay = getRunDelay();
    }
}

void SnifferDigGoal::resetTask()
{
    MoveToBlockGoal::resetTask();
    m_digTicks = 0;
    // 中途被其他目标打断（如恐慌）：若仍处于挖掘状态则回到空闲，避免状态机卡死。
    if (m_sniffer != nullptr && m_sniffer->getState() == SnifferEntity::State::Digging) {
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
    }
}

bool SnifferDigGoal::shouldMoveTo(IWorld* world, const BlockPos& pos)
{
    // 对齐 vanilla Sniffer.canDig(BlockPos)：目标方块（嗅探兽脚下方块）属于
    // `#minecraft:sniffer_diggable_block` 时才可挖掘。
    if (world == nullptr) {
        return false;
    }
    const BlockState* state = world->getBlockState(pos);
    return state != nullptr && BlockTags::SNIFFER_DIGGABLE_BLOCK().contains(*state);
}

} // namespace mc::entity::ai::goal