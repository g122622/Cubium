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

#include "GhastGoals.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/passive/happyghast/HappyGhastEntity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/core/CoordConverter.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/IChunkManager.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include <cmath>
#include <optional>

namespace mc::entity::ai::goal {

// ============================================================================
// GhastRandomFlyGoal
// ============================================================================

GhastRandomFlyGoal::GhastRandomFlyGoal(GhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_ghast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

GhastRandomFlyGoal::GhastRandomFlyGoal(HappyGhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_ghast(nullptr)
    , m_happyGhast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

MobEntity* GhastRandomFlyGoal::_host() const
{
    return m_ghast != nullptr ? static_cast<MobEntity*>(m_ghast) : static_cast<MobEntity*>(m_happyGhast);
}

bool GhastRandomFlyGoal::shouldExecute()
{
    MobEntity* host = _host();
    if (host == nullptr || host->moveController() == nullptr) {
        return false;
    }

    auto* moveController = host->moveController();

    // 条件0: 被拴在实体上时由拴绳牵引目标负责，不走随机巡游
    // （对齐 vanilla HappyGhastAi：跟随拴绳持有者优先于 IDLE 的随机飞行）。
    if (host->isLeashed() && host->leashHolderUuid().has_value()) {
        return false;
    }

    // 条件1: 移动控制器空闲（没有目标）
    if (!moveController->isUpdating()) {
        return true;
    }

    // 条件2: 目标距离太近（小于1格）
    f64 dx = moveController->getX() - host->x();
    f64 dy = moveController->getY() - host->y();
    f64 dz = moveController->getZ() - host->z();
    f64 distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < MIN_DISTANCE_SQ) {
        return true;
    }

    // 条件3: 目标距离太远（超过60格）
    if (distSq > MAX_DISTANCE_SQ) {
        return true;
    }

    return false;
}

void GhastRandomFlyGoal::startExecuting()
{
    MobEntity* host = _host();
    if (host == nullptr || host->moveController() == nullptr) {
        return;
    }

    // 在当前位置周围选择随机目标点
    math::IRandom& rng = host->getRandom();

    // 目标范围: 当前位置 ±16格
    f64 targetX = host->x() + (rng.nextFloat() * 2.0 - 1.0) * WANDER_RANGE;
    f64 targetY = host->y() + (rng.nextFloat() * 2.0 - 1.0) * WANDER_RANGE;
    f64 targetZ = host->z() + (rng.nextFloat() * 2.0 - 1.0) * WANDER_RANGE;

    // 目标点落在地表以下时把高度折回当前位置附近——对齐 vanilla Ghast.RandomFloatAroundGoal
    // 的 getSuitableFlyToPosition 末尾那段修正：
    //
    //   int j = level.getHeight(Heightmap.Types.MOTION_BLOCKING, blockpos.getX(), blockpos.getZ());
    //   if (j < blockpos.getY() && j > level.getMinY()) {
    //       vec31 = new Vec3(vec31.x(), p.getY() - Math.abs(p.getY() - vec31.y()), vec31.z());
    //   }
    //
    // 缺这段修正时目标 Y 完全自由（y±16）：y=2 处的恶魂会抽到地板下方的点，飞行控制器
    // 直接朝地下加速，穿过 y=0 的圆石地板一路掉到 y<0（实测掉到 y=-3.44），此后看不见
    // 玩家、永不发射火球。原版靠这段修正把这类目标点折回当前位置附近，绕开地下。
    //
    // 高度取自 MOTION_BLOCKING 高度图（与 vanilla 同类型）：IWorld::getHeight 用的是
    // WorldSurfaceWG，语义不同，不能替代。区块未加载时跳过修正，交给飞行控制器自行约束。
    if (IWorld* world = host->world(); world != nullptr) {
        const BlockPos targetPos(static_cast<i32>(std::floor(targetX)),
            static_cast<i32>(std::floor(targetY)),
            static_cast<i32>(std::floor(targetZ)));
        auto* chunkManager = world->chunkManager();
        ChunkData* chunk = chunkManager != nullptr
            ? chunkManager->tryToGetChunkInMem(util::core::CoordConverter::blockToChunk(targetPos.x),
                  util::core::CoordConverter::blockToChunk(targetPos.z))
            : nullptr;
        if (chunk != nullptr) {
            const i32 localX = targetPos.x - util::core::CoordConverter::blockToChunk(targetPos.x) * 16;
            const i32 localZ = targetPos.z - util::core::CoordConverter::blockToChunk(targetPos.z) * 16;
            const i32 groundY = chunk->getTopBlockY(HeightmapType::MotionBlocking, localX, localZ);
            if (groundY < targetPos.y && groundY > world->getMinBuildHeight()) {
                targetY = host->y() - std::abs(host->y() - targetY);
            }
        }
    }

    host->moveController()->setMoveTo(targetX, targetY, targetZ, 1.0);
}

// ============================================================================
// GhastFollowLeashHolderGoal
// ============================================================================

GhastFollowLeashHolderGoal::GhastFollowLeashHolderGoal(HappyGhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_ghast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

Entity* GhastFollowLeashHolderGoal::_resolveHolder() const
{
    if (m_ghast == nullptr || m_ghast->world() == nullptr || !m_ghast->isLeashed()) {
        return nullptr;
    }
    const std::optional<std::string>& holderUuid = m_ghast->leashHolderUuid();
    if (!holderUuid.has_value()) {
        return nullptr;
    }
    return m_ghast->world()->getEntityByUuid(*holderUuid);
}

bool GhastFollowLeashHolderGoal::shouldExecute()
{
    Entity* holder = _resolveHolder();
    if (holder == nullptr || m_ghast->moveController() == nullptr) {
        return false;
    }
    return m_ghast->distanceTo(*holder) > FOLLOW_DISTANCE;
}

bool GhastFollowLeashHolderGoal::shouldContinueExecuting()
{
    Entity* holder = _resolveHolder();
    if (holder == nullptr) {
        return false;
    }
    return m_ghast->distanceTo(*holder) > FOLLOW_DISTANCE * 0.75;
}

void GhastFollowLeashHolderGoal::startExecuting()
{
    tick();
}

void GhastFollowLeashHolderGoal::tick()
{
    Entity* holder = _resolveHolder();
    if (holder == nullptr || m_ghast->moveController() == nullptr) {
        return;
    }
    m_ghast->moveController()->setMoveTo(holder->x(), holder->y(), holder->z(), 1.0);
}

// ============================================================================
// GhastHappyFloatGoal
// ============================================================================

GhastHappyFloatGoal::GhastHappyFloatGoal(HappyGhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_ghast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

bool GhastHappyFloatGoal::shouldExecute()
{
    if (m_ghast == nullptr || !m_ghast->isChild() || m_ghast->moveController() == nullptr) {
        return false;
    }
    // 仅在移动控制器空闲时接管（成年切换为随机飞行后不再漂浮）。
    return !m_ghast->moveController()->isUpdating();
}

bool GhastHappyFloatGoal::shouldContinueExecuting()
{
    return m_ghast != nullptr && m_ghast->isChild();
}

void GhastHappyFloatGoal::startExecuting()
{
    m_hasTarget = false;
}

void GhastHappyFloatGoal::tick()
{
    if (m_ghast == nullptr) {
        return;
    }
    // 对齐 vanilla HappyGhast 的幼体悬浮：持续向上加速直到接近目标高度。
    // 目标高度在首次接管时以当前位置为基准固定，上升速度上限 0.4（近似 FlyingMoveControl
    // 的 8 格活动半径约束），越过目标高度后不再继续加速，避免无限上升。
    if (!m_hasTarget) {
        m_targetY = static_cast<f32>(m_ghast->y()) + FLOAT_HEIGHT;
        m_hasTarget = true;
    }
    if (static_cast<f32>(m_ghast->y()) < m_targetY && m_ghast->velocity().y < MAX_RISE_SPEED) {
        const Vector3 velocity = m_ghast->velocity();
        m_ghast->setVelocity(velocity.x, velocity.y + FLOAT_SPEED, velocity.z);
    }
}

// ============================================================================
// GhastLookAroundGoal
// ============================================================================

GhastLookAroundGoal::GhastLookAroundGoal(GhastEntity* ghast)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_ghast(ghast)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

void GhastLookAroundGoal::tick()
{
    if (m_ghast == nullptr) {
        return;
    }

    // 根据是否有攻击目标更新朝向
    LivingEntity* attackTarget = m_ghast->attackTarget();

    if (attackTarget == nullptr) {
        // 无攻击目标：朝向移动方向
        Vector3 velocity = m_ghast->velocity();
        f32 targetYaw = -static_cast<f32>(math::toDegrees(std::atan2(velocity.x, velocity.z)));
        m_ghast->setRotation(targetYaw, m_ghast->pitch());
        m_ghast->setRenderYawOffset(targetYaw);
    } else {
        // 有攻击目标：朝向攻击目标
        f64 dx = attackTarget->x() - m_ghast->x();
        f64 dz = attackTarget->z() - m_ghast->z();
        f64 distSq = dx * dx + dz * dz;

        if (distSq < LOOK_RANGE_SQ) {
            // 在攻击范围内，朝向目标
            f32 targetYaw = -static_cast<f32>(math::toDegrees(std::atan2(dx, dz)));
            m_ghast->setRotation(targetYaw, m_ghast->pitch());
            m_ghast->setRenderYawOffset(targetYaw);
        }
    }
}

// ============================================================================
// GhastFireballAttackGoal
// ============================================================================

GhastFireballAttackGoal::GhastFireballAttackGoal(GhastEntity* ghast)
    : Goal()
    , m_ghast(ghast)
    , m_target(nullptr)
    , m_attackTimer(0)
{
    MC_ASSERT_RELEASE(ghast != nullptr);
}

bool GhastFireballAttackGoal::shouldExecute()
{
    if (m_ghast == nullptr) {
        return false;
    }

    // 有攻击目标时执行
    LivingEntity* target = m_ghast->attackTarget();
    return target != nullptr && target->isAlive();
}

void GhastFireballAttackGoal::startExecuting()
{
    // 重置攻击计时器
    m_attackTimer = 0;
}

void GhastFireballAttackGoal::resetTask()
{
    if (m_ghast == nullptr) {
        return;
    }

    // 清除攻击状态
    m_ghast->setCharging(false);
    m_target = nullptr;
}

void GhastFireballAttackGoal::tick()
{
    if (m_ghast == nullptr || m_ghast->world() == nullptr) {
        return;
    }

    m_target = m_ghast->attackTarget();

    if (m_target == nullptr || !m_target->isAlive()) {
        return;
    }

    f64 distSq = m_ghast->distanceSqTo(*m_target);

    // 在攻击范围内（64格）且能看到目标
    if (distSq < ATTACK_RANGE_SQ && m_ghast->canSee(*m_target)) {
        // 增加攻击计时器
        ++m_attackTimer;

        // 充能音效（第10 tick）
        if (m_attackTimer == CHARGE_SOUND_TICK && !m_ghast->isSilent()) {
            // 充能音效事件
            BlockPos pos(
                static_cast<i32>(m_ghast->x()), static_cast<i32>(m_ghast->y()), static_cast<i32>(m_ghast->z()));
            m_ghast->world()->playEvent(world::WorldEvents::GHAST_WARN_SOUND, pos, 0);
        }

        // 充能完成（第20 tick）
        if (m_attackTimer >= CHARGE_DURATION) {
            // 发射火球
            m_ghast->shootFireball();

            // 进入冷却
            m_attackTimer = -COOLDOWN_DURATION;
        }

        // 更新攻击状态（用于客户端动画）
        m_ghast->setCharging(m_attackTimer > 10);
    } else {
        // 不在攻击范围内，逐渐减少计时器
        if (m_attackTimer > 0) {
            --m_attackTimer;
        }
        m_ghast->setCharging(false);
    }
}

} // namespace mc::entity::ai::goal
