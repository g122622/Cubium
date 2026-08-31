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

#pragma once

#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/IPositionTarget.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/entity/ai/brain/task/Task.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>
#include <string>
#include <vector>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace movement {

using namespace memory;

// TODO: 当前所有任务直接调用 owner->brain()，未检查空指针。
// 这是因为模板参数 E 必须是拥有 brain() 方法的实体类型（如 VillagerEntity），
// 但后续如果需要在可能没有 Brain 的实体上使用这些任务，需要添加空指针保护。

/**
 * @brief 移动到行走目标任务（对应 MC MoveToTargetSink）
 *
 * 读取 WALK_TARGET 记忆，执行实际寻路和移动。
 * 这是 Brain 系统中所有行走移动的核心执行器——
 * 其他任务只需设置 WALK_TARGET 记忆，本任务负责真正导航。
 */
template <typename E>
class MoveToTargetTask : public Task<E> {
public:
    MoveToTargetTask()
        : Task<E>({{MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::PATH, MemoryModuleStatus::REGISTERED}},
              60,
              200)
        , m_lastPathRecalcTime(0)
        , m_stuckCheckTime(0)
        , m_stuckCheckX(0.0)
        , m_stuckCheckZ(0.0)
    {}

    std::string getName() const override { return "MoveToTargetTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto walkTarget = brain.template getMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        if (!walkTarget.has_value()) {
            return false;
        }

        // 如果已经在完成范围内，不需要移动
        const auto& target = walkTarget->getTarget();
        f32 distSq = owner->distanceSqTo(target->getPosition().x, target->getPosition().y, target->getPosition().z);
        f32 closeEnough = static_cast<f32>(walkTarget->getDistance());
        if (distSq <= closeEnough * closeEnough) {
            return false;
        }

