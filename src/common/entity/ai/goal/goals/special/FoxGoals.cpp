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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/cave/CaveVinesBlock.hpp"
#include "common/world/block/blocks/cave/CaveVinesPlantBlock.hpp"
#include "common/world/block/blocks/vegetation/SweetBerryBushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
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

        auto type = living->entityType();

        // 鸡、兔子 -> 警觉
        if (type == entity::VanillaEntityTypeKeys::CHICKEN || type == entity::VanillaEntityTypeKeys::RABBIT) {
            return true;
        }

        // 怪物类型 -> 警觉
        if (dynamic_cast<MonsterEntity*>(living) != nullptr) {
            return true;
        }

        // 玩家检查
        Player* player = dynamic_cast<Player*>(living);
        if (player != nullptr && !player->isSpectator() && !player->isCreative()) {
            if (!m_fox->trusts(player->playerId())) {
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

    auto type = target->entityType();
    if (type != entity::VanillaEntityTypeKeys::CHICKEN && type != entity::VanillaEntityTypeKeys::RABBIT) {
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

    // 狐狸已经叼着物品时不再寻找浆果
    if (m_fox->isHoldingItem()) {
        return false;
    }

    return _searchForTarget();
}

bool FoxEatBerriesGoal::shouldContinueExecuting()
{
    if (m_fox->isSleeping()) {
        return false;
    }

    // 如果狐狸已经叼着物品（可能从其他来源获得），停止
    if (m_fox->isHoldingItem()) {
        return false;
    }

    // 检查目标方块是否仍然有效
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return false;
    }

    return _isValidTarget(world, m_targetPos);
}

void FoxEatBerriesGoal::startExecuting()
{
    m_eatTimer = 0;
    m_reached = false;
    m_fox->setSitting(false);

    // 导航到目标位置
    _moveToTarget();
}

void FoxEatBerriesGoal::resetTask()
{
    m_targetPos = BlockPos(0, 0, 0);
    m_eatTimer = 0;
    m_reached = false;
    m_fox->clearNavigation();
}

void FoxEatBerriesGoal::tick()
{
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return;
    }

    // 检查是否已到达目标附近
    f32 distSq = m_fox->distanceSqTo(static_cast<f32>(m_targetPos.x) + 0.5f,
        static_cast<f32>(m_targetPos.y),
        static_cast<f32>(m_targetPos.z) + 0.5f);

    if (distSq <= REACH_DISTANCE_SQ) {
        // 已到达目标附近
        m_reached = true;

        if (m_eatTimer >= EAT_DURATION) {
            // 吃完浆果
            _eatBerry();
        } else {
            m_eatTimer++;
        }
    } else {
        // 尚未到达，继续导航
        m_reached = false;

        // 5% 概率播放嗅探音效
        if (m_fox->getRandom().nextFloat() < 0.05f) {
            m_fox->playSniffSound();
        }

        // 定期重新导航
        if (m_fox->getRandom().nextInt(40) == 0) {
            _moveToTarget();
        }
    }
}

bool FoxEatBerriesGoal::_isValidTarget(const IWorld* world, const BlockPos& pos) const
{
    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    const Block* block = &state->getBlock();

    // 甜浆果丛：AGE >= 2（有浆果可采摘）
    if (block == VanillaBlocks::SWEET_BERRY_BUSH) {
        const auto* sweetBerry = static_cast<const blocks::SweetBerryBushBlock*>(block);
        return sweetBerry->getAge(*state) >= 2;
    }

    // 洞穴藤蔓：BERRIES == true
    if (block == VanillaBlocks::CAVE_VINES || block == VanillaBlocks::CAVE_VINES_PLANT) {
        return state->get(BlockStateProperties::BERRIES());
    }

    return false;
}

