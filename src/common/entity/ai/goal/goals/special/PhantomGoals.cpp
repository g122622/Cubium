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

/**
 * @file PhantomGoals.cpp
 * @brief 幻翼专用的AI目标类实现
 */

#include "PhantomGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/basic/PhantomEntity.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

#include <algorithm>
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// PhantomAttackPlayerTargetGoal
// ============================================================================

PhantomAttackPlayerTargetGoal::PhantomAttackPlayerTargetGoal(PhantomEntity* phantom)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    , m_phantom(phantom)
    , m_tickDelay(20) // 初始延迟20tick
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

bool PhantomAttackPlayerTargetGoal::shouldExecute()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return false;
    }

    // 每60tick搜索一次玩家（初始20tick后）
    if (m_tickDelay > 0) {
        --m_tickDelay;
        return false;
    }

    Player* player = _findAttackablePlayer();
    if (player != nullptr) {
        m_phantom->setAttackTarget(player);
        m_tickDelay = 60; // 成功后60tick延迟
        return true;
    }

    m_tickDelay = 60;
    return false;
}

bool PhantomAttackPlayerTargetGoal::shouldContinueExecuting()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 对齐 vanilla PhantomAttackPlayerTargetGoal.canContinueToUse：
    //   return Phantom.this.canAttack(getServerLevel(...), getTarget(), TargetingConditions.DEFAULT);
    // TargetingConditions.DEFAULT = forCombat()，range=-1（不检查距离），仅校验目标有效性、
    // 非旁观、非创造（forCombat 语义）。故此处不检查距离——旧实现的
    //   distanceSq <= followRange * followRange
    // 是非 vanilla 的错误约束：幻翼环绕点 anchor 被 setAnchorAboveTarget clamp 到海平面以上
    // （vanilla 行为），在低海拔世界（如 GameTest gridStartY=-59，玩家 y≈-57）幻翼飞至 anchor
    // （y≈64）与玩家垂直差 ~120 格 > followRange(64)，旧距离检查会在此误判"目标过远"而 resetTask
    // 清空 attackTarget，导致幻翼在"选目标→飞高→丢目标→重选"间循环，永远进不了 SWOOP 俯冲。
    // vanilla DEFAULT 不检查距离，幻翼一旦锁定目标即持续追击，故移除距离检查。
    auto* player = dynamic_cast<Player*>(target);
    if (player == nullptr) {
        return false;
    }

    // 不能攻击旁观者或创造模式玩家（对齐 TargetingConditions.forCombat）
    if (player->isSpectator() || player->isCreative()) {
        return false;
    }

    return true;
}

void PhantomAttackPlayerTargetGoal::resetTask()
{
    if (m_phantom != nullptr) {
        m_phantom->setAttackTarget(nullptr);
    }
}

Player* PhantomAttackPlayerTargetGoal::_findAttackablePlayer()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return nullptr;
    }

    IWorld* world = m_phantom->world();
    math::Vector3 pos = m_phantom->position();

    // 搜索范围对齐 vanilla PhantomAttackPlayerTargetGoal.canUse：
    //   getNearbyPlayers(attackTargeting, this, getBoundingBox().inflate(16, 64, 16))
    //   attackTargeting = TargetingConditions.forCombat().range(64)
    //   对列表按 Y 反向排序后，取第一个 canAttack(=TargetingConditions.DEFAULT.test) 通过的玩家。
    // 这里用 findClosestEntity<Player> 在 SEARCH_RANGE=64 球内取最近玩家，过滤条件对齐
    // TargetingConditions.forCombat 的语义：存活、非旁观、非创造（vanilla forCombat 排除 spectator/creative）。
    //
    // 【重要】vanilla 选目标时不检查玩家 Y 坐标 / 海平面——旧实现误加的
    //   if (player->position().y < SEA_LEVEL) return false;
    // 是非 vanilla 的错误过滤：GameTest 世界 gridStartY=-59，玩家 world y≈-57 远低于 SEA_LEVEL(63)，
    // 该过滤会把所有 GameTest 玩家全部滤掉，导致幻翼永远找不到攻击目标（phantom 攻击测试阻塞，
    // 也是偏离 vanilla 的 C++ 缺陷）。已移除。
    Player* nearestPlayer = EntityUtils::findClosestEntity<Player>(
        world, pos, static_cast<f32>(SEARCH_RANGE), m_phantom, [this](Player* player) -> bool {
            if (player == nullptr || !player->isAlive()) {
                return false;
            }
            // 不能攻击旁观者或创造模式玩家（对齐 TargetingConditions.forCombat）
            if (player->isSpectator() || player->isCreative()) {
                return false;
            }
            return true;
        });

    return nearestPlayer;
}