        // 冷却检查：如果最近无法到达行走目标，短暂冷却
        auto cantReachSince = brain.template getMemory<i64>(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
        if (cantReachSince.has_value()) {
            // 冷却 40 tick（2秒）
            i64 gameTime = world->currentTick();
            if (gameTime - cantReachSince.value() < 40) {
                return false;
            }
            // 超过冷却时间，清除无法到达标记
            brain.template removeMemory<i64>(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
        }

        return true;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto walkTarget = brain.template getMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        if (!walkTarget.has_value()) {
            return false;
        }

        // 导航器必须还在路径上
        auto* navigator = owner->navigator();
        if (!navigator || navigator->noPath()) {
            return false;
        }

        // 检查是否已到达目标
        const auto& target = walkTarget->getTarget();
        f32 distSq = owner->distanceSqTo(target->getPosition().x, target->getPosition().y, target->getPosition().z);
        f32 closeEnough = static_cast<f32>(walkTarget->getDistance());
        return distSq > closeEnough * closeEnough;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto walkTarget = brain.template getMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        if (!walkTarget.has_value()) {
            return;
        }

        m_lastPathRecalcTime = gameTime;
        m_stuckCheckTime = gameTime;
        m_stuckCheckX = static_cast<f64>(owner->x());
        m_stuckCheckZ = static_cast<f64>(owner->z());

        // 尝试计算路径
        _tryComputePath(world, owner, *walkTarget);
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto walkTarget = brain.template getMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        if (!walkTarget.has_value()) {
            return;
        }

        // 看向目标
        const auto& target = walkTarget->getTarget();
        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPosition(target->getPosition());
        }

        // 定期重新计算路径
        if (gameTime - m_lastPathRecalcTime >= 10) {
            m_lastPathRecalcTime = gameTime;

            const auto& targetPos = target->getPosition();
            auto* navigator = owner->navigator();
            if (navigator) {
                // 检查目标是否变化超过2格曼哈顿距离
                const auto* path = navigator->getPath();
                if (path && !path->empty()) {
                    const auto& pathTarget = path->getTarget();
                    i32 manhattan = std::abs(static_cast<i32>(targetPos.x) - pathTarget.x) +
                        std::abs(static_cast<i32>(targetPos.y) - pathTarget.y) +
                        std::abs(static_cast<i32>(targetPos.z) - pathTarget.z);
                    if (manhattan > 2) {
                        _tryComputePath(world, owner, *walkTarget);
                    }
                }
            }
        }

        // 卡住检测
        if (gameTime - m_stuckCheckTime >= 100) {
            f64 dx = static_cast<f64>(owner->x()) - m_stuckCheckX;
            f64 dz = static_cast<f64>(owner->z()) - m_stuckCheckZ;
            if (dx * dx + dz * dz < 2.25) {
                // 卡住了，清除行走目标
                brain.template removeMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
                return;
            }
            m_stuckCheckTime = gameTime;
            m_stuckCheckX = static_cast<f64>(owner->x());
            m_stuckCheckZ = static_cast<f64>(owner->z());
        }

        // 同步路径到 PATH 记忆
        auto* pathNav = owner->navigator();
        if (pathNav && pathNav->hasPath()) {
            brain.template setMemory<pathfinding::Path>(MemoryModuleTypes::PATH, *pathNav->getPath());
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }

        auto& brain = owner->brain();

        // 如果导航卡住，设置冷却标记
        auto* navigator2 = owner->navigator();
        if (navigator2 && navigator2->isStuck()) {
            brain.template setMemoryWithTTL<i64>(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE, gameTime + 40, 100);
        }

        // 清除 PATH 记忆
        brain.template removeMemory<pathfinding::Path>(MemoryModuleTypes::PATH);
    }

private:
    void _tryComputePath(IWorld* world, E* owner, const WalkTarget& walkTarget)
    {
        auto* navigator = owner->navigator();
        if (!navigator) {
            return;
        }

        const auto& target = walkTarget.getTarget();
        f32 speed = walkTarget.getSpeed();
        i32 closeEnough = walkTarget.getDistance();

        // 首先尝试直接寻路到目标
        bool pathFound = navigator->moveToRange(static_cast<f64>(target->getPosition().x),
            static_cast<f64>(target->getPosition().y),
            static_cast<f64>(target->getPosition().z),
            static_cast<f32>(closeEnough),
            static_cast<f64>(speed));

        // 如果直接寻路失败，尝试向目标方向偏移找替代路径
        if (!pathFound) {
            Vector3 ownerPos = owner->position();
            Vector3 targetPos = target->getPosition();
            f64 dx = static_cast<f64>(targetPos.x - ownerPos.x);
            f64 dz = static_cast<f64>(targetPos.z - ownerPos.z);
            f64 dist = std::sqrt(dx * dx + dz * dz);
            if (dist > 0.0) {
                dx /= dist;
                dz /= dist;
                // 在目标方向附近随机偏移尝试
                math::Random& rng = owner->getRandom();
                for (i32 attempt = 0; attempt < 10; ++attempt) {
                    f64 offsetX = (rng.nextDouble() - 0.5) * 10.0;
                    f64 offsetZ = (rng.nextDouble() - 0.5) * 10.0;
                    f64 targetX = static_cast<f64>(targetPos.x) + offsetX;
                    f64 targetZ = static_cast<f64>(targetPos.z) + offsetZ;

                    pathFound = navigator->moveToRange(targetX,
                        static_cast<f64>(targetPos.y),
                        targetZ,
                        static_cast<f32>(closeEnough),
                        static_cast<f64>(speed));
                    if (pathFound) {
                        break;
                    }
                }
            }
        }

        // 如果所有尝试都失败，设置无法到达标记
        if (!pathFound) {
            auto& brain = owner->brain();
            brain.template setMemoryWithTTL<i64>(
                MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE, world->currentTick(), 200);
        }
    }

    i64 m_lastPathRecalcTime;
    i64 m_stuckCheckTime;
    f64 m_stuckCheckX;
    f64 m_stuckCheckZ;
};

/**
 * @brief 随机漫步任务（对应 MC Stroll / RandomStroll）
 *
 * 当 WALK_TARGET 不存在时，随机选择一个行走目标并设置到记忆中。
 * 实际移动由 MoveToTargetTask 执行。
 */
