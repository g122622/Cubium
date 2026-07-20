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
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/entity/ai/brain/task/Task.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace action {

using mc::entity::ai::brain::memory::MemoryModuleStatus;
using mc::entity::ai::brain::memory::MemoryModuleTypes;
using mc::entity::ai::goal::constants::BREED_DETECTION_RANGE;
using mc::entity::ai::goal::constants::BREED_DISTANCE_SQ;
using mc::entity::ai::goal::constants::SPAWN_BABY_DELAY;

/**
 * @brief 近战攻击任务（对应 MC MeleeAttack）
 *
 * 当实体拥有攻击目标、不在冷却中、且处于近战攻击范围内时，
 * 执行近战攻击：面向目标、挥动手臂、调用 attackEntityAsMob、设置冷却。
 *
 * 记忆模块要求：
 * - ATTACK_TARGET (VALUE_PRESENT): 必须有攻击目标
 * - ATTACK_COOLING_DOWN (VALUE_ABSENT): 不能处于冷却中
 * - LOOK_TARGET (REGISTERED): 用于设置面向目标
 *
 * 攻击成功后设置 ATTACK_COOLING_DOWN 记忆，TTL 为 cooldownTicks，
 * 由 Brain 的记忆过期机制自动清除，从而允许下次攻击。
 *
 * 本任务为单次触发型：每次满足条件时触发一次攻击，而非持续执行。
 * 冷却由 ATTACK_COOLING_DOWN 记忆的 TTL 管理，无需手动计数器。
 *
 * @tparam E 实体类型，必须继承自 MobEntity
 */
template <typename E>
class AttackTask : public Task<E> {
public:
    /**
     * @brief 构造近战攻击任务
     * @param cooldownTicks 攻击冷却时间（tick），对应 MC MeleeAttack.create(int)
     */
    explicit AttackTask(i32 cooldownTicks = 20)
        : Task<E>({{MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_ABSENT},
                      {MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::REGISTERED}},
              cooldownTicks,
              cooldownTicks)
        , m_cooldownTicks(cooldownTicks)
    {}

    std::string getName() const override { return "AttackTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto attackTarget = brain.template getMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTarget.has_value() || *attackTarget == nullptr || !(*attackTarget)->isAlive()) {
            return false;
        }

        LivingEntity* target = *attackTarget;

        // 检查是否手持远程武器（持有可用非近战武器时不进行近战攻击）
        const ItemStack& mainHand = owner->getMainHandItem();
        if (!mainHand.isEmpty() && owner->canUseNonMeleeWeapon(mainHand)) {
            return false;
        }

        // 检查是否在近战攻击范围内
        return _isWithinMeleeAttackRange(owner, target);
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // 单次触发型任务，不需要持续执行
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto attackTarget = brain.template getMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTarget.has_value() || *attackTarget == nullptr) {
            return;
        }

        LivingEntity* target = *attackTarget;

        // 面向攻击目标
        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *target, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        // 挥动手臂（播放攻击动画）
        owner->swingArm();

        // 执行近战攻击
        owner->attackEntityAsMob(*target);

        // 设置攻击冷却记忆，TTL 后自动过期
        brain.template setMemoryWithTTL<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN, true, m_cooldownTicks);
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 单次触发型任务，所有逻辑在 startExecuting 中完成
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 无需清理：冷却记忆由 TTL 自动管理
    }

private:
    /**
     * @brief 检查目标是否在近战攻击范围内
     *
     * 计算公式：(攻击者宽度 * 2)^2 + 目标宽度
     * 与 MeleeAttackGoal::getAttackReachSqr() 一致
     */
    static bool _isWithinMeleeAttackRange(const E* owner, const LivingEntity* target)
    {
        // 使用 Entity 基类的 width()（public virtual），避免 AgeableEntity 的 protected 覆盖问题
        const Entity& ownerEntity = *owner;
        const Entity& targetEntity = *target;
        f32 attackerWidth = ownerEntity.width();
        f32 targetWidth = targetEntity.width();
        f32 reachWidth = attackerWidth * 2.0f;
        f32 attackReachSq = reachWidth * reachWidth + targetWidth;
        f64 distSq = static_cast<f64>(owner->distanceSqTo(*target));
        return distSq <= static_cast<f64>(attackReachSq);
    }

    i32 m_cooldownTicks;
};