void FoxEatBerriesGoal::_eatBerry()
{
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return;
    }

    // 当 mobGriefing 为 false 时，狐狸不会采摘浆果方块
    if (!world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return;
    }

    const BlockState* state = world->getBlockState(m_targetPos);
    if (state == nullptr) {
        return;
    }

    const Block* block = &state->getBlock();

    if (block == VanillaBlocks::SWEET_BERRY_BUSH) {
        _pickSweetBerries(*state);
    } else if (block == VanillaBlocks::CAVE_VINES || block == VanillaBlocks::CAVE_VINES_PLANT) {
        _pickGlowBerry(*state);
    }
}

void FoxEatBerriesGoal::_pickSweetBerries(const BlockState& state)
{
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return;
    }

    const auto* sweetBerry = static_cast<const blocks::SweetBerryBushBlock*>(&state.getBlock());
    i32 age = sweetBerry->getAge(state);
    bool fullyGrown = sweetBerry->isMaxAge(state);
    math::Random& rng = m_fox->getRandom();

    // 计算掉落数量：1 + random(0~1) + (fullyGrown ? 1 : 0)
    i32 berryCount = 1 + rng.nextInt(2) + (fullyGrown ? 1 : 0);

    // 如果主手为空，给狐狸装备1个甜浆果
    if (!m_fox->isHoldingItem()) {
        if (Items::SWEET_BERRIES != nullptr) {
            m_fox->setHeldItem(std::make_unique<ItemStack>(*Items::SWEET_BERRIES, 1));
        }
        berryCount--;
    }

    // 剩余的浆果以物品形式弹出
    if (berryCount > 0 && Items::SWEET_BERRIES != nullptr) {
        Vector3 centerPos = m_targetPos.center();
        ItemStack dropStack(*Items::SWEET_BERRIES, berryCount);
        ItemDropHelper::spawnItemEntity(world,
            dropStack,
            static_cast<f64>(centerPos.x),
            static_cast<f64>(centerPos.y),
            static_cast<f64>(centerPos.z),
            rng,
            ItemEntity::DEFAULT_PICKUP_DELAY);
    }

    // 播放采摘音效
    m_fox->playSound(SoundEvents::BLOCK_SWEET_BERRY_BUSH_BREAK, 1.0f, 1.0f);

    // 将 AGE 重置为 1
    const BlockState& newState = sweetBerry->withAge(state, 1);
    world->setBlockState(m_targetPos, &newState, 2);

    // 重置目标
    m_eatTimer = 0;
    m_reached = false;
}

void FoxEatBerriesGoal::_pickGlowBerry(const BlockState& state)
{
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return;
    }

    math::Random& rng = m_fox->getRandom();

    // 如果主手为空，给狐狸装备1个发光浆果
    if (!m_fox->isHoldingItem()) {
        if (Items::GLOW_BERRIES != nullptr) {
            m_fox->setHeldItem(std::make_unique<ItemStack>(*Items::GLOW_BERRIES, 1));
        }
    } else {
        // 主手已有物品，直接掉落1个发光浆果
        if (Items::GLOW_BERRIES != nullptr) {
            Vector3 centerPos = m_targetPos.center();
            ItemStack dropStack(*Items::GLOW_BERRIES, 1);
            ItemDropHelper::spawnItemEntity(world,
                dropStack,
                static_cast<f64>(centerPos.x),
                static_cast<f64>(centerPos.y),
                static_cast<f64>(centerPos.z),
                rng,
                ItemEntity::DEFAULT_PICKUP_DELAY);
        }
    }

    // 播放采摘音效
    f32 pitch = 0.8f + rng.nextFloat() * 0.4f;
    m_fox->playSound(SoundEvents::BLOCK_CAVE_VINES_PICK_BERRIES, 1.0f, pitch);

    // 设置 BERRIES = false（藤蔓保留）
    const BlockState& newState = state.with(BlockStateProperties::BERRIES(), false);
    world->setBlockState(m_targetPos, &newState, 2);

    // 重置目标
    m_eatTimer = 0;
    m_reached = false;
    m_eatTimer = 0;
    m_reached = false;
}

