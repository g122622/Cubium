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

#include "EndermanGoals.hpp"

#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/math/ray/Raycast.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/Block.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../../../../world/block/BlockRegistry.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/block/BlockTags.hpp"
#include "../../../../../world/gameevent/GameEvent.hpp"
#include "../../../../../world/gameevent/GameEvents.hpp"
#include "../../../../../world/gamerule/GameRules.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/monster/end/EndermanEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../registry/VanillaEntityTypeKeys.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/ray/Ray.hpp"
#include <limits>
#include <vector>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// EndermanStareGoal 实现
// ============================================================================

EndermanStareGoal::EndermanStareGoal(EndermanEntity* enderman)
    // mutex flags 用 Look|Move（非 vanilla Java 1.21.11 的 Jump|Move）：Move flag 注视时霸占压制
    // MeleeAttackGoal(Move|Look) 使末影人被注视时冻结不近战（对齐 vanilla EndermanFreezeWhenLookedAt
    // 注视冻结语义）；Look flag 抑制注视期间其他 Look goal（如 LookAtPlayerGoal）竞争视线控制器。
    // TODO: 对齐 vanilla 用 Jump|Move（Java EndermanFreezeWhenLookedAt flags=EnumSet.of(JUMP,MOVE)），
    //   需评估 Jump flag 在 Cubium 的实际效果及对注视期间 Look goal 调度的影响后切换。
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move})
    , m_enderman(enderman)
{
    MC_ASSERT(enderman != nullptr);
}

bool EndermanStareGoal::shouldExecute()
{
    // 获取攻击目标
    m_targetPlayer = m_enderman->getAttackTarget();

    // 检查是否是玩家
    if (m_targetPlayer == nullptr || m_targetPlayer->entityType() != entity::VanillaEntityTypeKeys::PLAYER) {
        return false;
    }

    // 检查距离是否在 16 格内
    f64 distSq = m_enderman->distanceSqTo(*m_targetPlayer);
    if (distSq > STARE_RANGE_SQ) {
        return false;
    }

    // 检查玩家是否正在注视末影人
    Player* player = dynamic_cast<Player*>(m_targetPlayer);
    if (player == nullptr) {
        return false;
    }
    return m_enderman->shouldAttackPlayer(*player);
}

void EndermanStareGoal::startExecuting()
{
    // 清除导航路径
    m_enderman->navigator()->clearPath();
}

void EndermanStareGoal::resetTask()
{
    m_targetPlayer = nullptr;
}

void EndermanStareGoal::tick()
{
    // 注视目标玩家的眼睛位置
    if (m_targetPlayer != nullptr) {
        m_enderman->lookController()->setLookPosition(
            m_targetPlayer->x(), m_targetPlayer->y() + m_targetPlayer->eyeHeight(), m_targetPlayer->z());
    }
}

// ============================================================================
// EndermanFindPlayerGoal 实现
// ============================================================================

EndermanFindPlayerGoal::EndermanFindPlayerGoal(EndermanEntity* enderman)
    : TargetGoal(enderman, true)
    , m_enderman(enderman)
{
    MC_ASSERT(enderman != nullptr);
}

bool EndermanFindPlayerGoal::shouldExecute()
{
    // 在附近查找正在注视末影人的玩家
    IWorld* world = m_enderman->world();
    if (world == nullptr) {
        return false;
    }

    // 获取附近的所有实体
    std::vector<Entity*> entities =
        world->getEntitiesInRange(m_enderman->position(), static_cast<f32>(TARGET_DISTANCE), m_enderman);

    // 遍历寻找最近符合条件的玩家
    Player* closestPlayer = nullptr;
    f64 closestDistSq = std::numeric_limits<f64>::max();

    for (Entity* entity : entities) {
        if (!entity->isAlive()) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 创造模式和观察者模式玩家不被攻击
        if (player->isCreative() || player->isSpectator()) {
            continue;
        }

        // 检查玩家是否正在注视末影人
        if (!m_enderman->shouldAttackPlayer(*player)) {
            continue;
        }

        // 检查距离
        f64 distSq = m_enderman->distanceSqTo(*player);
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestPlayer = player;
        }
    }

    m_targetPlayer = closestPlayer;
    return m_targetPlayer != nullptr;
}

void EndermanFindPlayerGoal::startExecuting()
{
    // 设置激怒计时器并设置愤怒状态
    m_aggroTime = AGGRO_DURATION;
    m_teleportTime = 0;

    // 设置被注视状态
    m_enderman->setScreaming(true);
}

void EndermanFindPlayerGoal::resetTask()
{
    m_targetPlayer = nullptr;
    TargetGoal::resetTask();
}