// ============================================================================
// PhantomMoveGoal
// ============================================================================

PhantomMoveGoal::PhantomMoveGoal(PhantomEntity* phantom)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_phantom(phantom)
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

bool PhantomMoveGoal::isNearOrbitOffset() const
{
    if (m_phantom == nullptr) {
        return false;
    }

    // 检查距离平方是否小于 4.0
    math::Vector3 offset = m_phantom->orbitOffset();
    math::Vector3 pos = m_phantom->position();
    f64 dx = offset.x - pos.x;
    f64 dy = offset.y - pos.y;
    f64 dz = offset.z - pos.z;
    return (dx * dx + dy * dy + dz * dz) < 4.0;
}

// ============================================================================
// PhantomOrbitPointGoal
// ============================================================================

PhantomOrbitPointGoal::PhantomOrbitPointGoal(PhantomEntity* phantom)
    : PhantomMoveGoal(phantom)
    , m_orbitAngle(0.0f)
    , m_orbitRadius(5.0f)
    , m_orbitHeightOffset(0.0f)
    , m_orbitDirection(1.0f)
{}

bool PhantomOrbitPointGoal::shouldExecute()
{
    if (m_phantom == nullptr) {
        return false;
    }

    // 无攻击目标或处于环绕阶段时执行
    LivingEntity* target = m_phantom->attackTarget();
    return target == nullptr || m_phantom->getAttackPhase() == PhantomEntity::AttackPhase::CIRCLE;
}

void PhantomOrbitPointGoal::startExecuting()
{
    if (m_phantom == nullptr) {
        return;
    }

    math::Random& rng = m_phantom->getRandom();

    // 初始化环绕参数
    m_orbitRadius = 5.0f + rng.nextFloat() * 10.0f;       // 5.0 + [0, 10.0)
    m_orbitHeightOffset = -4.0f + rng.nextFloat() * 9.0f; // -4.0 + [0, 9.0)
    m_orbitDirection = rng.nextBoolean() ? 1.0f : -1.0f;  // 随机方向

    _updateOrbitOffset();
}

void PhantomOrbitPointGoal::tick()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    math::Random& rng = m_phantom->getRandom();

    // 350tick 概率改变高度
    if (rng.nextInt(350) == 0) {
        m_orbitHeightOffset = -4.0f + rng.nextFloat() * 9.0f;
    }

    // 250tick 概率增加半径或反转方向
    if (rng.nextInt(250) == 0) {
        ++m_orbitRadius;
        if (m_orbitRadius > 15.0f) {
            m_orbitRadius = 5.0f;
            m_orbitDirection = -m_orbitDirection;
        }
    }

    // 450tick 概率重新选择起始角度
    if (rng.nextInt(450) == 0) {
        m_orbitAngle = rng.nextFloat() * math::TWO_PI;
        _updateOrbitOffset();
    }

    // 接近目标点时更新环绕偏移
    if (isNearOrbitOffset()) {
        _updateOrbitOffset();
    }

    // 避开地面和天花板
    math::Vector3 pos = m_phantom->position();
    math::Vector3 offset = m_phantom->orbitOffset();
    IWorld* world = m_phantom->world();

    // 检查下方方块
    BlockPos belowPos(static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y - 1.0)),
        static_cast<i32>(std::floor(pos.z)));
    const BlockState* belowState = world->getBlockState(belowPos);

    if (offset.y < pos.y && belowState != nullptr && !belowState->getBlock().isAir(*belowState)) {
        // 下方有方块，向上飞
        m_orbitHeightOffset = std::max(1.0f, m_orbitHeightOffset);
        _updateOrbitOffset();
    }

    // 检查上方方块
    BlockPos abovePos(static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y + 1.0)),
        static_cast<i32>(std::floor(pos.z)));
    const BlockState* aboveState = world->getBlockState(abovePos);

    if (offset.y > pos.y && aboveState != nullptr && !aboveState->getBlock().isAir(*aboveState)) {
        // 上方有方块，向下飞
        m_orbitHeightOffset = std::min(-1.0f, m_orbitHeightOffset);
        _updateOrbitOffset();
    }
}