bool FoxEatBerriesGoal::_searchForTarget()
{
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return false;
    }

    // 螺旋搜索附近的方块
    i32 foxX = static_cast<i32>(m_fox->x());
    i32 foxY = static_cast<i32>(m_fox->y());
    i32 foxZ = static_cast<i32>(m_fox->z());

    for (i32 yOff = -m_verticalSearchRange; yOff <= m_verticalSearchRange; ++yOff) {
        for (i32 xOff = -m_searchRange; xOff <= m_searchRange; ++xOff) {
            for (i32 zOff = -m_searchRange; zOff <= m_searchRange; ++zOff) {
                BlockPos pos(foxX + xOff, foxY + yOff, foxZ + zOff);
                if (_isValidTarget(world, pos)) {
                    m_targetPos = pos;
                    return true;
                }
            }
        }
    }

    return false;
}

void FoxEatBerriesGoal::_moveToTarget()
{
    // 导航到目标方块的上方
    m_fox->tryMoveTo(static_cast<f64>(m_targetPos.x) + 0.5,
        static_cast<f64>(m_targetPos.y),
        static_cast<f64>(m_targetPos.z) + 0.5,
        m_speed);
}

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
    // 主手已有物品时不搜索
    if (m_fox->isHoldingItem()) {
        return false;
    }

    // 有攻击目标或正在被攻击时不搜索
    if (m_fox->attackTarget() != nullptr || m_fox->getLastHurtBy() != nullptr) {
        return false;
    }

    // 不能行动时不搜索（坐着、蹲着、睡觉、卡住、激怒）
    if (!m_fox->canAct()) {
        return false;
    }

    // 1/10 概率触发
    if (m_fox->getRandom().nextInt(CHANCE) != 0) {
        return false;
    }

    // 搜索 8 格范围内的物品实体
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return false;
    }

    AxisAlignedBB searchBox = m_fox->boundingBox().expand(
        static_cast<f32>(SEARCH_RADIUS), static_cast<f32>(SEARCH_RADIUS), static_cast<f32>(SEARCH_RADIUS));

    std::vector<Entity*> nearbyEntities = world->getEntitiesInAABB(searchBox, m_fox);

    for (Entity* entity : nearbyEntities) {
        // 只关注物品实体
        if (entity->entityType() != entity::VanillaEntityTypeKeys::ITEM) {
            continue;
        }

        auto* itemEntity = static_cast<ItemEntity*>(entity);

        // 过滤条件：没有拾取延迟且存活
        if (!itemEntity->canBePickedUp()) {
            continue;
        }

        // 找到可拾取的物品，确认主手仍为空
        if (!m_fox->isHoldingItem()) {
            return true;
        }
    }

    return false;
}

void FoxFindItemsGoal::startExecuting()
{
    // 导航到最近的物品实体
    ItemEntity* nearestItem = _findNearestItem();
    if (nearestItem != nullptr) {
        m_fox->tryMoveTo(nearestItem->x(), nearestItem->y(), nearestItem->z(), MOVE_SPEED);
    }
}

void FoxFindItemsGoal::tick()
{
    // 持续导航到最近的物品
    if (m_fox->isHoldingItem()) {
        return;
    }

    ItemEntity* nearestItem = _findNearestItem();
    if (nearestItem != nullptr) {
        m_fox->tryMoveTo(nearestItem->x(), nearestItem->y(), nearestItem->z(), MOVE_SPEED);
    }
}