/**
 * @brief 繁殖任务（对应 MC AnimalMakeLove）
 *
 * 控制动物繁殖行为。当动物处于发情状态（isInLove）时，
 * 从可见实体中寻找同类发情伴侣，走向对方并在足够近时繁殖后代。
 *
 * TODO: 当 AnimalEntity 集成 Brain 系统后，需在对应实体的 initializeBrain() 中
 * 注册此任务和配套传感器（如扩展 BabySensor 以写入 BREED_TARGET）。
 * 当前仅 VillagerEntity 拥有 brain()，动物实体仍使用 Goal 系统的 BreedGoal。
 *
 * 记忆模块要求：
 * - VISIBLE_MOBS (VALUE_PRESENT): 必须有可见实体列表以寻找伴侣
 * - BREED_TARGET (VALUE_ABSENT): 尚未有繁殖目标
 * - WALK_TARGET (REGISTERED): 用于设置行走目标
 * - LOOK_TARGET (REGISTERED): 用于设置注视目标
 *
 * 流程：
 * 1. shouldExecute：检查 isInLove，从 VISIBLE_MOBS 中寻找可交配的伴侣
 * 2. startExecuting：设置 BREED_TARGET 记忆，注视伴侣，计算繁殖时间
 * 3. updateTask：持续走向伴侣，到达繁殖距离且时间到后生成后代
 * 4. resetTask：清除导航和繁殖目标记忆
 *
 * @tparam E 实体类型，必须继承自 AnimalEntity 且拥有 brain() 方法
 */
template <typename E>
class BreedTask : public Task<E> {
public:
    /**
     * @brief 构造繁殖任务
     * @param speed 走向伴侣的移动速度修正值
     * @param closeEnoughDistance 足够近的距离（方块），到达此距离视为可繁殖
     */
    explicit BreedTask(f32 speed = 1.0f, i32 closeEnoughDistance = 2)
        : Task<E>({{MemoryModuleTypes::VISIBLE_MOBS, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::BREED_TARGET, MemoryModuleStatus::VALUE_ABSENT},
                      {MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED},
                      {MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::REGISTERED}},
              SPAWN_BABY_DELAY,
              SPAWN_BABY_DELAY + 50)
        , m_speed(speed)
        , m_closeEnoughDistance(closeEnoughDistance)
        , m_spawnBabyTime(0)
    {}

