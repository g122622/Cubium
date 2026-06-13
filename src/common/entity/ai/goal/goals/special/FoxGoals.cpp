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

#include "FoxGoals.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// FoxPassiveGoal - 狐狸被动目标基类
// ============================================================================

FoxPassiveGoal::FoxPassiveGoal(FoxEntity* fox)
    : m_fox(fox)
{
    // 派生类会设置自己的互斥标志
}

bool FoxPassiveGoal::shouldExecute()
{
    // 激怒状态下不执行被动行为
    if (m_fox->isFoxAggroed()) {
        return false;
    }
    return canFoxStart();
}

bool FoxPassiveGoal::shouldContinueExecuting()
{
    // 激怒状态下停止被动行为
    if (m_fox->isFoxAggroed()) {
        return false;
    }
    return canFoxContinue();
}

bool FoxPassiveGoal::hasShelter() const
{
    if (m_fox->world() == nullptr) {
        return false;
    }

    // 检查是否看不到天空且路径权重 >= 0
    BlockPos pos(
        static_cast<i32>(m_fox->x()), static_cast<i32>(m_fox->boundingBox().maxY), static_cast<i32>(m_fox->z()));
    return !m_fox->world()->canSeeSky(pos) && m_fox->getPathWeight(pos.x, pos.y, pos.z) >= 0.0f;
}

bool FoxPassiveGoal::hasAlertableTarget() const
{
    if (m_fox->world() == nullptr) {
        return false;
    }

    // 检查周围 12 格内是否有警觉目标
    AxisAlignedBB searchBox = m_fox->boundingBox().expand(12.0f, 6.0f, 12.0f);
    std::vector<Entity*> nearbyEntities = m_fox->world()->getEntitiesInAABB(searchBox, m_fox);

    for (Entity* entity : nearbyEntities) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living == nullptr || living == m_fox) {
            continue;
        }

        auto type = living->typeId();

        // 鸡、兔子 -> 警觉
        if (type == entity::EntityTypeIdNumber::CHICKEN || type == entity::EntityTypeIdNumber::RABBIT) {
            return true;
        }

        // 怪物类型 -> 警觉
        if (dynamic_cast<MonsterEntity*>(living) != nullptr) {
            return true;
        }

        // 玩家检查
        Player* player = dynamic_cast<Player*>(living);
        if (player != nullptr && !player->isSpectator() && !player->isCreative()) {
            if (!m_fox->trusts(player->id())) {
                return true;
            }
        }
    }

    return false;
}

bool FoxPassiveGoal::canAct() const
{
    return !m_fox->isSitting() && !m_fox->isCrouching() && !m_fox->isSleeping() && !m_fox->isStuck() &&
        !m_fox->isFoxAggroed();
}

// ============================================================================
// FoxFollowTargetGoal - 狐狸跟踪猎物目标
// ============================================================================

FoxFollowTargetGoal::FoxFollowTargetGoal(FoxEntity* fox)
    : m_fox(fox)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool FoxFollowTargetGoal::shouldExecute()
{
    if (m_fox->isSleeping()) {
        return false;
    }

    LivingEntity* target = m_fox->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    auto type = target->typeId();
    if (type != entity::EntityTypeIdNumber::CHICKEN && type != entity::EntityTypeIdNumber::RABBIT) {
        return false;
    }

    f64 distSq = m_fox->distanceSqTo(*target);
    if (distSq <= START_FOLLOW_DISTANCE_SQ) {
        return false;
    }

    if (m_fox->isCrouching() || m_fox->isInterested() || m_fox->isJumping()) {
        return false;
    }

    m_target = target;
    return true;
}

bool FoxFollowTargetGoal::shouldContinueExecuting()
{
    if (m_target == nullptr || !m_target->isAlive()) {
        return false;
    }

    f64 distSq = m_fox->distanceSqTo(*m_target);
    return distSq > STOP_FOLLOW_DISTANCE_SQ;
}

void FoxFollowTargetGoal::startExecuting()
{
    m_fox->setSitting(false);
    m_fox->setStuck(false);
}

