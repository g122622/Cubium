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

#include "RaidGardenGoal.hpp"

#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/goals/special/MoveToBlockGoal.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/agricultural/CarrotBlock.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"

namespace mc::entity::ai::goal {

using namespace RaidGardenGoalConstants;

// ============================================================================
// 构造函数
// ============================================================================

RaidGardenGoal::RaidGardenGoal(RabbitEntity* rabbit)
    : MoveToBlockGoal(rabbit, MOVE_SPEED, SEARCH_LENGTH, VERTICAL_SEARCH_RANGE)
    , m_rabbit(rabbit)
{
    MC_ASSERT(rabbit != nullptr);
}

// ============================================================================
// shouldExecute（对应 MC Rabbit.RaidGardenGoal.canUse）
// ============================================================================

bool RaidGardenGoal::shouldExecute()
{
    if (m_rabbit == nullptr || m_creature == nullptr) {
        return false;
    }

    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return false;
    }

    // 对应 MC：if (this.nextStartTick <= 0) { ... }
    // MoveToBlockGoal::shouldExecute() 在 m_runDelay > 0 时递减并返回 false，
    // 当 m_runDelay == 0 时进入搜索分支。此处复用 m_runDelay 作为 nextStartTick。
    if (m_runDelay <= 0) {
        // 对应 MC：if (!getServerLevel(rabbit).getGameRules().get(GameRules.MOB_GRIEFING)) return false;
        // 注意：MC 在此直接返回 false，不重置 nextStartTick，因此下一 tick 仍会重新检查
        // mobGriefing。项目中等效：不调用基类 shouldExecute()（避免重置 m_runDelay），直接返回 false。
        if (!world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
            return false;
        }

        // 对应 MC：this.canRaid = false; this.wantsToRaid = this.rabbit.wantsMoreFood();
        m_canRaid = false;
        m_wantsToRaid = m_rabbit->wantsMoreFood();
    }

    // 对应 MC：return super.canUse();
    return MoveToBlockGoal::shouldExecute();
}

// ============================================================================
// shouldContinueExecuting（对应 MC Rabbit.RaidGardenGoal.canContinueToUse）
// ============================================================================

bool RaidGardenGoal::shouldContinueExecuting()
{
    // 对应 MC：return this.canRaid && super.canContinueToUse();
    // 注意：canRaid 在 isValidTarget/shouldMoveTo 中检测到成熟胡萝卜时设为 true。
    // 若 canRaid 为 false（如目标上方不是胡萝卜），即使路径未完成也提前结束。
    return m_canRaid && MoveToBlockGoal::shouldContinueExecuting();
}

// ============================================================================
// tick（对应 MC Rabbit.RaidGardenGoal.tick）
// ============================================================================

void RaidGardenGoal::tick()
{
    if (m_rabbit == nullptr || m_creature == nullptr) {
        return;
    }

    // 对应 MC：super.tick();
    MoveToBlockGoal::tick();

    // 对应 MC：setLookAt(blockPos.x+0.5, blockPos.y+1, blockPos.z+0.5, 10.0F, getMaxHeadXRot())
    // 朝向胡萝卜方块（耕地正上方）以产生低头啃食动画
    if (auto* mob = dynamic_cast<MobEntity*>(m_creature); mob != nullptr) {
        if (auto* lookControl = mob->lookController(); lookControl != nullptr) {
            lookControl->setLookPosition(static_cast<f64>(m_destinationBlock.x) + 0.5,
                static_cast<f64>(m_destinationBlock.y + 1),
                static_cast<f64>(m_destinationBlock.z) + 0.5,
                LOOK_DELTA_YAW,
                LOOK_DELTA_YAW);
        }
    }

    // 对应 MC：if (this.isReachedTarget()) { ... }
    // MoveToBlockGoal::tick() 中到达目标时设置 m_isAboveDestination = true
    if (!m_isAboveDestination) {
        return;
    }

    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return;
    }

    // 对应 MC：BlockPos blockpos = this.blockPos.above();
    const BlockPos carrotPos = m_destinationBlock.up();

    // 对应 MC：BlockState blockstate = level.getBlockState(blockpos);
    const BlockState* carrotState = world->getBlockState(carrotPos);
    if (carrotState == nullptr) {
        // 目标方块已不存在，结束本次掠夺
        m_canRaid = false;
        m_runDelay = NEXT_START_TICK;
        return;
    }

    // 对应 MC：Block block = blockstate.getBlock();
    //         if (this.canRaid && block instanceof CarrotBlock) { ... }
    if (m_canRaid && carrotState->is(VanillaBlocks::CARROTS)) {
        _raidCarrot(carrotPos, carrotState);
    }

    // 对应 MC：this.canRaid = false; this.nextStartTick = 10;
    m_canRaid = false;
    m_runDelay = NEXT_START_TICK;
}