    std::string getName() const override { return "BreedTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // 必须是处于发情状态的成年动物
        auto* animal = dynamic_cast<AgeableEntity*>(owner);
        if (!animal || animal->isChild() || !animal->isInLove()) {
            return false;
        }

        // 从可见实体中寻找可交配的伴侣
        auto* partner = _findValidBreedPartner(owner);
        if (!partner) {
            return false;
        }

        // 临时缓存伴侣，供 startExecuting 使用
        m_cachedPartner = partner;
        return true;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto breedTarget = brain.template getMemory<AgeableEntity*>(MemoryModuleTypes::BREED_TARGET);

        // 繁殖目标必须存活
        if (!breedTarget.has_value() || *breedTarget == nullptr || !(*breedTarget)->isAlive()) {
            return false;
        }

        AgeableEntity* partner = *breedTarget;

        // 确认双方仍可交配
        auto* animalOwner = dynamic_cast<AgeableEntity*>(owner);
        if (!animalOwner || !animalOwner->isInLove()) {
            return false;
        }

        auto* partnerAnimal = dynamic_cast<AnimalEntity*>(partner);
        if (!partnerAnimal || !partnerAnimal->isInLove()) {
            return false;
        }

        // 超时检查：超过繁殖时间上限则终止
        if (gameTime > m_spawnBabyTime + SPAWN_BABY_DELAY + 50) {
            return false;
        }

        return true;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto* animal = dynamic_cast<AgeableEntity*>(owner);
        if (!animal) {
            return;
        }

        AgeableEntity* partner = m_cachedPartner;
        m_cachedPartner = nullptr;

        if (!partner || !partner->isAlive()) {
            return;
        }

        auto& brain = owner->brain();

        // 在自身大脑中设置繁殖目标
        brain.template setMemory<AgeableEntity*>(MemoryModuleTypes::BREED_TARGET, partner);

        // 面向伴侣并走向对方
        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *partner, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        brain.template setMemory<memory::WalkTarget>(MemoryModuleTypes::WALK_TARGET,
            memory::WalkTarget(BlockPos(partner->position()), m_speed, m_closeEnoughDistance));

        // 计算繁殖后代的时间点（对应 MC: gameTime + 60 + random(50)）
        math::Random& rng = owner->getRandom();
        m_spawnBabyTime = gameTime + SPAWN_BABY_DELAY + rng.nextInt(50);
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto breedTarget = brain.template getMemory<AgeableEntity*>(MemoryModuleTypes::BREED_TARGET);
        if (!breedTarget.has_value() || *breedTarget == nullptr) {
            return;
        }

        AgeableEntity* partner = *breedTarget;

        // 面向伴侣
        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *partner, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        // 持续走向伴侣
        brain.template setMemory<memory::WalkTarget>(MemoryModuleTypes::WALK_TARGET,
            memory::WalkTarget(BlockPos(partner->position()), m_speed, m_closeEnoughDistance));

        // 检查是否到达繁殖距离且时间已到
        f64 distSq = owner->distanceSqTo(*partner);
        if (gameTime >= m_spawnBabyTime && distSq <= static_cast<f64>(BREED_DISTANCE_SQ)) {
            _spawnBabyFromBreeding(world, owner, partner);
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();

        // 清除繁殖目标
        brain.template removeMemory<AgeableEntity*>(MemoryModuleTypes::BREED_TARGET);

        // 清除导航相关记忆
        brain.template removeMemory<memory::WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        brain.template removeMemory<std::shared_ptr<memory::IPositionTarget>>(MemoryModuleTypes::LOOK_TARGET);

        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }

        m_spawnBabyTime = 0;
        m_cachedPartner = nullptr;
    }

private:
    /**
     * @brief 从可见实体中寻找有效的繁殖伴侣
     *
     * 在 VISIBLE_MOBS 记忆中搜索同类、发情中的动物。
     * 对应 MC AnimalMakeLove.findValidBreedPartner
     */
    AgeableEntity* _findValidBreedPartner(E* owner)
    {
        auto* animal = dynamic_cast<AnimalEntity*>(owner);
        if (!animal) {
            return nullptr;
        }

        auto& brain = owner->brain();
        auto visibleMobs = brain.template getMemory<std::vector<LivingEntity*>>(MemoryModuleTypes::VISIBLE_MOBS);
        if (!visibleMobs.has_value() || visibleMobs->empty()) {
            return nullptr;
        }

        // 在可见实体中搜索最近的同类发情伴侣
        AgeableEntity* closestPartner = nullptr;
        f32 closestDistSq = BREED_DETECTION_RANGE * BREED_DETECTION_RANGE;

        for (auto* mob : *visibleMobs) {
            if (!mob || !mob->isAlive()) {
                continue;
            }

            auto* candidate = dynamic_cast<AgeableEntity*>(mob);
            if (!candidate || candidate->isChild()) {
                continue;
            }

            auto* candidateAnimal = dynamic_cast<AnimalEntity*>(candidate);
            if (!candidateAnimal) {
                continue;
            }

            // 检查是否可以交配（同类、双方都发情）
            if (!animal->canMateWith(*candidateAnimal)) {
                continue;
            }

            f32 distSq = owner->distanceSqTo(*candidate);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                closestPartner = candidate;
            }
        }

