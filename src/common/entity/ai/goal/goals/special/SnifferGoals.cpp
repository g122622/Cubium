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

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/PrioritizedGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
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
    if (m_sniffer == nullptr || !m_sniffer->canDig()) {
        return false;
    }
    // 对齐 vanilla Digging 行为：仅在“嗅探目标已确定”（SNIFFER_DIGGING 记忆存在）时启动，
    // 且处于空闲或搜索状态（Searching 结束后同一 tick 内可直接进入挖掘，不打断搜索动画）。
    if (!m_sniffer->hasSniffingTarget()) {
        return false;
    }
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
        // 挖掘结束（对齐 vanilla FinishedDigging.stop(timedOut=true)）：
        //   transitionTo(IDLING).onDiggingComplete(true) → 记录已探索位置；
        //   擦除 SNIFFER_DIGGING 记忆（清除嗅探目标），设置 SNIFFER_HAPPY 记忆。
        m_sniffer->transitionTo(SnifferEntity::State::Rising);
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
        m_sniffer->storeExploredPosition(m_digPos);
        m_sniffer->setSniffingTarget(false);
        m_sniffer->setHappy(true);
        m_digTicks = 0;
        // 对齐 vanilla Digging.stop(timedOut=true)：设置 9600 tick 的嗅探冷却
        // （MemoryModuleType.SNIFF_COOLDOWN），冷却期内不再开启新一轮嗅探。
        m_sniffer->setSniffCooldownTicks(SNIFF_COOLDOWN_TICKS);
        m_runDelay = getRunDelay();
    }
}

void SnifferDigGoal::resetTask()
{
    MoveToBlockGoal::resetTask();
    // 是否在挖掘完成前被打断（对齐 vanilla finishedWork / timedOut 判定，须在清零计数前取）。
    const bool timedOut = m_digTicks >= DIG_TICKS;
    m_digTicks = 0;
    // 中途被其他目标打断（如恐慌）（对齐 vanilla Digging.stop(timedOut=false)）：
    //   resetSniffing → 擦除嗅探目标并回到 IDLING，避免状态机卡死。
    if (m_sniffer != nullptr && m_sniffer->getState() == SnifferEntity::State::Digging) {
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
    }
    if (m_sniffer != nullptr && !timedOut) {
        m_sniffer->setSniffingTarget(false);
    }
}

bool SnifferDigGoal::shouldMoveTo(IWorld* world, const BlockPos& pos)
{
    // 对齐 vanilla Sniffer.canDig(BlockPos)：目标方块（嗅探兽脚下方块）属于
    // `#minecraft:sniffer_diggable_block`，且不在已探索位置记忆（SNIFFER_EXPLORED_POSITIONS）中。
    if (world == nullptr || m_sniffer == nullptr) {
        return false;
    }
    const BlockState* state = world->getBlockState(pos);
    return state != nullptr && BlockTags::SNIFFER_DIGGABLE_BLOCK().contains(*state) &&
        !m_sniffer->isExploredPosition(pos);
}

// ============================================================================
// SnifferScentGoal / SnifferSniffGoal / SnifferHappyGoal
//
// vanilla 用 Brain 的 idle 活动在两个互斥的 one-shot 行为间随机选择：
// Scenting(40..80, 权重 1) 与 Sniffing(40..80, 权重 1)，FeelingHappy(40..100, 权重 2)。
// 本项目 Goal 体系无“按权重随机挑选”，以 getRunDelay() 的错开启动窗口 + 状态门控近似：
// 三个目标仅在 Idling 状态可启动，且同一时刻只能有一个进入执行（状态随之改变）。
// ============================================================================

namespace {

/// 计算 40~80（或 40~100）区间内本次应持续的 tick 数，对齐 vanilla CountingOperation 的闭区间。
i32 pickDuration(IWorld* world, i32 minTicks, i32 maxTicks)
{
    if (world == nullptr || maxTicks <= minTicks) {
        return minTicks;
    }
    return minTicks + world->getRandom().nextInt(maxTicks - minTicks + 1);
}

} // namespace

namespace sniffer {

bool isScenting(SnifferEntity* sniffer)
{
    return sniffer != nullptr && sniffer->getState() == SnifferEntity::State::Scenting;
}

bool isSniffing(SnifferEntity* sniffer)
{
    return sniffer != nullptr && sniffer->getState() == SnifferEntity::State::Sniffing;
}

bool isTempted(SnifferEntity* sniffer)
{
    return sniffer != nullptr && sniffer->temptGoal() != nullptr && sniffer->temptGoal()->isRunning();
}

bool isPanicking(SnifferEntity* sniffer)
{
    return sniffer != nullptr && sniffer->panicGoal() != nullptr && sniffer->panicGoal()->isRunning();
}

} // namespace sniffer

SnifferScentGoal::SnifferScentGoal(SnifferEntity* sniffer)
    : m_sniffer(sniffer)
{}

