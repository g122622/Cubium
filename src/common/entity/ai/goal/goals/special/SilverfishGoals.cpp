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

#include "SilverfishGoals.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "entity/ai/pathfinding/PathNavigator.hpp"
#include "util/Direction.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/mob/InfestedBlock.hpp"
#include "world/gamerule/GameRules.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// SilverfishHideInStoneGoal 实现
// ============================================================================

SilverfishHideInStoneGoal::SilverfishHideInStoneGoal(SilverfishEntity* silverfish)
    : RandomWalkingGoal(silverfish, 1.0, 10)
    , m_silverfish(silverfish)
    , m_facing(Direction::None)
    , m_doMerge(false)
{
    // MC 1.16.5: setMutexFlags(EnumSet.of(Goal.Flag.MOVE))
    // RandomWalkingGoal 已经设置了 MOVE 标志
}

bool SilverfishHideInStoneGoal::shouldExecute()
{
    // MC 1.16.5: 如果有攻击目标，不执行
    if (m_creature->attackTarget() != nullptr) {
        return false;
    }

    // MC 1.16.5: 如果导航器有路径，不执行
    auto* nav = m_creature->navigator();
    if (nav != nullptr && !nav->noPath()) {
        return false;
    }

    // 获取世界和随机数
    IWorld* world = m_silverfish->world();
    if (world == nullptr) {
        return false;
    }

    math::Random rng = m_silverfish->getRandom();

    // MC 1.16.5: 检查 mobGriefing 游戏规则 && 1/10 概率
    if (world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
        && rng.nextInt(MERGE_CHANCE) == 0) {
        // 随机选择一个方向
        auto allDirs = Directions::all();
        m_facing = allDirs[rng.nextInt(static_cast<i32>(allDirs.size()))];

        // 计算检查位置（蠹虫位置 + 0.5 高度偏移 + 方向偏移）
        BlockPos checkPos(
            static_cast<BlockCoord>(std::floor(m_silverfish->x())),
            static_cast<BlockCoord>(std::floor(m_silverfish->y() + 0.5)),
            static_cast<BlockCoord>(std::floor(m_silverfish->z())));
        checkPos = checkPos.offset(m_facing);

        // 检查是否是可以被虫蚀的方块（STONE, COBBLESTONE, STONE_BRICKS 等）
        const BlockState* state = world->getBlockState(checkPos);
        if (state != nullptr) {
            const Block& block = state->getBlock();
            // canContainSilverfish 检查方块是否有对应的虫蚀版本
            if (blocks::InfestedBlock::infest(block) != nullptr) {
                m_doMerge = true;
                return true;
            }
        }
    }

    // 否则执行普通的随机行走
    m_doMerge = false;
    return RandomWalkingGoal::shouldExecute();
}

bool SilverfishHideInStoneGoal::shouldContinueExecuting()
{
    // MC 1.16.5: 如果正在藏入，不继续（立即完成）
    if (m_doMerge) {
        return false;
    }
    return RandomWalkingGoal::shouldContinueExecuting();
}

void SilverfishHideInStoneGoal::startExecuting()
{
    if (m_doMerge) {
        // MC 1.16.5: 执行藏入石头的逻辑
        IWorld* world = m_silverfish->world();
        if (world == nullptr) {
            return;
        }

        // 计算目标位置
        BlockPos targetPos(
            static_cast<BlockCoord>(std::floor(m_silverfish->x())),
            static_cast<BlockCoord>(std::floor(m_silverfish->y() + 0.5)),
            static_cast<BlockCoord>(std::floor(m_silverfish->z())));
        targetPos = targetPos.offset(m_facing);

        const BlockState* state = world->getBlockState(targetPos);
        if (state != nullptr) {
            const Block& block = state->getBlock();
            // 检查是否是可以被虫蚀的普通方块（STONE, COBBLESTONE, STONE_BRICKS 等）
            const BlockState* infestedState = blocks::InfestedBlock::infest(block);
            if (infestedState != nullptr) {
                // MC 1.16.5: 将普通方块转换为虫蚀方块
                // iworld.setBlockState(blockpos, SilverfishBlock.infest(blockstate.getBlock()), 3)
                world->setBlockState(targetPos, infestedState, 3);

                // MC 1.16.5: 生成消散粒子效果
                // silverfishentity.spawnExplosionParticle()
                // 注：粒子效果会在实体移除时由客户端自动处理
                // 由于此文件在 common 模块，无法直接包含客户端头文件

                // 移除蠹虫实体
                m_silverfish->remove();
            }
        }
    } else {
        // 执行普通的随机行走
        RandomWalkingGoal::startExecuting();
    }
}

