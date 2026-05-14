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

#include "../../../../util/math/random/Random.hpp"
#include "../Brain.hpp"
#include "../memory/MemoryModuleStatus.hpp"
#include "../memory/MemoryModuleType.hpp"
#include "../schedule/Activity.hpp"
#include <string>
#include <unordered_map>

namespace mc {

// Forward declarations
class LivingEntity;
class IWorld;

namespace entity {
namespace ai {
namespace brain {
namespace task {

/**
 * @brief Brain任务状态
 */
enum class TaskStatus { STOPPED, RUNNING };

/**
 * @brief Brain任务基类
 *
 * Brain系统中的任务与Goal系统类似，但使用记忆模块
 * 参考 MC 1.16.5 Task
 *
 * @tparam E 实体类型
 */
template <typename E>
class Task {
public:
    using MemoryStateMap = std::unordered_map<const memory::MemoryModuleTypeBase*, memory::MemoryModuleStatus>;

    /**
     * @brief 构造任务
     * @param requiredMemoryState 需要的记忆状态
     * @param durationMin 最小持续时间
     * @param durationMax 最大持续时间
     */
    explicit Task(MemoryStateMap requiredMemoryState = {}, i32 durationMin = 60, i32 durationMax = 60)
        : m_requiredMemoryState(std::move(requiredMemoryState))
        , m_status(TaskStatus::STOPPED)
        , m_stopTime(0)
        , m_durationMin(durationMin)
        , m_durationMax(durationMax)
    {}

    virtual ~Task() = default;

    /**
     * @brief 获取任务状态
     */
    [[nodiscard]] TaskStatus getStatus() const { return m_status; }

    /**
     * @brief 尝试启动任务
     * @param world 世界
     * @param owner 实体
     * @param gameTime 游戏时间
     * @param random 随机数生成器（MC 1.16.5使用world.getRandom()）
     * @return 是否成功启动
     */
    bool start(IWorld* world, E* owner, i64 gameTime, math::Random& random)
    {
        if (hasRequiredMemories(owner) && shouldExecute(world, owner)) {
            m_status = TaskStatus::RUNNING;

            // MC 1.16.5: 使用随机数生成持续时间
            // int i = this.durationMin + worldIn.getRandom().nextInt(this.durationMax + 1 - this.durationMin);
            i32 duration = m_durationMin;
            if (m_durationMax > m_durationMin) {
                i32 range = m_durationMax - m_durationMin + 1;
                duration = m_durationMin + random.nextInt(range);
            }
            m_stopTime = gameTime + duration;

            startExecuting(world, owner, gameTime);
            return true;
        }
        return false;
    }

    /**
     * @brief 每tick更新
     * @param world 世界
     * @param owner 实体
     * @param gameTime 游戏时间
     */
    void tick(IWorld* world, E* owner, i64 gameTime)
    {
        if (!isTimedOut(gameTime) && shouldContinueExecuting(world, owner, gameTime)) {
            updateTask(world, owner, gameTime);
        } else {
            stop(world, owner, gameTime);
        }
    }

    /**
     * @brief 停止任务
     * @param world 世界
     * @param owner 实体
     * @param gameTime 游戏时间
     */
    void stop(IWorld* world, E* owner, i64 gameTime)
    {
        m_status = TaskStatus::STOPPED;
        resetTask(world, owner, gameTime);
    }

    /**
     * @brief 获取任务名称
     */
    [[nodiscard]] virtual std::string getName() const { return "Task"; }

protected:
    /**
     * @brief 检查是否应该执行
     */
    virtual bool shouldExecute(IWorld* /*world*/, E* /*owner*/) { return true; }

    /**
     * @brief 检查是否应该继续执行
     */
    virtual bool shouldContinueExecuting(IWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) { return false; }

    /**
     * @brief 开始执行时的回调
     */
    virtual void startExecuting(IWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {}

    /**
     * @brief 更新任务
     */
    virtual void updateTask(IWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {}

    /**
     * @brief 重置任务
     */
    virtual void resetTask(IWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {}

    /**
     * @brief 检查是否超时
     */
    [[nodiscard]] bool isTimedOut(i64 gameTime) const { return gameTime > m_stopTime; }

    MemoryStateMap m_requiredMemoryState;

private:
    /**
     * @brief 检查实体是否有所需的记忆状态
     * MC 1.16.5: Task.hasRequiredMemories()
     */
    bool hasRequiredMemories(E* owner)
    {
        if (!owner) {
            return false;
        }

        // MC 1.16.5: 遍历所有需要的记忆状态
        // 注意：实体类需要提供 brain() 方法返回 Brain<E>&
        for (const auto& [memType, status] : m_requiredMemoryState) {
            if (!owner->brain().hasMemory(memType, status)) {
                return false;
            }
        }
        return true;
    }

    TaskStatus m_status;
    i64 m_stopTime;
    i32 m_durationMin;
    i32 m_durationMax;
};

} // namespace task
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