template <typename E>
class StrollTask : public Task<E> {
public:
    explicit StrollTask(f32 speed = 1.0f, i32 interval = 40)
        : Task<E>({{MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT}}, interval, interval * 2)
        , m_speed(speed)
        , m_xzRange(static_cast<i32>(goal::constants::RANDOM_WALK_RANGE))
        , m_yRange(static_cast<i32>(goal::constants::RANDOM_WALK_VERTICAL_RANGE))
    {}

    std::string getName() const override { return "StrollTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // WALK_TARGET 缺失时才能触发（已通过 requiredMemoryState 保证）
        // 随机概率触发
        return owner->getRandom().nextInt(goal::constants::DEFAULT_WALK_CHANCE) == 0;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // 生成随机目标位置
        Vector3 targetPos;
        auto* creature = dynamic_cast<CreatureEntity*>(owner);
        if (!creature) {
            // TODO: StrollTask 要求实体为 CreatureEntity 类型以使用 RandomPositionGenerator，
            // 非 CreatureEntity（如飞行实体）需要其他漫步策略。当前静默跳过。
            return;
        }
        if (util::RandomPositionGenerator::findRandomTarget(creature, m_xzRange, m_yRange, targetPos)) {
            // 设置 WALK_TARGET 记忆，由 MoveToTargetTask 执行实际移动
            owner->brain().template setMemory<WalkTarget>(
                MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(targetPos), m_speed, 1));
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 不需要清除 WALK_TARGET，因为本任务是设置目标的，
        // 清除由 MoveToTargetTask 到达目标或超时后处理
    }

private:
    f32 m_speed;
    i32 m_xzRange;
    i32 m_yRange;
};

/**
 * @brief 看向实体任务（对应 MC SetEntityLookTarget / LookAtTargetSink）
 *
 * 概率触发时，在附近寻找实体并设置 LOOK_TARGET 记忆。
 * 如果目标仍可见，则持续看向目标。
 */
template <typename E>
class LookAtEntityTask : public Task<E> {
public:
    LookAtEntityTask(
        f32 range = goal::constants::LOOK_AT_MAX_DISTANCE, f32 probability = goal::constants::DEFAULT_LOOK_CHANCE)
        : Task<E>({{MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::VALUE_ABSENT}},
              goal::constants::LOOK_AT_MIN_TIME,
              goal::constants::LOOK_AT_MAX_TIME)
        , m_range(range)
        , m_probability(probability)
    {}