// ============================================================================
// SilverfishSummonOthersGoal 实现
// ============================================================================

SilverfishSummonOthersGoal::SilverfishSummonOthersGoal(SilverfishEntity* silverfish)
    : m_silverfish(silverfish)
    , m_lookForFriends(0)
{
    // 无互斥标志
}

void SilverfishSummonOthersGoal::notifyHurt()
{
    // MC 1.16.5: if (this.lookForFriends == 0) { this.lookForFriends = 20; }
    if (m_lookForFriends == 0) {
        m_lookForFriends = SUMMON_DURATION;
    }
}

bool SilverfishSummonOthersGoal::shouldExecute()
{
    // MC 1.16.5: return this.lookForFriends > 0
    return m_lookForFriends > 0;
}

void SilverfishSummonOthersGoal::tick()
{
    // MC 1.16.5: --this.lookForFriends
    --m_lookForFriends;

    if (m_lookForFriends <= 0) {
        IWorld* world = m_silverfish->world();
        if (world == nullptr) {
            return;
        }

        math::Random rng = m_silverfish->getRandom();
        BlockPos centerPos(m_silverfish->position());

        // MC 1.16.5: 遍历周围区域
        // for(int i = 0; i <= 5 && i >= -5; i = (i <= 0 ? 1 : 0) - i)
        // 这是一个特殊的遍历顺序：0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5
        // Y轴范围：-5 到 5
        // X/Z轴范围：-10 到 10

        for (i32 dy = 0; dy <= 5 || -dy <= 5; dy = (dy <= 0) ? 1 - dy : -dy) {
            for (i32 dx = 0; dx <= 10 || -dx <= 10; dx = (dx <= 0) ? 1 - dx : -dx) {
                for (i32 dz = 0; dz <= 10 || -dz <= 10; dz = (dz <= 0) ? 1 - dz : -dz) {
                    if (std::abs(dy) > 5) goto endYLoop;
                    if (std::abs(dx) > 10) goto endXLoop;
                    if (std::abs(dz) > 10) goto endZLoop;

                    BlockPos checkPos(centerPos.x + dx, centerPos.y + dy, centerPos.z + dz);
                    const BlockState* state = world->getBlockState(checkPos);

                    if (state != nullptr) {
                        const Block& block = state->getBlock();
                        // 检查是否是虫蚀方块（InfestedBlock 类型）
                        const blocks::InfestedBlock* infestedBlock =
                            dynamic_cast<const blocks::InfestedBlock*>(&block);

                        if (infestedBlock != nullptr) {
                            // 检查 mobGriefing 游戏规则
                            if (world->getGameRules().getBoolean(
                                    world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
                                // 破坏方块（会生成蠹虫）
                                // MC 1.16.5: world.destroyBlock(blockpos1, true, this.silverfish)
                                const BlockState* airState = BlockRegistry::instance().airState();
                                if (airState != nullptr) {
                                    world->setBlockState(checkPos, airState, 3);
                                    // onBlockRemoved 会在 InfestedBlock 中自动生成蠹虫
                                }
                            } else {
                                // 转换为原版方块
                                // MC 1.16.5: world.setBlockState(blockpos1, ((SilverfishBlock)block).getMimickedBlock().getDefaultState(), 3)
                                u32 hostBlockId = infestedBlock->getHostBlock();
                                const Block* hostBlock = BlockRegistry::instance().getBlock(hostBlockId);
                                if (hostBlock != nullptr) {
                                    world->setBlockState(checkPos, &hostBlock->defaultState(), 3);
                                }
                            }

                            // MC 1.16.5: if (random.nextBoolean()) { return; }
                            if (rng.nextBoolean()) {
                                return;
                            }
                        }
                    }
                }
                endZLoop:;
            }
            endXLoop:;
        }
        endYLoop:;
    }
}

} // namespace entity::ai::goal
} // namespace mc