        return closestPartner;
    }

    /**
     * @brief 执行繁殖，生成后代
     *
     * 对应 MC AnimalMakeLove.spawnChildFromBreeding
     * 逻辑与 BreedGoal::spawnBaby() 一致
     */
    void _spawnBabyFromBreeding(IWorld* world, E* owner, AgeableEntity* partner)
    {
        if (!world) {
            return;
        }

        auto* animal = dynamic_cast<AnimalEntity*>(owner);
        auto* partnerAnimal = dynamic_cast<AnimalEntity*>(partner);
        if (!animal || !partnerAnimal) {
            return;
        }

        // 重置双方发情状态
        animal->resetInLove();
        partnerAnimal->resetInLove();

        // 设置繁殖冷却
        animal->setGrowingAge(AgeableEntity::BREEDING_COOLDOWN);
        partnerAnimal->setGrowingAge(AgeableEntity::BREEDING_COOLDOWN);

        // 生成后代
        auto baby = animal->spawnBaby(*partnerAnimal);
        if (!baby) {
            return;
        }

        // 设置类型ID
        baby->setTypeId(animal->getTypeId());

        // 设置位置（在父方附近随机偏移）
        math::Random& rng = owner->getRandom();
        f64 babyX = static_cast<f64>(owner->x()) + (rng.nextDouble() - 0.5) * 2.0;
        f64 babyY = static_cast<f64>(owner->y());
        f64 babyZ = static_cast<f64>(owner->z()) + (rng.nextDouble() - 0.5) * 2.0;
        baby->setPosition(static_cast<f32>(babyX), static_cast<f32>(babyY), static_cast<f32>(babyZ));

        // 设置幼年年龄
        baby->setGrowingAge(AgeableEntity::BABY_AGE);

        // 初始化生成属性
        auto* babyMob = dynamic_cast<MobEntity*>(baby.get());
        if (babyMob) {
            auto difficultyInstance = combat::DifficultyInstance::at(*world,
                BlockPos(
                    static_cast<i32>(std::floor(babyX)), static_cast<i32>(babyY), static_cast<i32>(std::floor(babyZ))));
            babyMob->finalizeSpawn(*world, difficultyInstance, world::spawn::SpawnReason::Breeding);
        }

        // 解析触发繁殖的玩家
        u64 loveCause = animal->getLoveCause();
        if (loveCause == 0) {
            loveCause = partnerAnimal->getLoveCause();
        }

        // 生成实体到世界
        Entity* babyPtr = baby.get();
        world->spawnEntity(std::move(baby));

        // 触发繁殖事件（用于成就系统等）
        if (loveCause != 0) {
            world->onBredAnimals(loveCause, babyPtr, animal, partnerAnimal);
        }

        // 双方播放爱心粒子
        animal->spawnHeartParticles();
        partnerAnimal->spawnHeartParticles();

        // 生成经验球
        i32 xpCount = 1 + rng.nextInt(7);
        for (i32 i = 0; i < xpCount; ++i) {
            auto xpOrb = std::make_unique<ExperienceOrbEntity>(
                world, static_cast<f64>(owner->x()), static_cast<f64>(owner->y()), static_cast<f64>(owner->z()), 1);

            // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
            xpOrb->setTypeId(EntityTypes::EXPERIENCE_ORB);

            f32 vx = (rng.nextFloat() - 0.5f) * 0.2f;
            f32 vy = rng.nextFloat() * 0.2f;
            f32 vz = (rng.nextFloat() - 0.5f) * 0.2f;
            xpOrb->setVelocity(vx, vy, vz);
            world->spawnEntity(std::move(xpOrb));
        }
    }

    f32 m_speed;
    i32 m_closeEnoughDistance;
    i64 m_spawnBabyTime = 0;
    AgeableEntity* m_cachedPartner = nullptr;
};

