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
#include "common/entity/ai/brain/task/Task.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace task {
namespace movement {

/**
 * @brief 移动到位置任务
 *
 * 控制实体移动到指定位置。
 */
template <typename E>
class MoveToTargetTask : public Task<E> {
public:
    explicit MoveToTargetTask(f32 speed = 1.0f, i32 completionRange = 1)
        : Task<E>({}, 60, 200)
        , m_speed(speed)
        , m_completionRange(completionRange)
    {}

    std::string getName() const override { return "MoveToTargetTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // TODO: 检查是否有移动目标
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::WALK_TARGET);
        // return target.has_value();
        return false;
    }

    bool shouldContinueExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) return false;

        // TODO: 检查是否到达目标
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::WALK_TARGET);
        // if (!target.has_value()) return false;

        // return !hasReachedTarget(owner, target.value());
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 获取移动目标并开始导航
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::WALK_TARGET);
        // if (target.has_value()) {
        //     owner->getNavigator()->moveTo(target.value(), m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 持续移动
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 停止移动
        // owner->getNavigator()->clearPath();
    }

private:
    f32 m_speed;
    i32 m_completionRange;
};

/**
 * @brief 随机游走任务
 *
 * 控制实体随机移动。
 */
template <typename E>
class StrollTask : public Task<E> {
public:
    explicit StrollTask(f32 speed = 1.0f, i32 interval = 40)
        : Task<E>({}, interval, interval * 2)
        , m_speed(speed)
    {}

    std::string getName() const override { return "StrollTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;
        // TODO: 随机概率触发
        // return owner->getRandom().nextFloat() < 0.02f;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 随机选择目标位置
        // f32 x = owner->x() + (owner->getRandom().nextFloat() - 0.5f) * 20.0f;
        // f32 z = owner->z() + (owner->getRandom().nextFloat() - 0.5f) * 20.0f;
        // owner->getNavigator()->moveTo(x, owner->y(), z, m_speed);
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 清除路径
        // owner->getNavigator()->clearPath();
    }

private:
    f32 m_speed;
};

/**
 * @brief 看向实体任务
 *
 * 控制实体看向目标实体。
 */
template <typename E>
class LookAtEntityTask : public Task<E> {
public:
    LookAtEntityTask(f32 range = 8.0f, f32 probability = 0.02f)
        : Task<E>({}, 40, 80)
        , m_range(range)
        , m_probability(probability)
    {}

    std::string getName() const override { return "LookAtEntityTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;
        // TODO: 概率触发
        // return owner->getRandom().nextFloat() < m_probability;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 寻找附近的实体并看向它
        // auto entities = world->getEntitiesInRange(owner->position(), m_range);
        // if (!entities.empty()) {
        //     auto target = entities[owner->getRandom().nextInt(entities.size())];
        //     owner->getLookController()->setLookAt(target);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 重置看向
    }

private:
    f32 m_range;
    f32 m_probability;
};

/**
 * @brief 寻找隐蔽点任务
 *
 * 控制实体寻找安全的位置。
 */
template <typename E>
class FindHiddenBlockTask : public Task<E> {
public:
    FindHiddenBlockTask(f32 speed = 1.0f, i32 searchRange = 20)
        : Task<E>({}, 60, 120)
        , m_speed(speed)
        , m_searchRange(searchRange)
    {}

    std::string getName() const override { return "FindHiddenBlockTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // TODO: 检查是否需要隐蔽（例如白天对于僵尸）
        // return owner->shouldHide();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 寻找安全的隐蔽点
        // BlockPos hiddenPos = findHiddenBlock(world, owner);
        // if (hiddenPos.isValid()) {
        //     owner->getNavigator()->moveTo(hiddenPos, m_speed);
        //     owner->getBrain()->setMemory(memory::MemoryModuleTypes::HIDING_PLACE, hiddenPos);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 清除隐蔽状态
        // owner->getNavigator()->clearPath();
        // owner->getBrain()->removeMemory(memory::MemoryModuleTypes::HIDING_PLACE);
    }

private:
    f32 m_speed;
    i32 m_searchRange;
};

/**
 * @brief 追逐目标任务
 *
 * 控制实体追逐攻击目标。
 */
template <typename E>
class ChaseTask : public Task<E> {
public:
    ChaseTask(f32 speed = 1.5f, f32 stopDistance = 2.0f)
        : Task<E>({}, 60, 600)
        , m_speed(speed)
        , m_stopDistance(stopDistance)
    {}

    std::string getName() const override { return "ChaseTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // TODO: 检查是否有攻击目标
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // return target.has_value() && (*target)->isAlive();
        return false;
    }

    bool shouldContinueExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        if (!owner || !owner->isAlive()) return false;

        // TODO: 检查是否需要继续追逐
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // if (!target.has_value() || !(*target)->isAlive()) return false;

        // return owner->distanceTo(**target) > m_stopDistance;
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 开始追逐
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // if (target.has_value()) {
        //     owner->getNavigator()->moveTo(*target, m_speed);
        // }
    }

    void updateTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 更新追逐路径
        // auto brain = owner->getBrain();
        // auto target = brain->getMemory(memory::MemoryModuleTypes::ATTACK_TARGET);
        // if (target.has_value()) {
        //     owner->getLookController()->setLookAt(*target);
        //     owner->getNavigator()->moveTo(*target, m_speed);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 清除路径
        // owner->getNavigator()->clearPath();
    }

private:
    f32 m_speed;
    f32 m_stopDistance;
};

/**
 * @brief 避险任务
 *
 * 控制实体逃离危险。
 */
template <typename E>
class FleeTask : public Task<E> {
public:
    FleeTask(f32 speed = 1.5f, f32 fleeDistance = 16.0f)
        : Task<E>({}, 60, 200)
        , m_speed(speed)
        , m_fleeDistance(fleeDistance)
    {}

    std::string getName() const override { return "FleeTask"; }

protected:
    bool shouldExecute(ServerWorld* world, E* owner) override
    {
        if (!owner || !owner->isAlive()) return false;

        // TODO: 检查是否有逃避目标
        // auto brain = owner->getBrain();
        // auto avoidTarget = brain->getMemory(memory::MemoryModuleTypes::AVOID_TARGET);
        // return avoidTarget.has_value();
        return false;
    }

    void startExecuting(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 计算逃跑方向并开始移动
        // auto brain = owner->getBrain();
        // auto avoidTarget = brain->getMemory(memory::MemoryModuleTypes::AVOID_TARGET);
        // if (avoidTarget.has_value()) {
        //     auto target = *avoidTarget;
        //     f64 dx = owner->x() - target->x();
        //     f64 dz = owner->z() - target->z();
        //     f64 dist = std::sqrt(dx * dx + dz * dz);
        //     if (dist > 0.0) {
        //         dx /= dist;
        //         dz /= dist;
        //     }
        //
        //     f64 fleeX = owner->x() + dx * m_fleeDistance;
        //     f64 fleeZ = owner->z() + dz * m_fleeDistance;
        //     owner->getNavigator()->moveTo(fleeX, owner->y(), fleeZ, m_speed);
        // }
    }

    void resetTask(ServerWorld* world, E* owner, i64 gameTime) override
    {
        // TODO: 清除逃避状态
        // owner->getNavigator()->clearPath();
        // owner->getBrain()->removeMemory(memory::MemoryModuleTypes::AVOID_TARGET);
    }

private:
    f32 m_speed;
    f32 m_fleeDistance;
};

} // namespace movement
} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