void FoxFollowTargetGoal::resetTask()
{
    if (m_target != nullptr && isPathClear(m_fox, m_target)) {
        m_fox->setInterested(true);
        m_fox->setCrouching(true);
        m_fox->clearNavigation();

        auto* lookController = m_fox->lookController();
        if (lookController != nullptr) {
            lookController->setLookPositionWithEntity(*m_target,
                static_cast<f32>(m_fox->getHorizontalFaceSpeed()),
                static_cast<f32>(m_fox->getVerticalFaceSpeed()));
        }
    } else {
        m_fox->setInterested(false);
        m_fox->setCrouching(false);
    }

    m_target = nullptr;
}

void FoxFollowTargetGoal::tick()
{
    if (m_target == nullptr || !m_fox->isAlive()) {
        return;
    }

    auto* lookController = m_fox->lookController();
    if (lookController != nullptr) {
        lookController->setLookPositionWithEntity(*m_target,
            static_cast<f32>(m_fox->getHorizontalFaceSpeed()),
            static_cast<f32>(m_fox->getVerticalFaceSpeed()));
    }

    f64 distSq = m_fox->distanceSqTo(*m_target);

    if (distSq <= STOP_FOLLOW_DISTANCE_SQ) {
        m_fox->setInterested(true);
        m_fox->setCrouching(true);
        m_fox->clearNavigation();
    } else {
        m_fox->tryMoveTo(m_target->x(), m_target->y(), m_target->z(), APPROACH_SPEED);
    }
}

bool FoxFollowTargetGoal::isPathClear(FoxEntity* fox, LivingEntity* target)
{
    if (fox == nullptr || target == nullptr || fox->world() == nullptr) {
        return false;
    }

    IWorld* world = fox->world();

    f64 dx = target->x() - fox->x();
    f64 dz = target->z() - fox->z();
    f64 ratio = (dx != 0.0) ? (dz / dx) : 0.0;

    for (int i = 0; i < 6; ++i) {
        f64 progress = static_cast<f64>(i) / 6.0;

        f64 checkX;
        f64 checkZ;

        if (ratio == 0.0) {
            checkZ = dz * progress;
            checkX = dx * progress;
        } else {
            checkZ = dz * progress;
            checkX = checkZ / ratio;
        }

        for (int k = 1; k < 4; ++k) {
            BlockPos pos(static_cast<i32>(fox->x() + checkX),
                static_cast<i32>(fox->y() + k),
                static_cast<i32>(fox->z() + checkZ));

            const BlockState* state = world->getBlockState(pos);
            if (state != nullptr && !state->canBeReplaced()) {
                return false;
            }
        }
    }

    return true;
}

// ============================================================================
// FoxPounceGoal - 狐狸扑击目标
// ============================================================================

FoxPounceGoal::FoxPounceGoal(FoxEntity* fox)
    : m_fox(fox)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
}

bool FoxPounceGoal::shouldExecute()
{
    if (!m_fox->isFullyCrouched()) {
        return false;
    }

    LivingEntity* target = m_fox->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    if (!FoxFollowTargetGoal::isPathClear(m_fox, target)) {
        m_fox->tryMoveTo(target->x(), target->y(), target->z(), 0.0);
        m_fox->setCrouching(false);
        m_fox->setInterested(false);
        return false;
    }

    m_target = target;
    return true;
}

bool FoxPounceGoal::shouldContinueExecuting()
{
    if (m_target == nullptr || !m_target->isAlive()) {
        return false;
    }

    f64 motionYSq = m_fox->velocity().y * m_fox->velocity().y;
    f32 absPitch = std::abs(m_fox->pitch());

    bool inMotion = motionYSq >= MIN_MOTION_Y_SQ;
    bool pitching = absPitch >= MAX_PITCH_ANGLE;
    bool inAir = !m_fox->onGround();

    return (inMotion || pitching || inAir) && !m_fox->isStuck();
}