/**
 * @brief 进食任务
 *
 * 控制动物进食行为。当动物饥饿时，寻找附近的食物并前往进食。
 * 当前为占位实现，待实体饥饿系统完善后集成。
 *
 * TODO: 实体的饥饿状态（isHungry）系统尚未实现，需要在 AnimalEntity 或
 * AgeableEntity 中添加饥饿机制后才能完成此任务。
 */
template <typename E>
class EatTask : public Task<E> {
public:
    EatTask(i32 eatDuration = 40)
        : Task<E>({}, eatDuration, eatDuration)
        , m_eatDuration(eatDuration)
        , m_eatTimer(0)
    {}

    std::string getName() const override { return "EatTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // TODO: 实体的饥饿状态系统尚未实现，当前无法判断是否需要进食
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override { m_eatTimer = 0; }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        m_eatTimer++;
        // TODO: 当饥饿系统实现后，在此处执行进食逻辑
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override { m_eatTimer = 0; }

private:
    i32 m_eatDuration;
    i32 m_eatTimer;
};

/**
 * @brief 装死任务（对应 MC BecomePassiveIfMemoryPresent + PlayDead）
 *
 * 当实体拥有 PLAY_DEAD_TICKS 记忆时，进入被动状态：
 * 设置 PACIFIED 记忆（阻止攻击行为）、清除 ATTACK_TARGET、清除导航路径。
 * PLAY_DEAD_TICKS 由外部系统（如受到伤害时）设置，本任务响应并执行装死逻辑。
 *
 * TODO: 当美西螈（AxolotlEntity）等实体集成 Brain 系统后，需在对应实体的
 * initializeBrain() 中注册此任务。当前这些实体仍使用 Goal 系统。
 *
 * 记忆模块要求：
 * - PLAY_DEAD_TICKS (VALUE_PRESENT): 必须有装死计时器
 * - ATTACK_TARGET (REGISTERED): 用于清除攻击目标
 * - PACIFIED (REGISTERED): 用于设置被动状态标记
 *
 * @tparam E 实体类型
 */
template <typename E>
class PlayDeadTask : public Task<E> {
public:
    PlayDeadTask(i32 pacifiedDuration = 200)
        : Task<E>({{MemoryModuleTypes::PLAY_DEAD_TICKS, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::REGISTERED},
                      {MemoryModuleTypes::PACIFIED, MemoryModuleStatus::REGISTERED}},
              pacifiedDuration,
              pacifiedDuration)
        , m_pacifiedDuration(pacifiedDuration)
    {}

    std::string getName() const override { return "PlayDeadTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // PLAY_DEAD_TICKS 记忆必须存在（已通过 requiredMemoryState 保证）
        // 检查 PACIFIED 记忆是否不存在（避免重复触发）
        auto& brain = owner->brain();
        auto pacified = brain.template getMemory<bool>(MemoryModuleTypes::PACIFIED);
        return !pacified.has_value();
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // PLAY_DEAD_TICKS 仍然存在时继续装死
        auto& brain = owner->brain();
        auto playDeadTicks = brain.template getMemory<i32>(MemoryModuleTypes::PLAY_DEAD_TICKS);
        return playDeadTicks.has_value();
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();

        // 设置被动状态记忆，阻止攻击行为
        brain.template setMemoryWithTTL<bool>(MemoryModuleTypes::PACIFIED, true, m_pacifiedDuration);

        // 清除攻击目标
        brain.template removeMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET);

        // 清除导航路径
        auto* navigator = owner->navigator();
        if (navigator) {
            navigator->clearPath();
        }

        // 清除行走目标和注视目标
        brain.template removeMemory<memory::WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        brain.template removeMemory<std::shared_ptr<memory::IPositionTarget>>(MemoryModuleTypes::LOOK_TARGET);
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 装死期间持续清除导航和行走目标，防止其他任务移动实体
        auto& brain = owner->brain();
        brain.template removeMemory<memory::WalkTarget>(MemoryModuleTypes::WALK_TARGET);
        brain.template removeMemory<std::shared_ptr<memory::IPositionTarget>>(MemoryModuleTypes::LOOK_TARGET);
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 装死结束时不需要额外清理，PACIFIED 记忆由 TTL 自动过期
    }

private:
    i32 m_pacifiedDuration;
};