bool SnifferScentGoal::shouldExecute()
{
    if (m_sniffer == nullptr || m_sniffer->isChild() || m_sniffer->isInWater() || m_sniffer->getVehicle() != 0) {
        return false;
    }
    // 对齐 vanilla Scenting 行为的启动条件：空闲、未被引诱、未恐慌、无嗅探目标、
    // 无开心标记、无繁殖目标（BREED_TARGET 缺席）。
    if (m_sniffer->getState() != SnifferEntity::State::Idling || sniffer::isTempted(m_sniffer) ||
        sniffer::isPanicking(m_sniffer) || m_sniffer->hasSniffingTarget() || m_sniffer->isHappy() ||
        m_sniffer->isInLove()) {
        return false;
    }
    return true;
}

void SnifferScentGoal::startExecuting()
{
    m_ticks = 0;
    IWorld* world = m_sniffer != nullptr ? m_sniffer->world() : nullptr;
    m_goalTicks = pickDuration(world, MIN_TICKS, MAX_TICKS);
    if (m_sniffer != nullptr) {
        // 对齐 vanilla Scenting 行为起始：transitionTo(SCENTING) 并播放闻气味音效。
        m_sniffer->transitionTo(SnifferEntity::State::Scenting);
    }
}

bool SnifferScentGoal::shouldContinueExecuting()
{
    if (m_sniffer == nullptr) {
        return false;
    }
    ++m_ticks;
    return m_ticks < m_goalTicks;
}

void SnifferScentGoal::resetTask()
{
    // 对齐 vanilla Sniffer.onScentingStop：transitionTo(IDLING)。
    if (m_sniffer != nullptr && m_sniffer->getState() == SnifferEntity::State::Scenting) {
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
    }
    m_ticks = 0;
}

SnifferSniffGoal::SnifferSniffGoal(SnifferEntity* sniffer)
    : m_sniffer(sniffer)
{}

bool SnifferSniffGoal::shouldExecute()
{
    // 对齐 vanilla SnifferAi.Sniffing.checkExtraStartConditions：!isBaby() && canSniff()，
    // 并要求 WALK_TARGET / SNIFFER_SNIFFING_TARGET / SNIFF_COOLDOWN 三者均缺席。
    return m_sniffer != nullptr && !m_sniffer->isChild() && m_sniffer->canSniff() &&
        m_sniffer->getState() == SnifferEntity::State::Idling && !m_sniffer->hasSniffingTarget() &&
        !m_sniffer->isSniffCoolingDown() && !m_sniffer->isHappy();
}

void SnifferSniffGoal::startExecuting()
{
    m_ticks = 0;
    IWorld* world = m_sniffer != nullptr ? m_sniffer->world() : nullptr;
    m_goalTicks = pickDuration(world, MIN_TICKS, MAX_TICKS);
    if (m_sniffer != nullptr) {
        // 对齐 vanilla Sniffing 行为起始：transitionTo(SNIFFING) 并播放嗅探音效。
        m_sniffer->transitionTo(SnifferEntity::State::Sniffing);
    }
}

bool SnifferSniffGoal::shouldContinueExecuting()
{
    if (m_sniffer == nullptr) {
        return false;
    }
    if (!m_sniffer->canSniff()) {
        return false;
    }
    ++m_ticks;
    return m_ticks < m_goalTicks;
}

void SnifferSniffGoal::resetTask()
{
    if (m_sniffer == nullptr) {
        m_ticks = 0;
        return;
    }
    // 对齐 vanilla SnifferAi.Sniffing.stop：
    //   transitionTo(IDLING)；若自然结束（timedOut）则记录嗅探目标（对齐设置
    //   SNIFFER_SNIFFING_TARGET 与 SNIFFER_DIGGING 记忆）。本项目由 SnifferSearchGoal
    //   承接随后的搜索段（对应 vanilla SNIFF 活动的 Searching）。
    if (m_sniffer->getState() == SnifferEntity::State::Sniffing) {
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
    }
    const bool timedOut = m_ticks >= m_goalTicks;
    if (timedOut && m_sniffer->canDig()) {
        m_sniffer->setSniffingTarget(true);
    }
    m_ticks = 0;
}

// ============================================================================
// SnifferSearchGoal
// ============================================================================

SnifferSearchGoal::SnifferSearchGoal(SnifferEntity* sniffer)
    : m_sniffer(sniffer)
{}

bool SnifferSearchGoal::shouldExecute()
{
    // 对齐 vanilla SNIFF 活动的激活条件：嗅探目标存在、未恐慌、可嗅探。
    return m_sniffer != nullptr && m_sniffer->hasSniffingTarget() && !sniffer::isPanicking(m_sniffer) &&
        m_sniffer->canSniff();
}

