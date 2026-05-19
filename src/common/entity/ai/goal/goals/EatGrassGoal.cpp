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
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/WorldEvents.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/VanillaBlocks.hpp"
#include "../../../../world/gamerule/GameRules.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"

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

    // MC 1.16.5: 概率检查
    // 幼年动物 1/50，成年动物 1/1000
    math::Random rng = m_mob->getRandom();
    const i32 chance = m_isChild && m_isChild() ? CHILD_CHANCE : ADULT_CHANCE;
    if (rng.nextInt(chance) != 0) {
        return false;
    }

    // MC 1.16.5: 检查当前位置或下方是否有草
    m_world = m_mob->world();
    const BlockPos entityPos(m_mob->position());

    // 检查当前位置（草）
    if (isGrassAt(m_world, entityPos)) {
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

    // MC 1.16.5: 清除导航路径
    if (m_mob) {
        if (auto* nav = m_mob->navigator()) {
            nav->clearPath();
        }
    }

    // MC 1.16.5: 发送动画状态给客户端
    // 在原版中通过 world.setEntityState(entity, (byte)10) 实现
    // 这里我们暂时跳过客户端同步，因为需要网络系统支持
}

void EatGrassGoal::resetTask()
{
    m_eatingGrassTimer = 0;
}

void EatGrassGoal::tick()
{
    // MC 1.16.5: 递减计时器
    m_eatingGrassTimer = std::max(0, m_eatingGrassTimer - 1);

    // MC 1.16.5: 在第 4 tick 时执行吃草动作
    if (m_eatingGrassTimer == EAT_TICK) {
        eatGrass();
    }
}

bool EatGrassGoal::isGrassAt(IWorld* world, const BlockPos& pos) const
{
    if (!world) {
        return false;
    }

    const BlockState* state = world->getBlockState(pos);
    if (!state) {
        return false;
    }

    // MC 1.16.5: 检查是否为草（草丛/高草丛）
    // 原版使用 BlockStateMatcher.forBlock(Blocks.GRASS)
    return state->is(VanillaBlocks::SHORT_GRASS) || state->is(VanillaBlocks::TALL_GRASS);
}

void EatGrassGoal::eatGrass()
{
    if (!m_world || !m_mob) {
        return;
    }

    // MC 1.16.5: 检查 mobGriefing 游戏规则
    // 参考: net.minecraft.entity.ai.goal.EatGrassGoal.tick()
    // 原版等效: world.getGameRules().getBoolean(GameRules.MOB_GRIEFING)
    const bool canGrief = m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);

    if (m_isEatingGrassBlock) {
        // 草方块 -> 泥土
        const BlockState* currentState = m_world->getBlockState(m_targetPos);
        if (currentState != nullptr && currentState->is(VanillaBlocks::GRASS_BLOCK)) {
            if (canGrief) {
                // MC 1.16.5: world.playEvent(2001, pos, Block.getStateId(Blocks.GRASS_BLOCK.getDefaultState()))
                // 播放方块破坏效果（粒子 + 音效）
                m_world->playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS,
                    m_targetPos,
                    static_cast<i32>(VanillaBlocks::GRASS_BLOCK->defaultState().stateId()));

                // MC 1.16.5: world.setBlockState(pos, Blocks.DIRT.getDefaultState(), 2)
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
                // MC 1.16.5: world.destroyBlock(pos, false) - 不掉落物品
                const BlockState* airState = &VanillaBlocks::AIR->defaultState();
                m_world->setBlockState(m_targetPos, airState, 2);
            }
        }
    }

    // MC 1.16.5: 无论 mobGriefing 是否允许，都调用 eatGrassBonus
    // 这是因为羊吃草后会获得饱食度和重新长毛
    if (m_onEatGrass) {
        m_onEatGrass();
    }
}

} // namespace mc::entity::ai::goal