void PhantomOrbitPointGoal::_updateOrbitOffset()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    BlockPos orbitPos = m_phantom->orbitPosition();

    // 如果环绕位置未初始化，使用当前位置
    if (orbitPos.x == 0 && orbitPos.y == 0 && orbitPos.z == 0) {
        math::Vector3 pos = m_phantom->position();
        orbitPos = BlockPos(static_cast<i32>(std::floor(pos.x)),
            static_cast<i32>(std::floor(pos.y)),
            static_cast<i32>(std::floor(pos.z)));
        m_phantom->setOrbitPosition(orbitPos);
    }

    // 更新环绕角度
    m_orbitAngle += m_orbitDirection * 15.0f * math::DEG_TO_RAD;

    // 计算环绕偏移
    // orbitOffset = orbitPosition + (radius * cos(angle), heightOffset, radius * sin(angle))
    f32 offsetX = m_orbitRadius * std::cos(m_orbitAngle);
    f32 offsetY = -4.0f + m_orbitHeightOffset;
    f32 offsetZ = m_orbitRadius * std::sin(m_orbitAngle);

    math::Vector3f offset(static_cast<f32>(orbitPos.x) + offsetX,
        static_cast<f32>(orbitPos.y) + offsetY,
        static_cast<f32>(orbitPos.z) + offsetZ);

    m_phantom->setOrbitOffset(offset);
}

// ============================================================================
// PhantomPickAttackGoal
// ============================================================================

PhantomPickAttackGoal::PhantomPickAttackGoal(PhantomEntity* phantom)
    // 对齐 vanilla PhantomAttackStrategyGoal：不设任何 GoalFlag（vanilla `extends Goal` 无 setFlags）。
    // 本 goal 仅负责攻击阶段切换（CIRCLE→SWOOP 倒计时），自身不驱动移动——实际飞行由
    // OrbitPointGoal（CIRCLE 阶段）和 SweepAttackGoal（SWOOP 阶段，二者均带 Move flag）按 phase 互斥执行。
    // 旧实现误设 GoalFlag::Move，导致本 goal 一旦锁定目标（shouldExecute: target!=null）就长期占据
    // m_flagGoals[Move]，按优先级压制 SweepAttackGoal（同 Move、优先级更低）使其永远无法启动——
    // 幻翼攻击链在此断裂，永远不俯冲攻击（phantom 攻击测试 broken 的根因）。移除 flag 后，
    // SweepAttackGoal 可在 SWOOP 阶段正常抢占 Move 执行俯冲。
    : Goal()
    , m_phantom(phantom)
    , m_tickDelay(0)
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

bool PhantomPickAttackGoal::shouldExecute()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    return target != nullptr && target->isAlive();
}

void PhantomPickAttackGoal::startExecuting()
{
    // 初始延迟10tick后切换到俯冲
    m_tickDelay = 10;
    m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    _setOrbitPositionAboveTarget();
}

void PhantomPickAttackGoal::resetTask()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    // 更新环绕位置到目标上方
    LivingEntity* target = m_phantom->attackTarget();
    if (target != nullptr) {
        math::Random& rng = m_phantom->getRandom();
        math::Vector3 targetPos = target->position();

        // 获取最高方块位置 + 10~30格
        i32 surfaceY = m_phantom->world()->getHeight(
            static_cast<i32>(std::floor(targetPos.x)), static_cast<i32>(std::floor(targetPos.z)));

        BlockPos newOrbitPos(static_cast<i32>(std::floor(targetPos.x)),
            surfaceY + 10 + rng.nextInt(20),
            static_cast<i32>(std::floor(targetPos.z)));

        m_phantom->setOrbitPosition(newOrbitPos);
    }
}

void PhantomPickAttackGoal::tick()
{
    if (m_phantom == nullptr) {
        return;
    }

    // 只在环绕阶段处理
    if (m_phantom->getAttackPhase() == PhantomEntity::AttackPhase::CIRCLE) {
        --m_tickDelay;
        if (m_tickDelay <= 0) {
            // 切换到俯冲阶段
            m_phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
            _setOrbitPositionAboveTarget();

            // 俯冲持续时间：8-12秒 (160-240 tick)
            math::Random& rng = m_phantom->getRandom();
            m_tickDelay = (8 + rng.nextInt(4)) * 20;

            // 播放俯冲音效
            m_phantom->playSound(SoundEvents::ENTITY_PHANTOM_SWOOP, 10.0f, 0.95f + rng.nextFloat() * 0.1f);
        }
    }
}

