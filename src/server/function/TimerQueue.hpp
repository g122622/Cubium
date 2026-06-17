#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <algorithm>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {
namespace function {

/**
 * @brief 定时器队列 - 调度延迟执行的函数事件
 *
 * 使用优先队列（按触发时间排序）和哈希表（按事件名称索引）
 * 实现高效的按 tick 调度和按名称移除。
 *
 * 事件通过字符串 ID 标识，同一 ID + 同一触发时间的事件不会重复添加。
 * /schedule function 命令使用函数名作为 ID，/schedule clear 使用 ID 移除事件。
 *
 * 事件在 tick() 中被处理：所有 triggerTime <= currentTick 的事件
 * 将被收集并返回给调用者执行。
 *
 * 调度的事件在服务器重启后不持久化。
 *
 * TODO: 调度事件持久化到 level.dat。需要实现：
 * 1) 事件序列化到 NBT 2) 从 level.dat 加载恢复 3) 服务器关闭时保存
 */
class TimerQueue {
public:
    /**
     * @brief 定时器事件类型
     */
    enum class EventType : u8 {
        Function,    ///< 单个函数
        FunctionTag, ///< 函数标签
    };

    /**
     * @brief 到期的事件
     */
    struct DueEvent {
        std::string id;       ///< 事件标识符（函数名或 #函数标签名）
        EventType type;       ///< 事件类型
        ResourceLocation loc; ///< 函数或标签的 ResourceLocation
    };

    /**
     * @brief 定时器事件
     */
    struct Event {
        u64 triggerTime;      ///< 触发的游戏 tick
        u64 sequentialId;     ///< 插入顺序（用于同 tick 事件排序）
        std::string id;       ///< 事件标识符
        EventType type;       ///< 事件类型
        ResourceLocation loc; ///< 函数或标签的 ResourceLocation

        /**
         * @brief 事件比较器（用于优先队列，按触发时间和插入顺序排序）
         */
        bool operator>(const Event& other) const
        {
            if (triggerTime != other.triggerTime) {
                return triggerTime > other.triggerTime;
            }
            return sequentialId > other.sequentialId;
        }
    };

    TimerQueue() = default;
    ~TimerQueue() = default;

    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;
    TimerQueue(TimerQueue&&) = default;
    TimerQueue& operator=(TimerQueue&&) = default;

    /**
     * @brief 调度一个函数事件
     *
     * 如果同一 id + triggerTime 的事件已存在，则不会重复添加。
     *
     * @param id 事件标识符（函数名，如 "minecraft:tick"）
     * @param loc 函数的 ResourceLocation
     * @param triggerTime 触发的游戏 tick
     */
    void scheduleFunction(const std::string& id, ResourceLocation loc, u64 triggerTime);

    /**
     * @brief 调度一个函数标签事件
     *
     * @param id 事件标识符（带 # 前缀，如 "#minecraft:tick"）
     * @param loc 标签的 ResourceLocation
     * @param triggerTime 触发的游戏 tick
     */
    void scheduleFunctionTag(const std::string& id, ResourceLocation loc, u64 triggerTime);

    /**
     * @brief 移除指定 ID 的所有事件
     * @return 移除的事件数量
     */
    i32 remove(const std::string& id);

    /**
     * @brief 处理当前 tick 到期的事件
     *
     * 遍历优先队列，收集所有 triggerTime <= currentTick 的事件。
     * 返回的事件列表由调用者负责执行。
     *
     * @param currentTick 当前游戏 tick
     * @return 到期的事件列表
     */
    std::vector<DueEvent> tick(u64 currentTick);

    /**
     * @brief 获取所有已调度事件的 ID 集合
     */
    [[nodiscard]] std::vector<std::string> getEventIds() const;

    /**
     * @brief 检查是否有待调度的事件
     */
    [[nodiscard]] bool isEmpty() const noexcept;

    /**
     * @brief 获取待调度事件数量
     */
    [[nodiscard]] Size size() const noexcept;

    /**
     * @brief 清空所有事件
     */
    void clear();

private:
    void scheduleInternal(const std::string& id, ResourceLocation loc, u64 triggerTime, EventType type);

    /// 优先队列（最小堆，按触发时间排序）
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> m_queue;

    /// 哈希表索引：(event_id, trigger_time) -> 是否存在（用于去重）
    struct EventKey {
        std::string id;
        u64 triggerTime;

        bool operator==(const EventKey& other) const { return id == other.id && triggerTime == other.triggerTime; }
    };

    struct EventKeyHash {
        Size operator()(const EventKey& key) const
        {
            Size h1 = std::hash<std::string>{}(key.id);
            Size h2 = std::hash<u64>{}(key.triggerTime);
            return h1 ^ (h2 << 1);
        }
    };

    std::unordered_set<EventKey, EventKeyHash> m_scheduledKeys;

    /// 递增序号，用于同 tick 事件排序
    u64 m_nextSequentialId = 0;
};

} // namespace function
} // namespace mc