void FoxPounceGoal::startExecuting()
{
    m_fox->setJumping(true);
    m_fox->setPounceReady(true);
    m_fox->setInterested(false);

    if (m_target == nullptr) {
        return;
    }

    auto* lookController = m_fox->lookController();
    if (lookController != nullptr) {
        lookController->setLookPositionWithEntity(*m_target, 60.0f, 30.0f);
    }

    f64 dx = m_target->x() - m_fox->x();
    f64 dy = m_target->y() - m_fox->y();
    f64 dz = m_target->z() - m_fox->z();

    f64 dist = std::sqrt(dx * dx + dz * dz);
    if (dist > 0.001) {
        dx /= dist;
        dz /= dist;
    }

    Vector3 currentVel = m_fox->velocity();
    m_fox->setVelocity(currentVel.x + dx * POUNCE_HORIZONTAL_FACTOR,
        POUNCE_VERTICAL_FACTOR,
        currentVel.z + dz * POUNCE_HORIZONTAL_FACTOR);

    m_fox->clearNavigation();
}

void FoxPounceGoal::resetTask()
{
    m_fox->setCrouching(false);
    m_fox->setCrouchAmount(0.0f);
    m_fox->setInterested(false);
    m_fox->setPounceReady(false);
    m_target = nullptr;
}

void FoxPounceGoal::tick()
{
    if (m_target != nullptr && m_target->isAlive()) {
        auto* lookController = m_fox->lookController();
        if (lookController != nullptr) {
            lookController->setLookPositionWithEntity(*m_target, 60.0f, 30.0f);
        }
    }

    if (m_fox->isStuck()) {
        return;
    }

    Vector3 motion = m_fox->velocity();

    // 空中俯仰角调整
    if (motion.y * motion.y < 0.03 && m_fox->pitch() != 0.0f) {
        // 使用 rotLerp 进行角度插值
        f32 currentPitch = m_fox->pitch();
        f32 targetPitch = 0.0f;
        f32 diff = targetPitch - currentPitch;
        while (diff < -180.0f)
            diff += 360.0f;
        while (diff >= 180.0f)
            diff -= 360.0f;
        f32 newPitch = currentPitch + 0.2f * diff;
        m_fox->setRotation(m_fox->yaw(), newPitch);
    } else {
        f64 horizontalSpeed = std::sqrt(motion.x * motion.x + motion.z * motion.z);
        f64 speed = std::sqrt(horizontalSpeed * horizontalSpeed + motion.y * motion.y);

        if (speed > 0.001) {
            f64 pitchRad = std::acos(horizontalSpeed / speed);
            if (motion.y < 0) {
                pitchRad = -pitchRad;
            }
            f32 pitchDeg = static_cast<f32>(math::toDegrees(pitchRad));
            m_fox->setRotation(m_fox->yaw(), pitchDeg);
        }
    }

    if (m_target != nullptr && m_fox->distanceTo(*m_target) <= ATTACK_DISTANCE) {
        m_fox->attackEntityAsMob(*m_target);
        m_fox->playBiteSound();
    } else if (m_fox->pitch() > 0.0f && m_fox->onGround()) {
        Vector3 vel = m_fox->velocity();

        if (m_fox->world() != nullptr && vel.y != 0.0f) {
            BlockPos pos(static_cast<i32>(m_fox->x()), static_cast<i32>(m_fox->y()), static_cast<i32>(m_fox->z()));
            const BlockState* state = m_fox->world()->getBlockState(pos);

            if (state != nullptr && state->is(VanillaBlocks::SNOW)) {
                m_fox->setRotation(m_fox->yaw(), STUCK_PITCH_ANGLE);
                m_fox->setAttackTarget(nullptr);
                m_fox->setStuck(true);
            }
        }
    }
}

// ============================================================================
// FoxBiteGoal - 狐狸咬击目标
// ============================================================================

FoxBiteGoal::FoxBiteGoal(FoxEntity* fox, f64 speed, bool useLongMemory)
    : MeleeAttackGoal(fox, speed, useLongMemory)
    , m_foxEntity(fox)
{}

bool FoxBiteGoal::shouldExecute()
{
    if (m_foxEntity->isSitting() || m_foxEntity->isSleeping() || m_foxEntity->isCrouching() || m_foxEntity->isStuck()) {
        return false;
    }
    return MeleeAttackGoal::shouldExecute();
}