    std::string getName() const override { return "LookAtEntityTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // 随机概率触发
        if (owner->getRandom().nextFloat() >= m_probability) {
            return false;
        }

        // 优先看向攻击目标（记忆存 id，反查后校验存活）
        auto attackTargetId = owner->brain().template getMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        if (attackTargetId.has_value() && world != nullptr) {
            Entity* target = world->getEntity(*attackTargetId);
            if (target != nullptr && target->isAlive()) {
                m_lookTargetId = *attackTargetId;
                return true;
            }
        }

        // 搜索附近的可见实体（VISIBLE_MOBS 存 id 列表）
        auto visibleMobIds =
            owner->brain().template getMemory<std::vector<EntityInstanceId>>(MemoryModuleTypes::VISIBLE_MOBS);
        if (visibleMobIds.has_value() && !visibleMobIds->empty() && world != nullptr) {
            f32 rangeSq = m_range * m_range;
            // 在范围内随机选择一个实体
            std::vector<EntityInstanceId> candidates;
            for (EntityInstanceId mobId : *visibleMobIds) {
                // id 反查 + 存活校验：实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针
                Entity* mob = world->getEntity(mobId);
                if (mob == nullptr || !mob->isAlive()) {
                    continue;
                }
                f32 distSq = owner->distanceSqTo(*mob);
                if (distSq <= rangeSq) {
                    candidates.push_back(mobId);
                }
            }
            if (!candidates.empty()) {
                m_lookTargetId = candidates[owner->getRandom().nextInt(static_cast<i32>(candidates.size()))];
                return true;
            }
        }

        // 如果没有可见实体列表，尝试从世界获取
        if (world) {
            auto entities = world->getEntitiesInRange(owner->position(), m_range, owner);
            if (!entities.empty()) {
                // 筛选 LivingEntity
                std::vector<LivingEntity*> livingEntities;
                for (auto* entity : entities) {
                    auto* living = dynamic_cast<LivingEntity*>(entity);
                    if (living && living->isAlive()) {
                        livingEntities.push_back(living);
                    }
                }
                if (!livingEntities.empty()) {
                    m_lookTargetId =
                        livingEntities[owner->getRandom().nextInt(static_cast<i32>(livingEntities.size()))]->id();
                    return true;
                }
            }
        }

        return false;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        if (m_lookTargetId == INVALID_ENTITY_ID) {
            return false;
        }

        // 检查目标是否仍在范围内
        if (world != nullptr) {
            Entity* target = world->getEntity(m_lookTargetId);
            if (target != nullptr && target->isAlive()) {
                f32 distSq = owner->distanceSqTo(*target);
                return distSq <= m_range * m_range;
            }
        }

        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (m_lookTargetId == INVALID_ENTITY_ID || world == nullptr) {
            return;
        }

        // 看向目标的眼睛位置
        if (Entity* target = world->getEntity(m_lookTargetId); target != nullptr) {
            if (auto* lookCtrl = owner->lookController()) {
                lookCtrl->setLookPositionWithEntity(
                    *target, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
            }
        }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        if (m_lookTargetId == INVALID_ENTITY_ID || world == nullptr) {
            return;
        }

        // 持续看向目标
        if (Entity* target = world->getEntity(m_lookTargetId); target != nullptr) {
            if (auto* lookCtrl = owner->lookController()) {
                lookCtrl->setLookPositionWithEntity(
                    *target, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override { m_lookTargetId = INVALID_ENTITY_ID; }

private:
    f32 m_range;
    f32 m_probability;
    EntityInstanceId m_lookTargetId = INVALID_ENTITY_ID;
};

/**
 * @brief 寻找隐蔽点任务（对应 MC LocateHidingPlace / HideInHome）
 *
 * 当需要隐蔽时，搜索附近的床或安全区域，设置 HIDING_PLACE 和 WALK_TARGET 记忆。
 * 实际移动由 MoveToTargetTask 执行。
 */
template <typename E>
class FindHiddenBlockTask : public Task<E> {
public:
    FindHiddenBlockTask(f32 speed = 1.0f)
        : Task<E>({{MemoryModuleTypes::HIDING_PLACE, MemoryModuleStatus::VALUE_ABSENT},
                      {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT}},
              60,
              120)
        , m_speed(speed)
    {}

    std::string getName() const override { return "FindHiddenBlockTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // 检查是否需要隐蔽（被伤害、听到铃声等）
        auto& brain = owner->brain();
        auto hurtBy = brain.template getMemory<DamageSource*>(MemoryModuleTypes::HURT_BY);
        auto heardBell = brain.template getMemory<i64>(MemoryModuleTypes::HEARD_BELL_TIME);

        return hurtBy.has_value() || heardBell.has_value();
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();

        // 尝试从 HOME 记忆获取家位置
        auto home = brain.template getMemory<GlobalPos>(MemoryModuleTypes::HOME);
        BlockPos hiddenPos;
        bool found = false;

        if (home.has_value()) {
            // 使用家位置作为隐蔽点
            if (home->getDimensionId() == owner->dimension()) {
                hiddenPos = home->getPos();
                found = true;
            }
        }

        // 如果没有家位置，尝试寻找最近的床
        if (!found) {
            auto nearestBed = brain.template getMemory<BlockPos>(MemoryModuleTypes::NEAREST_BED);
            if (nearestBed.has_value()) {
                hiddenPos = *nearestBed;
                found = true;
            }
        }

        if (found) {
            // 设置隐蔽点记忆
            brain.template setMemory<BlockPos>(MemoryModuleTypes::HIDING_PLACE, hiddenPos);

            // 设置行走目标
            f32 distSq = owner->distanceSqTo(
                static_cast<f32>(hiddenPos.x), static_cast<f32>(hiddenPos.y), static_cast<f32>(hiddenPos.z));

            // 如果距离较远，设置行走目标
            if (distSq > 4.0f * 4.0f) {
                brain.template setMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET, WalkTarget(hiddenPos, m_speed, 2));
            }
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }
        // 不清除 HIDING_PLACE 记忆，让其他逻辑决定何时清除
    }

private:
    f32 m_speed;
};

/**
 * @brief 追逐攻击目标任务（对应 MC SetWalkTargetFromAttackTargetIfTargetOutOfRange + MeleeAttack）
 *
 * 读取 ATTACK_TARGET 记忆，追踪攻击目标并导航接近。
 * 实际移动由 MoveToTargetTask 执行（本任务设置 WALK_TARGET）。
 */
template <typename E>
class ChaseTask : public Task<E> {
public:
    ChaseTask(f32 speed = 1.5f, f32 stopDistance = 2.0f)
        : Task<E>({{MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED},
                      {MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::REGISTERED}},
              60,
              600)
        , m_speed(speed)
        , m_stopDistance(stopDistance)
    {}

    std::string getName() const override { return "ChaseTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto attackTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTargetId.has_value() || *attackTargetId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* target = (world != nullptr) ? world->getEntity(*attackTargetId) : nullptr;
        if (target == nullptr || !target->isAlive()) {
            return false;
        }

        // 检查目标是否超出追逐距离
        f32 distSq = owner->distanceSqTo(*target);
        return distSq > m_stopDistance * m_stopDistance;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto attackTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTargetId.has_value() || *attackTargetId == INVALID_ENTITY_ID) {
            return false;
        }

        Entity* target = (world != nullptr) ? world->getEntity(*attackTargetId) : nullptr;
        if (target == nullptr || !target->isAlive()) {
            return false;
        }

        // 超出最大追逐距离时放弃
        f32 distSq = owner->distanceSqTo(*target);
        if (distSq > goal::constants::MELEE_ATTACK_STOP_DISTANCE * goal::constants::MELEE_ATTACK_STOP_DISTANCE) {
            return false;
        }

        // 到达停止距离内则停止追逐
        return distSq > m_stopDistance * m_stopDistance;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override { _updateWalkAndLookTarget(world, owner); }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override { _updateWalkAndLookTarget(world, owner); }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 清除行走目标
        owner->brain().template removeMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
    }

private:
    void _updateWalkAndLookTarget(IWorld* world, E* owner)
    {
        auto& brain = owner->brain();
        auto attackTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTargetId.has_value() || *attackTargetId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* targetEntity = (world != nullptr) ? world->getEntity(*attackTargetId) : nullptr;
        if (targetEntity == nullptr || !targetEntity->isAlive()) {
            return;
        }
        LivingEntity* target = dynamic_cast<LivingEntity*>(targetEntity);
        if (target == nullptr) {
            return;
        }

        // 看向攻击目标
        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *target, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        // 设置或更新行走目标
        f32 distSq = owner->distanceSqTo(*target);
        if (distSq > m_stopDistance * m_stopDistance) {
            brain.template setMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET,
                WalkTarget(BlockPos(target->position()), m_speed, static_cast<i32>(m_stopDistance)));
        } else {
            // 已到达攻击范围，清除行走目标
            brain.template removeMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        }
    }

    f32 m_speed;
    f32 m_stopDistance;
};

/**
 * @brief 避险任务（对应 MC SetWalkTargetAwayFrom / AnimalPanic）
 *
 * 读取 AVOID_TARGET 记忆，计算远离威胁的方向，设置 WALK_TARGET。
 * 实际移动由 MoveToTargetTask 执行。
 */
template <typename E>
class FleeTask : public Task<E> {
public:
    FleeTask(f32 speed = 1.5f, f32 fleeDistance = goal::constants::AVOID_ESCAPE_RANGE)
        : Task<E>({{MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT}},
              60,
              200)
        , m_speed(speed)
        , m_fleeDistance(fleeDistance)
    {}

    std::string getName() const override { return "FleeTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto avoidTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::AVOID_TARGET);
        if (!avoidTargetId.has_value() || *avoidTargetId == INVALID_ENTITY_ID) {
            return false;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr，不再解引用悬垂指针）
        Entity* threat = (world != nullptr) ? world->getEntity(*avoidTargetId) : nullptr;
        if (threat == nullptr || !threat->isAlive()) {
            return false;
        }

        // 检查威胁是否在检测范围内
        f32 distSq = owner->distanceSqTo(*threat);
        return distSq <= goal::constants::AVOID_DETECTION_RANGE * goal::constants::AVOID_DETECTION_RANGE;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override { _setFleeTarget(world, owner); }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 持续更新逃跑方向
        _setFleeTarget(world, owner);
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }
        // 清除逃避目标记忆
        owner->brain().template removeMemory<EntityInstanceId>(MemoryModuleTypes::AVOID_TARGET);
    }

private:
    void _setFleeTarget(IWorld* world, E* owner)
    {
        auto& brain = owner->brain();
        auto avoidTargetId = brain.template getMemory<EntityInstanceId>(MemoryModuleTypes::AVOID_TARGET);
        if (!avoidTargetId.has_value() || *avoidTargetId == INVALID_ENTITY_ID) {
            return;
        }

        // id 反查 + 存活校验（实体析构后 getEntity 返回 nullptr）
        Entity* threatEntity = (world != nullptr) ? world->getEntity(*avoidTargetId) : nullptr;
        if (threatEntity == nullptr || !threatEntity->isAlive()) {
            return;
        }
        LivingEntity* threat = dynamic_cast<LivingEntity*>(threatEntity);
        if (threat == nullptr) {
            return;
        }

        // 使用 RandomPositionGenerator 生成远离威胁的位置
        // TODO: FleeTask 要求 CreatureEntity 类型以使用 RandomPositionGenerator，
        // 非 CreatureEntity 将使用下方的备用反方向计算
        auto* creature = dynamic_cast<CreatureEntity*>(owner);
        if (creature) {
            Vector3 fleePos;
            if (util::RandomPositionGenerator::findRandomTargetBlockAwayFrom(creature,
                    static_cast<i32>(goal::constants::AVOID_ESCAPE_RANGE),
                    static_cast<i32>(goal::constants::RANDOM_WALK_VERTICAL_RANGE),
                    threat->position(),
                    fleePos)) {
                // 验证逃跑位置比当前位置更远离威胁
                f64 threatToFleeSq = static_cast<f64>(threat->distanceSqTo(fleePos.x, fleePos.y, fleePos.z));
                f64 threatToOwnerSq = static_cast<f64>(threat->distanceSqTo(*owner));
                if (threatToFleeSq >= threatToOwnerSq) {
                    brain.template setMemory<WalkTarget>(
                        MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(fleePos), m_speed, 1));
                    return;
                }
            }
        }

        // 备用方案：计算反方向移动
        Vector3 ownerPos = owner->position();
        Vector3 threatPos = threat->position();
        f64 dx = static_cast<f64>(ownerPos.x - threatPos.x);
        f64 dz = static_cast<f64>(ownerPos.z - threatPos.z);
        f64 dist = std::sqrt(dx * dx + dz * dz);
        if (dist > 0.0) {
            dx /= dist;
            dz /= dist;
        } else {
            // 随机方向
            f64 angle = owner->getRandom().nextDouble() * 2.0 * math::PI;
            dx = std::cos(angle);
            dz = std::sin(angle);
        }

        f64 fleeX = static_cast<f64>(ownerPos.x) + dx * m_fleeDistance;
        f64 fleeZ = static_cast<f64>(ownerPos.z) + dz * m_fleeDistance;

        brain.template setMemory<WalkTarget>(MemoryModuleTypes::WALK_TARGET,
            WalkTarget(
                BlockPos(static_cast<i32>(fleeX), static_cast<i32>(ownerPos.y), static_cast<i32>(fleeZ)), m_speed, 1));
    }

    f32 m_speed;
    f32 m_fleeDistance;
};

} // namespace movement
} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
