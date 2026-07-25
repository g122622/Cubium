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

#include "EatGrassGoal.hpp"

#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

namespace mc::entity::ai::goal {

EatGrassGoal::EatGrassGoal(MobEntity* mob, EatGrassCallback onEatGrass, IsChildCallback isChild)
    : m_mob(mob)
    , m_onEatGrass(std::move(onEatGrass))
    , m_isChild(std::move(isChild))
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
}

bool EatGrassGoal::shouldExecute()
{
    if (!m_mob || !m_mob->world()) {
        return false;
    }

    // 概率检查：幼年动物 1/50，成年动物 1/1000
    math::Random& rng = m_mob->getRandom();
    const i32 chance = m_isChild && m_isChild() ? CHILD_CHANCE : ADULT_CHANCE;
    if (rng.nextInt(chance) != 0) {
        return false;
    }

    // 检查当前位置或下方是否有草
    m_world = m_mob->world();
    const BlockPos entityPos(m_mob->position());

    // 检查当前位置（草）
    if (_isGrassAt(m_world, entityPos)) {
        m_targetPos = entityPos;
        m_isEatingGrassBlock = false; // 是草
        return true;
    }

    // 检查下方位置（草方块）
    const BlockPos belowPos = entityPos.down();
    if (m_world->getBlockState(belowPos) != nullptr &&
        m_world->getBlockState(belowPos)->is(VanillaBlocks::GRASS_BLOCK)) {
        m_targetPos = belowPos;
        m_isEatingGrassBlock = true; // 是草方块
        return true;
    }

    return false;
}

bool EatGrassGoal::shouldContinueExecuting()
{
    return m_eatingGrassTimer > 0;
}

void EatGrassGoal::startExecuting()
{
    m_eatingGrassTimer = EAT_DURATION;

    // 清除导航路径
    if (m_mob) {
        if (auto* nav = m_mob->navigator()) {
            nav->clearPath();
        }
    }

    // 通知客户端开始吃草动画
    if (m_mob && m_mob->world()) {
        m_mob->world()->broadcastEntityStatus(m_mob->id(), static_cast<u8>(network::EntityStatus::EatBlock));
    }
}

void EatGrassGoal::resetTask()
{
    m_eatingGrassTimer = 0;
}

void EatGrassGoal::tick()
{
    // 递减计时器
    m_eatingGrassTimer = std::max(0, m_eatingGrassTimer - 1);

    // 在第 4 tick 时执行吃草动作
    if (m_eatingGrassTimer == EAT_TICK) {
        _eatGrass();
    }
}

bool EatGrassGoal::_isGrassAt(IWorld* world, const BlockPos& pos) const
{
    if (!world) {
        return false;
    }

    const BlockState* state = world->getBlockState(pos);
    if (!state) {
        return false;
    }

    // 检查是否为草（草丛/高草丛）
    return state->is(VanillaBlocks::SHORT_GRASS) || state->is(VanillaBlocks::TALL_GRASS);
}

void EatGrassGoal::_eatGrass()
{
    if (!m_world || !m_mob) {
        return;
    }

    // 检查 mobGriefing 游戏规则
    const bool canGrief = m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);

    if (m_isEatingGrassBlock) {
        // 草方块 -> 泥土
        const BlockState* currentState = m_world->getBlockState(m_targetPos);
        if (currentState != nullptr && currentState->is(VanillaBlocks::GRASS_BLOCK)) {
            if (canGrief) {
                // 播放方块破坏效果（粒子 + 音效）
                m_world->playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS,
                    m_targetPos,
                    static_cast<i32>(VanillaBlocks::GRASS_BLOCK->defaultState().stateId()));

                // 设置为泥土，flags=2 表示通知邻居并同步客户端
                const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
                m_world->setBlockState(m_targetPos, dirtState, 2);
            }
        }
    } else {
        // 草（草丛/高草丛） -> 空气
        const BlockState* currentState = m_world->getBlockState(m_targetPos);
        if (currentState != nullptr &&
            (currentState->is(VanillaBlocks::SHORT_GRASS) || currentState->is(VanillaBlocks::TALL_GRASS))) {
            if (canGrief) {
                // 不掉落物品，直接移除
                const BlockState* airState = &VanillaBlocks::AIR->defaultState();
                m_world->setBlockState(m_targetPos, airState, 2);
            }
        }
    }

    // 无论 mobGriefing 是否允许，都调用吃草回调
    // 羊吃草后会获得饱食度和重新长毛
    if (m_onEatGrass) {
        m_onEatGrass();
    }
}

} // namespace mc::entity::ai::goal