bool FoxBiteGoal::shouldContinueExecuting()
{
    if (m_foxEntity->isSitting() || m_foxEntity->isSleeping() || m_foxEntity->isCrouching() || m_foxEntity->isStuck()) {
        return false;
    }
    return MeleeAttackGoal::shouldContinueExecuting();
}

void FoxBiteGoal::startExecuting()
{
    m_foxEntity->setInterested(false);
    MeleeAttackGoal::startExecuting();
}

void FoxBiteGoal::checkAndPerformAttack(LivingEntity* enemy, f64 distToEnemySqr)
{
    f32 reachSq = getAttackReachSqr(enemy);

    if (distToEnemySqr <= reachSq && m_attackCooldown <= 0) {
        m_attackCooldown = ATTACK_COOLDOWN_TICKS;
        m_foxEntity->attackEntityAsMob(*enemy);
        m_foxEntity->playBiteSound();
    }
}

// ============================================================================
// FoxFindShelterGoal - 狐狸寻找庇护所目标（简化实现）
// ============================================================================

FoxFindShelterGoal::FoxFindShelterGoal(FoxEntity* fox, f64 speed)
    : FoxPassiveGoal(fox)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
    m_cooldown = fox->getRandom().nextInt(COOLDOWN_MAX - COOLDOWN_MIN + 1) + COOLDOWN_MIN;
}

bool FoxFindShelterGoal::canFoxStart()
{
    if (m_fox->attackTarget() != nullptr || m_fox->isSleeping()) {
        return false;
    }

    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return false;
    }

    if (world->isThundering()) {
        return true;
    }

    if (m_cooldown > 0) {
        m_cooldown--;
        return false;
    }

    m_cooldown = COOLDOWN_MAX;
    return world->isDaytime() && hasShelter();
}

bool FoxFindShelterGoal::canFoxContinue()
{
    return !m_fox->isSleeping() && m_hasShelter;
}

void FoxFindShelterGoal::startExecuting()
{
    m_fox->resetAllStates();
    m_hasShelter = false;
}

void FoxFindShelterGoal::resetTask()
{
    m_hasShelter = false;
}

void FoxFindShelterGoal::tick()
{
    if (m_hasShelter) {
        m_fox->tryMoveTo(m_shelterPos.x, m_shelterPos.y, m_shelterPos.z, m_speed);
    }
}

// ============================================================================
// FoxSleepGoal - 狐狸睡眠目标
// ============================================================================

FoxSleepGoal::FoxSleepGoal(FoxEntity* fox)
    : FoxPassiveGoal(fox)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
    m_cooldown = fox->getRandom().nextInt(COOLDOWN_MAX);
}

bool FoxSleepGoal::canFoxStart()
{
    if (m_cooldown > 0) {
        m_cooldown--;
        return false;
    }

    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return false;
    }

    return world->isDaytime() && hasShelter() && !hasAlertableTarget();
}

bool FoxSleepGoal::canFoxContinue()
{
    return canFoxStart() || m_fox->isSleeping();
}

void FoxSleepGoal::startExecuting()
{
    m_fox->setSitting(false);
    m_fox->setCrouching(false);
    m_fox->setInterested(false);
    m_fox->setJumping(false);
    m_fox->setSleeping(true);
    m_fox->clearNavigation();

    auto* moveController = m_fox->moveController();
    if (moveController != nullptr) {
        moveController->setMoveTo(m_fox->x(), m_fox->y(), m_fox->z(), 0.0);
    }
}

void FoxSleepGoal::resetTask()
{
    m_cooldown = m_fox->getRandom().nextInt(COOLDOWN_MAX);
    m_fox->resetAllStates();
}

// ============================================================================
// FoxEatBerriesGoal - 狐狸吃浆果目标
// ============================================================================

FoxEatBerriesGoal::FoxEatBerriesGoal(FoxEntity* fox, f64 speed, i32 searchRange, i32 verticalSearchRange)
    : m_fox(fox)
    , m_speed(speed)
    , m_searchRange(searchRange)
    , m_verticalSearchRange(verticalSearchRange)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool FoxEatBerriesGoal::shouldExecute()
{
    if (m_fox->isSleeping()) {
        return false;
    }

    // TODO: 完整实现需要检测甜浆果丛
    return false;
}

