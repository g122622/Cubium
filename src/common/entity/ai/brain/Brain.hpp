#pragma once

#include "memory/Memory.hpp"
#include "memory/MemoryModuleType.hpp"
#include "memory/MemoryModuleStatus.hpp"
#include "schedule/Activity.hpp"
#include "schedule/Schedule.hpp"
#include "sensor/Sensor.hpp"
#include "sensor/SensorType.hpp"
#include "task/Task.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include <vector>
#include <functional>

namespace mc {

// Forward declarations
class LivingEntity;
class ServerWorld;

namespace entity {
namespace ai {
namespace brain {

/**
 * @brief Brain系统 - 高级AI控制系统
 *
 * Brain系统提供比Goal系统更高级的AI控制，支持：
 * - 记忆模块(短期/长期记忆)
 * - 传感器(自动感知环境)
 * - 日程(基于时间的活动切换)
 * - 活动(Activity)状态管理
 *
 * 参考 MC 1.16.5 Brain
 *
 * @tparam E 实体类型
 */
template <typename E>
class Brain {
public:
    using MemoryMap = std::unordered_map<const memory::MemoryModuleTypeBase*, std::optional<memory::Memory<void*>>>;
    using SensorMap = std::unordered_map<std::string, std::unique_ptr<sensor::Sensor<E>>>;
    using TaskSet = std::unordered_set<std::unique_ptr<task::Task<E>>>;
    using ActivityTaskMap = std::unordered_map<schedule::Activity, TaskSet>;
    using PriorityTaskMap = std::unordered_map<i32, ActivityTaskMap>;
    using MemoryRequirement = std::pair<const memory::MemoryModuleTypeBase*, memory::MemoryModuleStatus>;
    using ActivityRequirementMap = std::unordered_map<schedule::Activity, std::unordered_set<MemoryRequirement>>;
    using ForgettingMap = std::unordered_map<schedule::Activity, std::unordered_set<const memory::MemoryModuleTypeBase*>>;

    /**
     * @brief 构造Brain
     */
    Brain() = default;

    /**
     * @brief 注册内存模块类型
     */
    template <typename T>
    void registerMemory(const memory::MemoryModuleType<T>* type) {
        m_memories[type] = std::nullopt;
    }

    /**
     * @brief 注册传感器
     */
    void registerSensor(std::unique_ptr<sensor::Sensor<E>> sensor) {
        if (sensor) {
            // 注册传感器使用的内存模块
            for (auto* memType : sensor->getUsedMemories()) {
                if (m_memories.find(memType) == m_memories.end()) {
                    m_memories[memType] = std::nullopt;
                }
            }
            m_sensors.push_back(std::move(sensor));
        }
    }

    /**
     * @brief 设置日程
     */
    void setSchedule(const schedule::Schedule& schedule) {
        m_schedule = schedule;
    }

    /**
     * @brief 获取日程
     */
    [[nodiscard]] const schedule::Schedule& getSchedule() const {
        return m_schedule;
    }

    /**
     * @brief 设置后备活动
     */
    void setFallbackActivity(const schedule::Activity& activity) {
        m_fallbackActivity = activity;
    }

    /**
     * @brief 设置默认活动
     */
    void setDefaultActivities(const std::unordered_set<schedule::Activity>& activities) {
        m_defaultActivities = activities;
    }

    /**
     * @brief 注册活动
     * @param activity 活动
     * @param priority 优先级
     * @param tasks 任务列表
     * @param requirements 记忆要求
     * @param memoriesToForget 切换活动时要遗忘的记忆
     */
    void registerActivity(
        const schedule::Activity& activity,
        i32 priority,
        std::vector<std::unique_ptr<task::Task<E>>> tasks,
        const std::unordered_set<MemoryRequirement>& requirements = {},
        const std::unordered_set<const memory::MemoryModuleTypeBase*>& memoriesToForget = {}) {

        if (!requirements.empty()) {
            m_requiredMemoryStates[activity] = requirements;
        }

        if (!memoriesToForget.empty()) {
            m_memoriesToForget[activity] = memoriesToForget;
        }

        for (auto& task : tasks) {
            m_tasks[priority][activity].insert(std::move(task));
        }
    }

