#pragma once

#include "../memory/MemoryModuleStatus.hpp"
#include "../memory/MemoryModuleType.hpp"
#include "../schedule/Activity.hpp"
#include <unordered_map>
#include <string>

namespace mc {

// Forward declarations
class LivingEntity;
class ServerWorld;

namespace entity {
namespace ai {
namespace brain {
namespace task {

/**
 * @brief Brain任务状态
 */
enum class TaskStatus {
    STOPPED,
    RUNNING
};

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
        , m_durationMax(durationMax) {}

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
     * @return 是否成功启动
     */
    bool start(ServerWorld* world, E* owner, i64 gameTime) {
        if (hasRequiredMemories(owner) && shouldExecute(world, owner)) {
            m_status = TaskStatus::RUNNING;

            // 计算随机持续时间
            i32 duration = m_durationMin;
            if (m_durationMax > m_durationMin) {
                i32 range = m_durationMax - m_durationMin + 1;
                duration = m_durationMin + (static_cast<i32>(gameTime) % range);
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
    void tick(ServerWorld* world, E* owner, i64 gameTime) {
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
    void stop(ServerWorld* world, E* owner, i64 gameTime) {
        m_status = TaskStatus::STOPPED;
        resetTask(world, owner, gameTime);
    }

    /**
     * @brief 获取任务名称
     */
    [[nodiscard]] virtual std::string getName() const {
        return "Task";
    }

protected:
    /**
     * @brief 检查是否应该执行
     */
    virtual bool shouldExecute(ServerWorld* /*world*/, E* /*owner*/) {
        return true;
    }

    /**
     * @brief 检查是否应该继续执行
     */
    virtual bool shouldContinueExecuting(ServerWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {
        return false;
    }

    /**
     * @brief 开始执行时的回调
     */
    virtual void startExecuting(ServerWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {}

    /**
     * @brief 更新任务
     */
    virtual void updateTask(ServerWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {}

    /**
     * @brief 重置任务
     */
    virtual void resetTask(ServerWorld* /*world*/, E* /*owner*/, i64 /*gameTime*/) {}

    /**
     * @brief 检查是否超时
     */
    [[nodiscard]] bool isTimedOut(i64 gameTime) const {
        return gameTime > m_stopTime;
    }

    MemoryStateMap m_requiredMemoryState;

private:
    /**
     * @brief 检查实体是否有所需的记忆状态
     */
    bool hasRequiredMemories(E* owner);

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