bool FoxEatBerriesGoal::shouldContinueExecuting()
{
    return false;
}

void FoxEatBerriesGoal::startExecuting() {}

void FoxEatBerriesGoal::resetTask() {}

void FoxEatBerriesGoal::tick() {}

// ============================================================================
// FoxFindItemsGoal - 狐狸寻找物品目标
// ============================================================================

FoxFindItemsGoal::FoxFindItemsGoal(FoxEntity* fox)
    : m_fox(fox)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool FoxFindItemsGoal::shouldExecute()
{
    // TODO: 完整实现需要检测附近的物品实体
    return false;
}

void FoxFindItemsGoal::startExecuting() {}

void FoxFindItemsGoal::tick() {}

// ============================================================================
// FoxSitAndLookGoal - 狐狸坐下观察目标
// ============================================================================

FoxSitAndLookGoal::FoxSitAndLookGoal(FoxEntity* fox)
    : FoxPassiveGoal(fox)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool FoxSitAndLookGoal::canFoxStart()
{
    math::Random rng = m_fox->getRandom();
    if (rng.nextFloat() >= TRIGGER_CHANCE) {
        return false;
    }

    if (m_fox->getLastHurtBy() != nullptr || m_fox->isSleeping() || m_fox->attackTarget() != nullptr) {
        return false;
    }

    auto* navigator = m_fox->navigator();
    if (navigator != nullptr && !navigator->noPath()) {
        return false;
    }

    if (hasAlertableTarget()) {
        return false;
    }

    if (m_fox->isPounceReady() || m_fox->isCrouching()) {
        return false;
    }

    return true;
}

bool FoxSitAndLookGoal::canFoxContinue()
{
    return m_lookCount > 0;
}

void FoxSitAndLookGoal::startExecuting()
{
    _chooseRandomLookDirection();

    math::Random rng = m_fox->getRandom();
    m_lookCount = LOOK_COUNT_MIN + rng.nextInt(LOOK_COUNT_MAX - LOOK_COUNT_MIN + 1);

    m_fox->setSitting(true);
    m_fox->clearNavigation();
}

void FoxSitAndLookGoal::resetTask()
{
    m_fox->setSitting(false);
}

void FoxSitAndLookGoal::tick()
{
    --m_lookTimer;

    if (m_lookTimer <= 0) {
        --m_lookCount;
        _chooseRandomLookDirection();
    }

    auto* lookController = m_fox->lookController();
    if (lookController != nullptr) {
        lookController->setLookPosition(m_fox->x() + m_lookX,
            m_fox->y() + m_fox->eyeHeight(),
            m_fox->z() + m_lookZ,
            static_cast<f32>(m_fox->getHorizontalFaceSpeed()),
            static_cast<f32>(m_fox->getVerticalFaceSpeed()));
    }
}

void FoxSitAndLookGoal::_chooseRandomLookDirection()
{
    math::Random rng = m_fox->getRandom();

    f64 angle = static_cast<f64>(math::TWO_PI) * rng.nextDouble();
    m_lookX = std::cos(angle);
    m_lookZ = std::sin(angle);

    m_lookTimer = LOOK_DURATION_MIN + rng.nextInt(LOOK_DURATION_MAX - LOOK_DURATION_MIN + 1);
}

// ============================================================================
// FoxRevengeGoal - 狐狸复仇目标
// ============================================================================

FoxRevengeGoal::FoxRevengeGoal(FoxEntity* fox)
    : entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>(fox, true, 10)
    , m_foxEntity(fox)
{}

bool FoxRevengeGoal::shouldExecute()
{
    // TODO: 完整实现需要检查信任玩家是否被攻击
    return false;
}

void FoxRevengeGoal::startExecuting()
{
    if (m_trustedEntity != nullptr) {
        m_revengeTimestamp = m_trustedEntity->lastHurtByTimestamp();
    }

    m_foxEntity->playSound(SoundEvents::ENTITY_FOX_AGGRO, 1.0f, 1.0f);
    m_foxEntity->setFoxAggroed(true);
    m_foxEntity->wakeUp();

    entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>::startExecuting();
}

} // namespace entity::ai::goal
} // namespace mc