void SnifferSearchGoal::startExecuting()
{
    m_searchTicks = 0;
    if (m_sniffer != nullptr) {
        m_sniffer->transitionTo(SnifferEntity::State::Searching);
    }
}

bool SnifferSearchGoal::shouldContinueExecuting()
{
    if (m_sniffer == nullptr) {
        return false;
    }
    if (!m_sniffer->canSniff()) {
        // 对齐 vanilla Searching.canStillUse：不可嗅探时立即回到 IDLING 并结束。
        if (m_sniffer->getState() == SnifferEntity::State::Searching) {
            m_sniffer->transitionTo(SnifferEntity::State::Idling);
        }
        return false;
    }
    ++m_searchTicks;
    if (m_searchTicks >= MAX_SEARCH_TICKS) {
        return false;
    }
    // 找到可挖掘方块后结束搜索（随后由 SnifferDigGoal 接管，对齐 vanilla 设置 SNIFFER_DIGGING）。
    return !findDigPosition().has_value();
}

void SnifferSearchGoal::tick()
{
    if (m_sniffer == nullptr) {
        return;
    }
    if (!findDigPosition().has_value()) {
        return;
    }
    // 已锁定挖掘点：退出搜索状态，让 SnifferDigGoal 在同一 tick 内接管并前往挖掘。
    if (m_sniffer->getState() == SnifferEntity::State::Searching) {
        m_sniffer->transitionTo(SnifferEntity::State::Idling);
    }
}

void SnifferSearchGoal::resetTask()
{
    if (m_sniffer == nullptr) {
        m_searchTicks = 0;
        return;
    }
    // 对齐 vanilla Searching.stop：可挖掘且仍持有嗅探目标时保留目标供 Digging 使用，否则擦除。
    const bool foundDigSpot = m_sniffer->canDig() && m_sniffer->canSniff() && m_sniffer->hasSniffingTarget() &&
        m_sniffer->getState() != SnifferEntity::State::Searching;
    if (!foundDigSpot) {
        m_sniffer->setSniffingTarget(false);
    }
    m_searchTicks = 0;
}

std::optional<BlockPos> SnifferSearchGoal::findDigPosition() const
{
    if (m_sniffer == nullptr || m_sniffer->world() == nullptr) {
        return std::nullopt;
    }
    IWorld* world = m_sniffer->world();
    const BlockPos origin = m_sniffer->onPos();
    // 对齐 vanilla Sniffer.calculateDigPosition：尝试 5 次、搜索长度依次为 10/12/14/16/18 的取点。
    // 本项目以确定性的自近及远、自下而上扫描替代随机取点，语义等价（定位最近的未探索可挖掘方块）。
    for (i32 step = 0; step < 5; ++step) {
        const i32 radius = SEARCH_LENGTH + 2 * step;
        for (i32 dy = -3; dy <= 1; ++dy) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                for (i32 dz = -radius; dz <= radius; ++dz) {
                    const BlockPos pos(origin.x + dx, origin.y + dy, origin.z + dz);
                    const BlockState* state = world->getBlockState(pos);
                    if (state == nullptr || !BlockTags::SNIFFER_DIGGABLE_BLOCK().contains(*state)) {
                        continue;
                    }
                    if (m_sniffer->isExploredPosition(pos)) {
                        continue;
                    }
                    return pos;
                }
            }
        }
    }
    return std::nullopt;
}

SnifferHappyGoal::SnifferHappyGoal(SnifferEntity* sniffer)
    : m_sniffer(sniffer)
{}

bool SnifferHappyGoal::shouldExecute()
{
    if (m_sniffer == nullptr) {
        return false;
    }
    // 对齐 vanilla FeelingHappy：仅繁殖后（Sniffer.setHappy）且处于空闲时执行一次。
    return m_sniffer->isHappy() && m_sniffer->getState() == SnifferEntity::State::Idling;
}

void SnifferHappyGoal::startExecuting()
{
    m_ticks = 0;
    IWorld* world = m_sniffer != nullptr ? m_sniffer->world() : nullptr;
    m_goalTicks = pickDuration(world, MIN_TICKS, MAX_TICKS);
    if (m_sniffer != nullptr) {
        m_sniffer->transitionTo(SnifferEntity::State::FeelingHappy);
    }
}

bool SnifferHappyGoal::shouldContinueExecuting()
{
    if (m_sniffer == nullptr) {
        return false;
    }
    ++m_ticks;
    return m_ticks < m_goalTicks;
}

void SnifferHappyGoal::resetTask()
{
    if (m_sniffer != nullptr) {
        if (m_sniffer->getState() == SnifferEntity::State::FeelingHappy) {
            m_sniffer->transitionTo(SnifferEntity::State::Idling);
        }
        // 开心标记只消费一次（对齐 vanilla setHappy(false)）。
        m_sniffer->setHappy(false);
    }
    m_ticks = 0;
}

} // namespace mc::entity::ai::goal