    /**
     * @brief 切换到指定活动
     */
    void switchTo(const schedule::Activity& activity) {
        if (hasRequiredMemories(activity)) {
            startActivity(activity);
        } else {
            startActivity(m_fallbackActivity);
        }
    }

    /**
     * @brief 检查是否正在进行某活动
     */
    [[nodiscard]] bool hasActivity(const schedule::Activity& activity) const {
        return m_activities.find(activity) != m_activities.end();
    }

    /**
     * @brief 获取当前活动
     */
    [[nodiscard]] std::optional<schedule::Activity> getCurrentActivity() const {
        for (const auto& activity : m_activities) {
            if (m_defaultActivities.find(activity) == m_defaultActivities.end()) {
                return activity;
            }
        }
        return std::nullopt;
    }

    // ========== 内存操作 ==========

    /**
     * @brief 检查是否有某个记忆
     */
    [[nodiscard]] bool hasMemory(const memory::MemoryModuleTypeBase* type) const {
        return hasMemory(type, memory::MemoryModuleStatus::VALUE_PRESENT);
    }

    /**
     * @brief 检查记忆状态
     */
    [[nodiscard]] bool hasMemory(const memory::MemoryModuleTypeBase* type,
                                  memory::MemoryModuleStatus status) const {
        auto it = m_memories.find(type);
        if (it == m_memories.end()) {
            return status == memory::MemoryModuleStatus::REGISTERED;
        }

        const auto& memoryOpt = it->second;
        switch (status) {
            case memory::MemoryModuleStatus::VALUE_PRESENT:
                return memoryOpt.has_value();
            case memory::MemoryModuleStatus::VALUE_ABSENT:
                return !memoryOpt.has_value();
            case memory::MemoryModuleStatus::REGISTERED:
                return true;
        }
        return false;
    }

    /**
     * @brief 获取记忆值
     */
    template <typename T>
    std::optional<T> getMemory(const memory::MemoryModuleType<T>* type) const {
        auto it = m_memories.find(type);
        if (it == m_memories.end() || !it->second.has_value()) {
            return std::nullopt;
        }
        // 注意：这里需要类型转换，实际实现可能需要更复杂的存储
        auto* typedMemory = static_cast<const memory::Memory<T>*>(it->second.value().getValue());
        if (typedMemory) {
            return typedMemory->getValue();
        }
        return std::nullopt;
    }

    /**
     * @brief 设置记忆(永久)
     */
    template <typename T>
    void setMemory(const memory::MemoryModuleType<T>* type, const T& value) {
        auto it = m_memories.find(type);
        if (it != m_memories.end()) {
            m_memories[type] = memory::Memory<void*>(reinterpret_cast<void*>(const_cast<T*>(&value)));
        }
    }

    /**
     * @brief 设置记忆(带TTL)
     */
    template <typename T>
    void setMemoryWithTTL(const memory::MemoryModuleType<T>* type, const T& value, i64 ttl) {
        auto it = m_memories.find(type);
        if (it != m_memories.end()) {
            m_memories[type] = memory::Memory<void*>(reinterpret_cast<void*>(const_cast<T*>(&value)), ttl);
        }
    }

    /**
     * @brief 移除记忆
     */
    template <typename T>
    void removeMemory(const memory::MemoryModuleType<T>* type) {
        m_memories[type] = std::nullopt;
    }

    // ========== Tick更新 ==========

    /**
     * @brief 每tick更新
     */
    void tick(ServerWorld* world, E* entity, i64 gameTime, i32 dayTime) {
        // 更新记忆TTL
        tickMemories();

        // 更新传感器
        tickSensors(world, entity);

        // 根据日程更新活动
        updateActivity(dayTime, gameTime);

        // 启动符合条件的任务
        startTasks(world, entity, gameTime);

        // 更新运行中的任务
        tickTasks(world, entity, gameTime);
    }

    /**
     * @brief 停止所有任务
     */
    void stopAllTasks(ServerWorld* world, E* owner, i64 gameTime) {
        for (auto& runningTask : getRunningTasks()) {
            runningTask->stop(world, owner, gameTime);
        }
    }