bool EndermanFindPlayerGoal::shouldContinueExecuting()
{
    if (m_targetPlayer != nullptr) {
        // 如果玩家不再注视末影人，停止
        if (!m_enderman->shouldAttackPlayer(*m_targetPlayer)) {
            return false;
        }

        // 继续注视玩家
        m_enderman->lookController()->setLookPositionWithEntity(*m_targetPlayer, 10.0f, 10.0f);
        return true;
    }

    // 如果有攻击目标，检查是否还能看到
    if (m_target != nullptr) {
        // 使用 TargetGoal 的视线记忆检查
        if (m_checkSight && m_unseenTicks > m_unseenMemoryTicks) {
            return false;
        }
        return m_target->isAlive();
    }

    return TargetGoal::shouldContinueExecuting();
}

void EndermanFindPlayerGoal::tick()
{
    if (m_enderman->getAttackTarget() == nullptr) {
        // 还没有被激怒，继续注视玩家
        m_target = nullptr;
    }

    if (m_targetPlayer != nullptr) {
        // 倒计时激怒时间
        if (--m_aggroTime <= 0) {
            // 激怒末影人，设置攻击目标
            m_target = m_targetPlayer;
            m_targetPlayer = nullptr;
            TargetGoal::startExecuting();
        }
    } else {
        // 已经被激怒，处理攻击逻辑
        if (m_target != nullptr && !m_enderman->isRiding()) {
            Player* playerTarget = dynamic_cast<Player*>(m_target);

            if (playerTarget != nullptr && m_enderman->shouldAttackPlayer(*playerTarget)) {
                // 玩家仍在注视末影人
                f64 distSq = m_enderman->distanceSqTo(*m_target);

                // 近距离瞬移躲避
                if (distSq < TELEPORT_NEAR_DISTANCE_SQ) {
                    m_enderman->teleport();
                    m_teleportTime = 0;
                }
            } else if (m_target->distanceSqTo(*m_enderman) > TELEPORT_FAR_DISTANCE_SQ) {
                // 远距离瞬移接近
                if (m_teleportTime++ >= TELEPORT_COOLDOWN_TICKS) {
                    if (m_enderman->teleportToTarget()) {
                        m_teleportTime = 0;
                    }
                }
            }
        }

        TargetGoal::tick();
    }
}

bool EndermanFindPlayerGoal::shouldAttackPlayer(Player* player) const
{
    if (player == nullptr) {
        return false;
    }
    return m_enderman->shouldAttackPlayer(*player);
}

// ============================================================================
// EndermanPlaceBlockGoal 实现
// ============================================================================

EndermanPlaceBlockGoal::EndermanPlaceBlockGoal(EndermanEntity* enderman)
    : Goal(EnumSet<GoalFlag>{})
    , m_enderman(enderman)
{
    MC_ASSERT(enderman != nullptr);
}

bool EndermanPlaceBlockGoal::shouldExecute()
{
    // 1. 必须拿着方块
    if (!m_enderman->isHoldingBlock() || m_enderman->getHeldBlockState() == nullptr) {
        return false;
    }

    // 2. 检查 mobGriefing 游戏规则
    IWorld* world = m_enderman->world();
    if (world == nullptr) {
        return false;
    }

    if (!world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return false;
    }

    // 3. 1/2000 概率执行
    return m_enderman->getRandom().nextInt(PLACE_CHANCE) == 0;
}

void EndermanPlaceBlockGoal::tick()
{
    IWorld* world = m_enderman->world();
    if (world == nullptr) {
        return;
    }

    const BlockState* heldState = m_enderman->getHeldBlockState();
    if (heldState == nullptr) {
        return;
    }

    math::Random& rng = m_enderman->getRandom();

    // 在末影人周围 2x2x2 范围内随机选择放置位置
    i32 x = math::floorTo<i32>(m_enderman->x() - 1.0 + rng.nextDouble() * 2.0);
    i32 y = math::floorTo<i32>(m_enderman->y() + rng.nextDouble() * 2.0);
    i32 z = math::floorTo<i32>(m_enderman->z() - 1.0 + rng.nextDouble() * 2.0);
    BlockPos pos(x, y, z);

    const BlockState* currentState = world->getBlockState(pos);
    if (currentState == nullptr) {
        return;
    }

    BlockPos belowPos(x, y - 1, z);
    const BlockState* belowState = world->getBlockState(belowPos);
    if (belowState == nullptr) {
        return;
    }

    // 根据邻居状态更新方块形状（处理方向性方块等）
    BlockState updatedState = Block::updateFromNeighbourShapes(*heldState, *world, pos);

    // 检查是否可以放置
    if (!canPlaceBlock(world, pos, &updatedState, currentState, belowState, belowPos)) {
        return;
    }

    // 放置方块
    world->setBlockState(pos, &updatedState, 3);

    // 发出方块放置游戏事件
    world->gameEvent(gameevent::GameEvents::BLOCK_PLACE,
        pos,
        gameevent::GameEvent::Context::of(static_cast<const Entity*>(m_enderman), &updatedState));

    // 清除拿着的方块
    m_enderman->setHeldBlockState(nullptr);
}