void PhantomPickAttackGoal::_setOrbitPositionAboveTarget()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr) {
        return;
    }

    math::Random& rng = m_phantom->getRandom();
    math::Vector3 targetPos = target->position();

    // 目标上方 20-40 格
    i32 orbitY = static_cast<i32>(std::floor(targetPos.y)) + 20 + rng.nextInt(20);

    // 不能低于海平面
    if (orbitY < world::SEA_LEVEL) {
        orbitY = world::SEA_LEVEL + 1;
    }

    BlockPos orbitPos(static_cast<i32>(std::floor(targetPos.x)), orbitY, static_cast<i32>(std::floor(targetPos.z)));

    m_phantom->setOrbitPosition(orbitPos);
}

// ============================================================================
// PhantomSweepAttackGoal
// ============================================================================

PhantomSweepAttackGoal::PhantomSweepAttackGoal(PhantomEntity* phantom)
    : PhantomMoveGoal(phantom)
    , m_catCheckTimer(20) // 初始化为20，确保首次调用 _checkForCats() 时立即检测
    , m_isScaredOfCat(false)
{}

bool PhantomSweepAttackGoal::shouldExecute()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    return target != nullptr && m_phantom->getAttackPhase() == PhantomEntity::AttackPhase::SWOOP;
}

bool PhantomSweepAttackGoal::shouldContinueExecuting()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 检查目标是否是玩家，且不在旁观/创造模式
    auto* player = dynamic_cast<Player*>(target);
    if (player != nullptr && (player->isSpectator() || player->isCreative())) {
        return false;
    }

    // 检查是否仍在俯冲阶段
    if (!shouldExecute()) {
        return false;
    }

    // 检测猫：如果附近有猫，幻翼停止俯冲攻击
    return _checkForCats();
}

void PhantomSweepAttackGoal::resetTask()
{
    if (m_phantom != nullptr) {
        m_phantom->setAttackTarget(nullptr);
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    }
}

void PhantomSweepAttackGoal::tick()
{
    if (m_phantom == nullptr) {
        return;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr) {
        return;
    }

    // 设置环绕偏移为目标位置（眼睛高度 0.5）
    math::Vector3f targetPos = target->position();
    math::Vector3f offset(targetPos.x, targetPos.y + target->eyeHeight() * 0.5f, targetPos.z);
    m_phantom->setOrbitOffset(offset);

    // 检测碰撞
    // 如果幻翼的碰撞箱扩大0.2格后与目标碰撞箱相交，执行攻击
    AxisAlignedBB phantomBB = m_phantom->boundingBox().grow(0.2);
    AxisAlignedBB targetBB = target->boundingBox();

    if (phantomBB.intersects(targetBB)) {
        // 执行攻击
        m_phantom->attackEntityAsMob(*target);
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);

        // 播放攻击音效
        // 世界事件 PHANTOM_BITE_SOUND 是幻翼攻击事件
        if (IWorld* world = m_phantom->world()) {
            world->playEvent(world::WorldEvents::PHANTOM_BITE_SOUND,
                BlockPos(math::floorTo<i32>(m_phantom->x()),
                    math::floorTo<i32>(m_phantom->y()),
                    math::floorTo<i32>(m_phantom->z())),
                0);
        }
    }
    // 水平碰撞或受伤时切回环绕
    else if (m_phantom->collidedHorizontally() || m_phantom->hurtTime() > 0) {
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    }
}

bool PhantomSweepAttackGoal::_checkForCats()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return true; // 无世界时继续攻击
    }

    // 每20tick检测一次猫
    ++m_catCheckTimer;
    if (m_catCheckTimer < 20) {
        return !m_isScaredOfCat; // 非检测tick，保持上次的害怕状态
    }
    m_catCheckTimer = 0;

    // 对齐 MC 原版：搜索幻翼碰撞箱各方向扩展16格范围内的Cat实体
    // 使用 getBoundingBox().inflate(16.0) 搜索
    AxisAlignedBB searchBox = m_phantom->boundingBox().grow(16.0);
    IWorld* world = m_phantom->world();
    auto entities = world->getEntitiesInAABB(searchBox, m_phantom);

    bool foundCat = false;
    for (Entity* entity : entities) {
        // 检查是否为猫实体（使用 typeId 快速匹配）
        if (entity->entityType() == entity::VanillaEntityTypeKeys::CAT && entity->isAlive()) {
            CatEntity* cat = static_cast<CatEntity*>(entity);
            cat->hiss();
            foundCat = true;
        }
    }

    m_isScaredOfCat = foundCat;
    return !m_isScaredOfCat; // 有猫时返回false（停止攻击），无猫时返回true（继续攻击）
}

} // namespace mc::entity::ai::goal