/**
 * @brief 跳跃任务
 *
 * 控制实体跳跃行为。当实体在地面且冷却时间已过时执行跳跃。
 * 通过 JUMP_COOLDOWN 记忆的 TTL 机制管理冷却时间。
 *
 * 记忆模块要求：
 * - JUMP_COOLDOWN (VALUE_ABSENT): 冷却期间不能跳跃
 *
 * @tparam E 实体类型
 */
template <typename E>
class JumpTask : public Task<E> {
public:
    JumpTask(i32 cooldownTicks = 20)
        : Task<E>({{MemoryModuleTypes::JUMP_COOLDOWN, MemoryModuleStatus::VALUE_ABSENT}}, 10, 30)
        , m_cooldownTicks(cooldownTicks)
    {}

    std::string getName() const override { return "JumpTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        // 必须在地面上才能跳跃
        if (!owner->isOnGround()) {
            return false;
        }

        return true;
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // 单次触发型：跳跃在 startExecuting 中完成
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // 执行跳跃
        auto* jumpCtrl = owner->jumpController();
        if (jumpCtrl) {
            jumpCtrl->setJumping();
        }

        // 设置冷却记忆
        owner->brain().template setMemoryWithTTL<i32>(
            MemoryModuleTypes::JUMP_COOLDOWN, m_cooldownTicks, m_cooldownTicks);
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 单次触发型，不需要持续更新
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 冷却由 JUMP_COOLDOWN 记忆的 TTL 自动管理
    }

private:
    i32 m_cooldownTicks;
};

/**
 * @brief 踢攻击任务
 *
 * 控制马等实体的踢击攻击行为。当攻击目标在近距离范围内时，
 * 对目标造成伤害。通过 ATTACK_TARGET 记忆获取攻击目标，
 * 通过 ATTACK_COOLING_DOWN 记忆管理冷却。
 *
 * 记忆模块要求：
 * - ATTACK_TARGET (VALUE_PRESENT): 必须有攻击目标
 * - ATTACK_COOLING_DOWN (VALUE_ABSENT): 冷却期间不能攻击
 *
 * @tparam E 实体类型
 */
template <typename E>
class KickTask : public Task<E> {
public:
    KickTask(f32 range = 2.0f, i32 cooldownTicks = 20)
        : Task<E>({{MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT},
                      {MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_ABSENT}},
              cooldownTicks,
              cooldownTicks)
        , m_range(range)
        , m_cooldownTicks(cooldownTicks)
    {}

    std::string getName() const override { return "KickTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) {
            return false;
        }

        auto& brain = owner->brain();
        auto attackTarget = brain.template getMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTarget.has_value() || *attackTarget == nullptr || !(*attackTarget)->isAlive()) {
            return false;
        }

        // 检查目标是否在踢击范围内
        f64 distSq = owner->distanceSqTo(**attackTarget);
        return distSq <= static_cast<f64>(m_range * m_range);
    }

    bool shouldContinueExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // 单次触发型
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        auto& brain = owner->brain();
        auto attackTarget = brain.template getMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET);
        if (!attackTarget.has_value() || *attackTarget == nullptr) {
            return;
        }

        LivingEntity* target = *attackTarget;

        // 面向攻击目标
        if (auto* lookCtrl = owner->lookController()) {
            lookCtrl->setLookPositionWithEntity(
                *target, owner->getHorizontalFaceSpeed(), owner->getVerticalFaceSpeed());
        }

        // 执行踢击攻击
        owner->attackEntityAsMob(*target);

        // 设置冷却记忆
        brain.template setMemoryWithTTL<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN, true, m_cooldownTicks);
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 单次触发型
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 冷却由 ATTACK_COOLING_DOWN 记忆的 TTL 自动管理
    }

private:
    f32 m_range;
    i32 m_cooldownTicks;
};

} // namespace action
} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