bool EndermanPlaceBlockGoal::canPlaceBlock(IWorld* world,
    const BlockPos& pos,
    const BlockState* state,
    const BlockState* currentState,
    const BlockState* belowState,
    const BlockPos& belowPos) const
{
    // 1. 目标位置必须是空气
    if (!currentState->isAir()) {
        return false;
    }

    // 2. 下方方块不能是空气
    if (belowState->isAir()) {
        return false;
    }

    // 3. 下方方块不能是基岩
    const Block& belowBlock = belowState->owner();
    if (belowBlock.blockLocation() == ResourceLocation("minecraft", "bedrock")) {
        return false;
    }

    // 4. 下方方块的碰撞形状必须在顶面方向完全覆盖（等价于 MC Java 的 isCollisionShapeFullBlock）
    if (!Block::hasEnoughSolidSide(*world, belowPos, Direction::Up)) {
        return false;
    }

    // 5. 方块必须能在目标位置存活（isValidPosition / canSurvive）
    auto& blockReader = static_cast<IBlockReader&>(*world);
    if (!state->owner().isValidPosition(*state, blockReader, pos)) {
        return false;
    }

    // 6. 放置位置不能有实体碰撞
    AxisAlignedBB box(static_cast<f32>(pos.x),
        static_cast<f32>(pos.y),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z + 1));
    std::vector<Entity*> entities = world->getEntitiesInAABB(box, m_enderman);
    if (!entities.empty()) {
        return false;
    }

    return true;
}

// ============================================================================
// EndermanTakeBlockGoal 实现
// ============================================================================

EndermanTakeBlockGoal::EndermanTakeBlockGoal(EndermanEntity* enderman)
    : Goal(EnumSet<GoalFlag>{})
    , m_enderman(enderman)
{
    MC_ASSERT(enderman != nullptr);
}

bool EndermanTakeBlockGoal::shouldExecute()
{
    // 1. 必须没有拿着方块
    if (m_enderman->isHoldingBlock()) {
        return false;
    }

    // 2. 检查 mobGriefing 游戏规则
    IWorld* world = m_enderman->world();
    if (world == nullptr) {
        return false;
    }

    if (!world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return false;
    }

    // 3. 1/20 概率执行
    return m_enderman->getRandom().nextInt(TAKE_CHANCE) == 0;
}

void EndermanTakeBlockGoal::tick()
{
    IWorld* world = m_enderman->world();
    if (world == nullptr) {
        return;
    }

    math::Random& rng = m_enderman->getRandom();

    // 在末影人周围 4x3x4 范围内随机选择拾取位置
    i32 x = math::floorTo<i32>(m_enderman->x() - 2.0 + rng.nextDouble() * 4.0);
    i32 y = math::floorTo<i32>(m_enderman->y() + rng.nextDouble() * 3.0);
    i32 z = math::floorTo<i32>(m_enderman->z() - 2.0 + rng.nextDouble() * 4.0);
    BlockPos pos(x, y, z);

    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return;
    }

    const Block& targetBlock = state->owner();

    // 检查方块是否在 ENDERMAN_HOLDABLE 标签中
    if (!BlockTags::ENDERMAN_HOLDABLE().contains(targetBlock)) {
        return;
    }

    // 射线检测：从末影人脚部所在方块的中心到目标方块中心，确保视线无阻挡
    // 参考 MC Java EnderMan.EndermanTakeBlockGoal.tick() 中的 level.clip() 调用
    Vector3f startPos(static_cast<f32>(math::floorTo<i32>(m_enderman->x())) + 0.5f,
        static_cast<f32>(y) + 0.5f,
        static_cast<f32>(math::floorTo<i32>(m_enderman->z())) + 0.5f);
    Vector3f targetPos(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f, static_cast<f32>(z) + 0.5f);
    Vector3f direction = (targetPos - startPos).normalized();
    f32 distance = static_cast<f32>((targetPos - startPos).length());

    Ray ray(startPos, direction);
    RaycastContext context(ray, distance);
    BlockRaycastResult result = raycastBlocks(context, *world);

    // 只有当射线命中目标方块时才允许拾取（确保视线无阻挡）
    if (!result.isHit() || result.blockPos() != pos) {
        return;
    }

    // 移除方块（设置空气）
    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState == nullptr) {
        return;
    }
    world->setBlockState(pos, airState, 3);

    // 发出方块破坏游戏事件
    world->gameEvent(gameevent::GameEvents::BLOCK_DESTROY,
        pos,
        gameevent::GameEvent::Context::of(static_cast<const Entity*>(m_enderman), state));

    // 设置拿着的方块（使用默认状态）
    m_enderman->setHeldBlockState(&targetBlock.defaultState());
}

} // namespace entity::ai::goal
} // namespace mc