// ============================================================================
// shouldMoveTo（对应 MC Rabbit.RaidGardenGoal.isValidTarget）
// ============================================================================

bool RaidGardenGoal::shouldMoveTo(IWorld* world, const BlockPos& pos)
{
    if (world == nullptr) {
        return false;
    }

    // 边界检查（pos.up() 需要访问上方方块，确保 y+1 在世界范围内）
    if (!world->isWithinWorldBounds(pos.x, pos.y, pos.z) || !world->isWithinWorldBounds(pos.x, pos.y + 1, pos.z)) {
        return false;
    }

    // 对应 MC：BlockState blockstate = p_479418_.getBlockState(p_479817_);
    const BlockState* groundState = world->getBlockState(pos);
    if (groundState == nullptr) {
        return false;
    }

    // 对应 MC：if (blockstate.is(Blocks.FARMLAND) && this.wantsToRaid && !this.canRaid) { ... }
    if (!groundState->is(VanillaBlocks::FARMLAND) || !m_wantsToRaid || m_canRaid) {
        return false;
    }

    // 对应 MC：blockstate = p_479418_.getBlockState(p_479817_.above());
    const BlockState* aboveState = world->getBlockState(pos.up());
    if (aboveState == nullptr) {
        return false;
    }

    // 对应 MC：if (blockstate.getBlock() instanceof CarrotBlock &&
    //         ((CarrotBlock)blockstate.getBlock()).isMaxAge(blockstate)) {
    //     this.canRaid = true; return true;
    // }
    if (!aboveState->is(VanillaBlocks::CARROTS)) {
        return false;
    }

    auto* carrotBlock = dynamic_cast<const blocks::CarrotBlock*>(VanillaBlocks::CARROTS);
    if (carrotBlock == nullptr) {
        return false;
    }

    if (!carrotBlock->isMaxAge(*aboveState)) {
        return false;
    }

    m_canRaid = true;
    return true;
}

// ============================================================================
// _raidCarrot（对应 MC Rabbit.RaidGardenGoal.tick() 中的掠夺分支）
// ============================================================================

void RaidGardenGoal::_raidCarrot(const BlockPos& carrotPos, const BlockState* carrotState)
{
    if (m_rabbit == nullptr || m_creature == nullptr || carrotState == nullptr) {
        return;
    }

    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return;
    }

    // 对应 MC：int i = blockstate.getValue(CarrotBlock.AGE);
    auto* carrotBlock = dynamic_cast<const blocks::CropBlock*>(VanillaBlocks::CARROTS);
    if (carrotBlock == nullptr) {
        return;
    }
    const i32 age = carrotBlock->getAge(*carrotState);

    if (age == 0) {
        // 对应 MC：level.setBlock(blockpos, Blocks.AIR.defaultBlockState(), 2);
        //          level.destroyBlock(blockpos, true, this.rabbit);
        // MC 中先设为 AIR 再调用 destroyBlock，但 destroyBlock 看到 AIR 直接返回（空操作），
        // 因此 AGE==0 的胡萝卜直接消失，无粒子/音效/掉落。此处忠实复刻此行为。
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        world->setBlockState(carrotPos, airState, 2);
    } else {
        // 对应 MC：level.setBlock(blockpos, blockstate.setValue(CarrotBlock.AGE, i - 1), 2);
        const BlockState& newState = carrotBlock->withAge(age - 1);
        world->setBlockState(carrotPos, &newState, 2);

        // 对应 MC：level.gameEvent(GameEvent.BLOCK_CHANGE, blockpos, GameEvent.Context.of(this.rabbit));
        world->gameEvent(gameevent::GameEvents::BLOCK_CHANGE, carrotPos, gameevent::GameEvent::Context::of(m_rabbit));

        // 对应 MC：level.levelEvent(2001, blockpos, Block.getId(blockstate));
        // levelEvent(2001) 的 data 参数为方块状态 ID（Block.getId(blockstate)）
        world->playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, carrotPos, static_cast<i32>(carrotState->stateId()));
    }

    // 对应 MC：this.rabbit.moreCarrotTicks = 40;
    m_rabbit->setMoreCarrotTicks(MORE_CARROTS_DELAY);
}

} // namespace mc::entity::ai::goal
