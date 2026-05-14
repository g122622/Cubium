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
#include "../../../world/ServerWorld.hpp"
#include "../../memory/Brain.hpp"
#include "../Task.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace action {

/**
 * @brief 攻击任务
 *
 * 控制实体攻击目标。
 *
 * 参考 MC 1.16.5 AttackTask
 */
template <typename E>
class AttackTask : public Task<E> {
public:
    AttackTask(i32 attackInterval = 20, f32 attackRange = 2.0f)
        : Task<E>({}, attackInterval, attackInterval * 2)
        , m_attackInterval(attackInterval)
        , m_attackRange(attackRange)
        , m_attackCooldown(0)
    {}

    std::string getName() const override { return "AttackTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // if (!target.has_value() || !(*target)->isAlive()) return false;
        //
        // return owner->distanceTo(**target) <= m_attackRange;
        return false;
    }

    bool shouldContinueExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // return target.has_value() && (*target)->isAlive();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override { m_attackCooldown = 0; }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // if (!target.has_value() || !(*target)->isAlive()) return;
        //
        // auto targetEntity = *target;
        //
        // // 看向目标
        // owner->getLookController()->setLookAt(targetEntity);
        //
        // // 攻击冷却
        // if (m_attackCooldown <= 0) {
        //     // 检查距离
        //     if (owner->distanceTo(*targetEntity) <= m_attackRange) {
        //         owner->attack(targetEntity);
        //         m_attackCooldown = m_attackInterval;
        //     }
        // } else {
        //     m_attackCooldown--;
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override { m_attackCooldown = 0; }

private:
    i32 m_attackInterval;
    f32 m_attackRange;
    i32 m_attackCooldown;
};

/**
 * @brief 繁殖任务
 *
 * 控制动物繁殖行为。
 *
 * 参考 MC 1.16.5 BreedTask
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
    bool shouldExecute(ServerWorld* world, E* owner) override
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

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // auto brain = owner->getBrain();
        // auto breedTarget = brain->getMemory(memory::MemoryModuleTypes::BREED_TARGET);
        // if (breedTarget.has_value()) {
        //     owner->getNavigator()->moveTo(*breedTarget, m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
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

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
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
 * 参考 MC 1.16.5 EatTask
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
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;
        // 检查是否饥饿
        // return owner->isHungry();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        m_eatTimer = 0;
        // 寻找食物
        // auto food = findNearbyFood(world, owner);
        // if (food) {
        //     owner->getNavigator()->moveTo(food, 1.0f);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        m_eatTimer++;
        // if (m_eatTimer >= m_eatDuration) {
        //     owner->eat();
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override { m_eatTimer = 0; }

private:
    i32 m_eatDuration;
    i32 m_eatTimer;
};

/**
 * @brief 装死任务
 *
 * 控制实体装死行为（如负鼠）。
 *
 * 参考 MC 1.16.5 PlayDeadTask
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
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto playDead = brain->getMemory(memory::MemoryModuleTypes::PLAY_DEAD);
        // return playDead.has_value() && *playDead;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        m_isPlayingDead = true;
        // owner->setPlayingDead(true);
        // owner->getNavigator()->clearPath();
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // 保持装死状态
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
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
 * 参考 MC 1.16.5 JumpTask
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
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;
        // return owner->isOnGround() && m_cooldownTimer <= 0;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // owner->jump(m_jumpHeight);
        m_cooldownTimer = m_cooldown;
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        if (m_cooldownTimer > 0) {
            m_cooldownTimer--;
        }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override { m_cooldownTimer = 0; }

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
 * 参考 MC 1.16.5 KickTask
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
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // return target.has_value() && (*target)->isAlive() &&
        //        owner->distanceTo(**target) <= m_range;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
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
