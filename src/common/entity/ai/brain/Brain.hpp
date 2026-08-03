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

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "memory/Memory.hpp"
#include "memory/MemoryModuleStatus.hpp"
#include "memory/MemoryModuleType.hpp"
#include "schedule/Activity.hpp"
#include "schedule/Schedule.hpp"
#include "sensor/Sensor.hpp"
#include "sensor/SensorType.hpp"
#include "task/Task.hpp"
#include <any>
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// std::hash 特化用于 MemoryRequirement pair
namespace std {
template <>
struct hash<std::pair<const mc::entity::ai::brain::memory::MemoryModuleTypeBase*,
    mc::entity::ai::brain::memory::MemoryModuleStatus>> {
    size_t operator()(const std::pair<const mc::entity::ai::brain::memory::MemoryModuleTypeBase*,
        mc::entity::ai::brain::memory::MemoryModuleStatus>& p) const noexcept
    {
        auto h1 = reinterpret_cast<size_t>(p.first);
        auto h2 = static_cast<size_t>(p.second);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std

namespace mc {

// Forward declarations
class LivingEntity;
class IWorld;

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
 * @tparam E 实体类型
 */
template <typename E>
class Brain {
public:
    // 使用 std::any 存储类型安全的记忆值
    // 存储的值是 Memory<T> 类型，通过 std::any 进行类型擦除
    using MemoryMap = std::unordered_map<const memory::MemoryModuleTypeBase*, std::optional<std::any>>;
    using SensorMap = std::unordered_map<std::string, std::unique_ptr<sensor::Sensor<E>>>;
    using TaskSet = std::unordered_set<std::unique_ptr<task::Task<E>>>;
    using ActivityTaskMap = std::unordered_map<schedule::Activity, TaskSet>;
    // 使用 TreeMap 保证优先级顺序（数值小的先执行）
    using PriorityTaskMap = std::map<i32, ActivityTaskMap>;
    using MemoryRequirement = std::pair<const memory::MemoryModuleTypeBase*, memory::MemoryModuleStatus>;
    using ActivityRequirementMap = std::unordered_map<schedule::Activity, std::unordered_set<MemoryRequirement>>;
    using ForgettingMap =
        std::unordered_map<schedule::Activity, std::unordered_set<const memory::MemoryModuleTypeBase*>>;

    /**
     * @brief 构造Brain
     */
    Brain() noexcept = default;

    /**
     * @brief 注册内存模块类型
     */
    template <typename T>
    void registerMemory(const memory::MemoryModuleType<T>* type)
    {
        m_memories[type] = std::nullopt;
    }

    /**
     * @brief 注册传感器
     */
    void registerSensor(std::unique_ptr<sensor::Sensor<E>> sensor)
    {
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
     * @param schedule 日程指针（通常指向静态日程实例）
     */
    void setSchedule(const schedule::Schedule* schedule) noexcept { m_schedulePtr = schedule; }

    /**
     * @brief 获取日程
     */
    [[nodiscard]] const schedule::Schedule* getSchedule() const noexcept { return m_schedulePtr; }

    /**
     * @brief 设置后备活动
     */
    void setFallbackActivity(const schedule::Activity& activity) noexcept { m_fallbackActivity = activity; }

    /**
     * @brief 设置默认活动
     */
    void setDefaultActivities(const std::unordered_set<schedule::Activity>& activities)
    {
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
    void registerActivity(const schedule::Activity& activity,
        i32 priority,
        std::vector<std::unique_ptr<task::Task<E>>> tasks,
        const std::unordered_set<MemoryRequirement>& requirements = {},
        const std::unordered_set<const memory::MemoryModuleTypeBase*>& memoriesToForget = {})
    {

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
    void switchTo(const schedule::Activity& activity)
    {
        if (_hasRequiredMemories(activity)) {
            _startActivity(activity);
        } else {
            _startActivity(m_fallbackActivity);
        }
    }

    /**
     * @brief 检查是否正在进行某活动
     */
    [[nodiscard]] bool hasActivity(const schedule::Activity& activity) const noexcept
    {
        return m_activities.find(activity) != m_activities.end();
    }

    /**
     * @brief 获取当前活动
     */
    [[nodiscard]] std::optional<schedule::Activity> getCurrentActivity() const
    {
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
    [[nodiscard]] bool hasMemory(const memory::MemoryModuleTypeBase* type) const noexcept
    {
        return hasMemory(type, memory::MemoryModuleStatus::VALUE_PRESENT);
    }

    /**
     * @brief 检查记忆状态
     *
     * 未注册的记忆任何状态都返回false
     */
    [[nodiscard]] bool hasMemory(
        const memory::MemoryModuleTypeBase* type, memory::MemoryModuleStatus status) const noexcept
    {
        auto it = m_memories.find(type);
        if (it == m_memories.end()) {
            // 未注册的记忆返回false，即使状态是REGISTERED
            return false;
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
     * @tparam T 记忆值类型
     * @param type 记忆模块类型
     * @return 记忆值，如果不存在或类型不匹配返回 std::nullopt
     */
    template <typename T>
    std::optional<T> getMemory(const memory::MemoryModuleType<T>* type) const
    {
        auto it = m_memories.find(type);
        if (it == m_memories.end() || !it->second.has_value()) {
            return std::nullopt;
        }
        try {
            // 尝试从 std::any 中提取 Memory<T>
            const auto& anyValue = it->second.value();
            if (anyValue.type() == typeid(memory::Memory<T>)) {
                const auto& memory = std::any_cast<const memory::Memory<T>&>(anyValue);
                return memory.getValue();
            }
        }
        catch (const std::bad_any_cast&) {
            // 类型不匹配，返回 nullopt
        }
        return std::nullopt;
    }

    /**
     * @brief 设置记忆(永久)
     * @tparam T 记忆值类型
     * @param type 记忆模块类型
     * @param value 记忆值
     */
    template <typename T>
    void setMemory(const memory::MemoryModuleType<T>* type, const T& value)
    {
        m_memories[type] = memory::Memory<T>::permanent(value);
        m_memoryTTL[type] = std::numeric_limits<i64>::max(); // 永不过期
    }

    /**
     * @brief 设置记忆(带TTL)
     * @tparam T 记忆值类型
     * @param type 记忆模块类型
     * @param value 记忆值
     * @param ttl 存活时间(ticks)
     */
    template <typename T>
    void setMemoryWithTTL(const memory::MemoryModuleType<T>* type, const T& value, i64 ttl)
    {
        m_memories[type] = memory::Memory<T>::timed(value, ttl);
        m_memoryTTL[type] = ttl;
    }

    /**
     * @brief 移除记忆
     * @tparam T 记忆值类型
     * @param type 记忆模块类型
     */
    template <typename T>
    void removeMemory(const memory::MemoryModuleType<T>* type)
    {
        m_memories[type] = std::nullopt;
        m_memoryTTL.erase(type);
    }

    // ========== Tick更新 ==========

    /**
     * @brief 每tick更新
     * @param world 世界
     * @param entity 实体
     * @param gameTime 游戏时间
     * @param dayTime 日内时间
     * @param random 随机数生成器（用于任务持续时间）
     */
    void tick(IWorld* world, E* entity, i64 gameTime, i32 dayTime, math::Random& random)
    {
        // 更新记忆TTL
        _tickMemories();

        // 更新传感器
        _tickSensors(world, entity);

        // 根据日程更新活动
        _updateActivity(dayTime, gameTime);

        // 启动符合条件的任务
        _startTasks(world, entity, gameTime, random);

        // 更新运行中的任务
        _tickTasks(world, entity, gameTime);
    }

    /**
     * @brief 停止所有任务
     */
    void stopAllTasks(IWorld* world, E* owner, i64 gameTime)
    {
        for (auto& [priority, activityMap] : m_tasks) {
            for (auto& [activity, taskSet] : activityMap) {
                for (auto& task : taskSet) {
                    if (task->getStatus() == task::TaskStatus::RUNNING) {
                        task->stop(world, owner, gameTime);
                    }
                }
            }
        }
    }

    /**
     * @brief 重置Brain状态
     */
    void clear() noexcept
    {
        m_memories.clear();
        m_activities.clear();
        m_activities.insert(m_defaultActivities.begin(), m_defaultActivities.end());
    }

private:
    /**
     * @brief 更新记忆TTL
     */
    void _tickMemories()
    {
        auto it = m_memoryTTL.begin();
        while (it != m_memoryTTL.end()) {
            if (it->second != std::numeric_limits<i64>::max()) {
                it->second--;
                if (it->second <= 0) {
                    m_memories[it->first] = std::nullopt;
                    it = m_memoryTTL.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    /**
     * @brief 更新传感器
     */
    void _tickSensors(IWorld* world, E* entity)
    {
        for (auto& sensor : m_sensors) {
            sensor->tick(world, entity);
        }
    }

    /**
     * @brief 根据日程更新活动
     */
    void _updateActivity(i32 dayTime, i64 gameTime)
    {
        if (m_schedulePtr && gameTime - m_lastGameTime > 20) {
            m_lastGameTime = gameTime;
            auto scheduledActivity = m_schedulePtr->getScheduledActivity(dayTime);
            if (!hasActivity(scheduledActivity)) {
                switchTo(scheduledActivity);
            }
        }
    }

    /**
     * @brief 检查是否有活动所需的记忆
     *
     * 如果活动没有配置记忆要求，返回 false
     */
    [[nodiscard]] bool _hasRequiredMemories(const schedule::Activity& activity) const noexcept
    {
        auto it = m_requiredMemoryStates.find(activity);
        if (it == m_requiredMemoryStates.end()) {
            // 活动没有记忆要求配置时返回 false
            return false;
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
    void _startActivity(const schedule::Activity& activity)
    {
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
    void _startTasks(IWorld* world, E* entity, i64 gameTime, math::Random& random)
    {
        for (auto& [priority, activityMap] : m_tasks) {
            for (auto& [activity, taskSet] : activityMap) {
                if (hasActivity(activity)) {
                    for (auto& task : taskSet) {
                        if (task->getStatus() == task::TaskStatus::STOPPED) {
                            task->start(world, entity, gameTime, random);
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief 更新任务
     */
    void _tickTasks(IWorld* world, E* entity, i64 gameTime)
    {
        for (auto& [priority, activityMap] : m_tasks) {
            for (auto& [activity, taskSet] : activityMap) {
                for (auto& task : taskSet) {
                    if (task->getStatus() == task::TaskStatus::RUNNING) {
                        task->tick(world, entity, gameTime);
                    }
                }
            }
        }
    }

    /**
     * @brief 获取运行中的任务
     */
    std::vector<task::Task<E>*> getRunningTasks()
    {
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
    // m_memories 存储 Memory<T> 类型的值（通过 std::any 类型擦除）
    MemoryMap m_memories;
    // m_memoryTTL 存储每个记忆的剩余TTL（用于过期检查）
    std::unordered_map<const memory::MemoryModuleTypeBase*, i64> m_memoryTTL;
    std::vector<std::unique_ptr<sensor::Sensor<E>>> m_sensors;
    PriorityTaskMap m_tasks;
    const schedule::Schedule* m_schedulePtr = nullptr;
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
