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

#include "../../../core/MobEntity.hpp"
#include "../../memory/Brain.hpp"
#include "../../memory/MemoryModuleStatus.hpp"
#include "../../memory/MemoryModuleType.hpp"
#include "../Task.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace action {

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
        f32 attackerWidth = owner->width();
        f32 targetWidth = target->width();
        f32 reachWidth = attackerWidth * 2.0f;
        f32 attackReachSq = reachWidth * reachWidth + targetWidth;
        f64 distSq = static_cast<f64>(owner->distanceSqTo(*target));
        return distSq <= static_cast<f64>(attackReachSq);
    }

    i32 m_cooldownTicks;
};

/**
 * @brief 繁殖任务
 *
 * 控制动物繁殖行为。
 *
 * TODO: 功能尚未实现
 */
template <typename E>
class BreedTask : public Task<E> {
public:
    BreedTask(f32 speed = 1.0f)
        : Task<E>({}, 60, 120)
        , m_speed(speed)
    {}

    std::string getName() const override { return "BreedTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // 检查是否可以繁殖
        // AgeableEntity* ageable = dynamic_cast<AgeableEntity*>(owner);
        // if (!ageable || !ageable->canBreed()) return false;
        //
        // auto brain = owner->getBrain();
        // auto breedTarget = brain->getMemory(memory::MemoryModuleTypes::BREED_TARGET);
        // return breedTarget.has_value();
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // auto brain = owner->getBrain();
        // auto breedTarget = brain->getMemory(memory::MemoryModuleTypes::BREED_TARGET);
        // if (breedTarget.has_value()) {
        //     owner->getNavigator()->moveTo(*breedTarget, m_speed);
        // }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // auto brain = owner->getBrain();
        // auto breedTarget = brain->getMemory(memory::MemoryModuleTypes::BREED_TARGET);
        // if (!breedTarget.has_value()) return;
        //
        // auto partner = *breedTarget;
        // owner->getLookController()->setLookAt(partner);
        //
        // // 检查距离
        // if (owner->distanceTo(*partner) < 2.0f) {
        //     // 执行繁殖
        //     owner->breedWith(partner);
        //     owner->getBrain()->removeMemory(memory::MemoryModuleTypes::BREED_TARGET);
        // }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // owner->getNavigator()->clearPath();
        // owner->getBrain()->removeMemory(memory::MemoryModuleTypes::BREED_TARGET);
    }

private:
    f32 m_speed;
};

/**
 * @brief 进食任务
 *
 * 控制动物进食行为。
 *
 * TODO: 功能尚未实现
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
        if (!owner || !owner->isAlive()) return false;
        // 检查是否饥饿
        // return owner->isHungry();
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        m_eatTimer = 0;
        // 寻找食物
        // auto food = findNearbyFood(world, owner);
        // if (food) {
        //     owner->getNavigator()->moveTo(food, 1.0f);
        // }
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        m_eatTimer++;
        // if (m_eatTimer >= m_eatDuration) {
        //     owner->eat();
        // }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override { m_eatTimer = 0; }

private:
    i32 m_eatDuration;
    i32 m_eatTimer;
};

/**
 * @brief 装死任务
 *
 * 控制实体装死行为（如负鼠）。
 *
 * TODO: 功能尚未实现
 */
template <typename E>
class PlayDeadTask : public Task<E> {
public:
    PlayDeadTask(i32 minDuration = 100, i32 maxDuration = 200)
        : Task<E>({}, minDuration, maxDuration)
        , m_isPlayingDead(false)
    {}

    std::string getName() const override { return "PlayDeadTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto playDead = brain->getMemory(memory::MemoryModuleTypes::PLAY_DEAD);
        // return playDead.has_value() && *playDead;
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        m_isPlayingDead = true;
        // owner->setPlayingDead(true);
        // owner->getNavigator()->clearPath();
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        // 保持装死状态
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override
    {
        m_isPlayingDead = false;
        // owner->setPlayingDead(false);
    }

private:
    bool m_isPlayingDead;
};

/**
 * @brief 跳跃任务
 *
 * 控制实体跳跃行为。
 *
 * TODO: 功能尚未实现
 */
template <typename E>
class JumpTask : public Task<E> {
public:
    JumpTask(f32 jumpHeight = 0.5f, i32 cooldown = 20)
        : Task<E>({}, 10, 30)
        , m_jumpHeight(jumpHeight)
        , m_cooldown(cooldown)
        , m_cooldownTimer(0)
    {}

    std::string getName() const override { return "JumpTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;
        // return owner->isOnGround() && m_cooldownTimer <= 0;
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // owner->jump(m_jumpHeight);
        m_cooldownTimer = m_cooldown;
    }

    void updateTask(IWorld* world, E* owner, i64 gameTime) override
    {
        if (m_cooldownTimer > 0) {
            m_cooldownTimer--;
        }
    }

    void resetTask(IWorld* world, E* owner, i64 gameTime) override { m_cooldownTimer = 0; }

private:
    f32 m_jumpHeight;
    i32 m_cooldown;
    i32 m_cooldownTimer;
};

/**
 * @brief 踢攻击任务
 *
 * 控制马踢攻击。
 *
 * TODO: 功能尚未实现
 */
template <typename E>
class KickTask : public Task<E> {
public:
    KickTask(f32 range = 2.0f, i32 damage = 2)
        : Task<E>({}, 20, 40)
        , m_range(range)
        , m_damage(damage)
    {}

    std::string getName() const override { return "KickTask"; }

protected:
    bool shouldExecute(IWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // return target.has_value() && (*target)->isAlive() &&
        //        owner->distanceTo(**target) <= m_range;
        return false;
    }

    void startExecuting(IWorld* world, E* owner, i64 gameTime) override
    {
        // 执行踢攻击
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // if (target.has_value()) {
        //     (*target)->hurt(owner, m_damage);
        // }
    }

private:
    f32 m_range;
    i32 m_damage;
};

} // namespace action
} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