    /**
     * @brief 重置Brain状态
     */
    void clear() {
        m_memories.clear();
        m_activities.clear();
        m_activities.insert(m_defaultActivities.begin(), m_defaultActivities.end());
    }

private:
    /**
     * @brief 更新记忆TTL
     */
    void tickMemories() {
        std::vector<const memory::MemoryModuleTypeBase*> toRemove;

        for (auto& [type, memoryOpt] : m_memories) {
            if (memoryOpt.has_value()) {
                auto& memory = memoryOpt.value();
                memory.tick();
                if (memory.isExpired()) {
                    toRemove.push_back(type);
                }
            }
        }

        for (auto* type : toRemove) {
            m_memories[type] = std::nullopt;
        }
    }

    /**
     * @brief 更新传感器
     */
    void tickSensors(ServerWorld* world, E* entity) {
        for (auto& sensor : m_sensors) {
            sensor->tick(world, entity);
        }
    }

    /**
     * @brief 根据日程更新活动
     */
    void updateActivity(i32 dayTime, i64 gameTime) {
        if (gameTime - m_lastGameTime > 20) {
            m_lastGameTime = gameTime;
            auto scheduledActivity = m_schedule.getScheduledActivity(dayTime);
            if (!hasActivity(scheduledActivity)) {
                switchTo(scheduledActivity);
            }
        }
    }

    /**
     * @brief 检查是否有活动所需的记忆
     */
    [[nodiscard]] bool hasRequiredMemories(const schedule::Activity& activity) const {
        auto it = m_requiredMemoryStates.find(activity);
        if (it == m_requiredMemoryStates.end()) {
            return true;
        }

        for (const auto& [memType, status] : it->second) {
            if (!hasMemory(memType, status)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 开始活动
     */
    void startActivity(const schedule::Activity& activity) {
        if (hasActivity(activity)) {
            return;
        }

        // 遗忘切换前的记忆
        for (const auto& act : m_activities) {
            if (act != activity) {
                auto it = m_memoriesToForget.find(act);
                if (it != m_memoriesToForget.end()) {
                    for (auto* memType : it->second) {
                        m_memories[memType] = std::nullopt;
                    }
                }
            }
        }

        m_activities.clear();
        m_activities.insert(m_defaultActivities.begin(), m_defaultActivities.end());
        m_activities.insert(activity);
    }

    /**
     * @brief 启动任务
     */
    void startTasks(ServerWorld* world, E* entity, i64 gameTime) {
        for (auto& [priority, activityMap] : m_tasks) {
            for (auto& [activity, taskSet] : activityMap) {
                if (hasActivity(activity)) {
                    for (auto& task : taskSet) {
                        if (task->getStatus() == task::TaskStatus::STOPPED) {
                            task->start(world, entity, gameTime);
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief 更新任务
     */
    void tickTasks(ServerWorld* world, E* entity, i64 gameTime) {
        for (auto& task : getRunningTasks()) {
            task->tick(world, entity, gameTime);
        }
    }

    /**
     * @brief 获取运行中的任务
     */
    std::vector<task::Task<E>*> getRunningTasks() {
        std::vector<task::Task<E>*> result;
        for (auto& [priority, activityMap] : m_tasks) {
            for (auto& [activity, taskSet] : activityMap) {
                for (auto& task : taskSet) {
                    if (task->getStatus() == task::TaskStatus::RUNNING) {
                        result.push_back(task.get());
                    }
                }
            }
        }
        return result;
    }

    // 数据成员
    std::unordered_map<const memory::MemoryModuleTypeBase*, std::optional<memory::Memory<void*>>> m_memories;
    std::vector<std::unique_ptr<sensor::Sensor<E>>> m_sensors;
    PriorityTaskMap m_tasks;
    schedule::Schedule m_schedule;
    ActivityRequirementMap m_requiredMemoryStates;
    ForgettingMap m_memoriesToForget;
    std::unordered_set<schedule::Activity> m_defaultActivities;
    std::unordered_set<schedule::Activity> m_activities;
    schedule::Activity m_fallbackActivity = schedule::Activity::IDLE;
    i64 m_lastGameTime = -9999;
};

} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