ItemEntity* FoxFindItemsGoal::_findNearestItem() const
{
    IWorld* world = m_fox->world();
    if (world == nullptr) {
        return nullptr;
    }

    AxisAlignedBB searchBox = m_fox->boundingBox().expand(
        static_cast<f32>(SEARCH_RADIUS), static_cast<f32>(SEARCH_RADIUS), static_cast<f32>(SEARCH_RADIUS));

    std::vector<Entity*> nearbyEntities = world->getEntitiesInAABB(searchBox, m_fox);

    ItemEntity* nearestItem = nullptr;
    f64 nearestDistSq = SEARCH_RADIUS * SEARCH_RADIUS;

    for (Entity* entity : nearbyEntities) {
        // 只关注物品实体
        if (entity->entityType() != entity::VanillaEntityTypeKeys::ITEM) {
            continue;
        }

        auto* itemEntity = static_cast<ItemEntity*>(entity);

        // 过滤条件：没有拾取延迟且存活
        if (!itemEntity->canBePickedUp()) {
            continue;
        }

        f64 distSq = m_fox->distanceSqTo(*itemEntity);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestItem = itemEntity;
        }
    }

    return nearestItem;
}

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
    math::Random& rng = m_fox->getRandom();
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

    math::Random& rng = m_fox->getRandom();
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
    math::Random& rng = m_fox->getRandom();

    f64 angle = static_cast<f64>(math::TWO_PI) * rng.nextDouble();
    m_lookX = std::cos(angle);
    m_lookZ = std::sin(angle);

    m_lookTimer = LOOK_DURATION_MIN + rng.nextInt(LOOK_DURATION_MAX - LOOK_DURATION_MIN + 1);
}

// ============================================================================
// FoxStuckInSnowGoal - 狐狸卡在雪中目标
// ============================================================================

FoxStuckInSnowGoal::FoxStuckInSnowGoal(FoxEntity* fox)
    : m_fox(fox)
{
    // 卡住期间禁止移动、观察和跳跃
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
}

bool FoxStuckInSnowGoal::shouldExecute()
{
    // 当狐狸处于卡住状态时激活
    return m_fox->isStuck();
}

bool FoxStuckInSnowGoal::shouldContinueExecuting()
{
    // 持续执行直到不再卡住或倒计时结束
    return m_fox->isStuck() && m_countdown > 0;
}

void FoxStuckInSnowGoal::startExecuting()
{
    // 设置卡住持续时间（40 tick = 2 秒）
    // 对应 MC Java: this.countdown = this.adjustedTickDelay(40);
    m_countdown = STUCK_DURATION;
}

void FoxStuckInSnowGoal::resetTask()
{
    // 目标结束时清除卡住状态，狐狸恢复自由
    m_fox->setStuck(false);
}

void FoxStuckInSnowGoal::tick()
{
    m_countdown--;
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
    if (!m_foxEntity) return false;

    IWorld* world = m_foxEntity->world();
    if (!world) return false;

    const auto& trustedPlayers = m_foxEntity->getTrustedPlayers();
    if (trustedPlayers.empty()) return false;

    // 使用 getPlayers() 遍历玩家列表（通常仅 1~10 名玩家），
    // 比 getEntitiesInAABB() 的 64 格全实体搜索高效得多
    std::vector<Entity*> players = world->getPlayers();

    for (u64 trustedPlayerId : trustedPlayers) {
        Player* trustedPlayer = nullptr;
        for (Entity* entity : players) {
            Player* player = dynamic_cast<Player*>(entity);
            if (player && player->playerId() == trustedPlayerId && player->isAlive()) {
                trustedPlayer = player;
                break;
            }
        }

        if (trustedPlayer == nullptr) continue;

        LivingEntity* attacker = trustedPlayer->getLastHurtBy();
        if (attacker == nullptr || !attacker->isAlive()) continue;

        if (attacker == m_foxEntity) continue;

        if (attacker == trustedPlayer) continue;

        i32 hurtTimestamp = trustedPlayer->lastHurtByTimestamp();
        if (hurtTimestamp == m_revengeTimestamp) continue;

        if (!isSuitableTarget(attacker)) continue;

        const Player* attackerPlayer = dynamic_cast<const Player*>(attacker);
        if (attackerPlayer != nullptr && m_foxEntity->trusts(attackerPlayer->playerId())) {
            continue;
        }

        m_attackerOfTrusted = attacker;
        m_trustedEntity = trustedPlayer;
        m_target = attacker;
        return true;
    }

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
