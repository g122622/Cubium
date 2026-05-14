#pragma once

#include "../../../../core/Types.hpp"
#include "Activity.hpp"
#include "DutyTime.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

class ScheduleDuties;

/**
 * @brief 日程安排
 *
 * 定义实体在不同时间的活动安排
 * 参考 MC 1.16.5 Schedule
 */
class Schedule {
public:
    Schedule();
    ~Schedule() = default;

    // 禁止拷贝
    Schedule(const Schedule&) = delete;
    Schedule& operator=(const Schedule&) = delete;

    // 允许移动
    Schedule(Schedule&&) = default;
    Schedule& operator=(Schedule&&) = default;

    /**
     * @brief 添加一个时间点的活动
     * @param dayTime 一天中的时间(0-23999 ticks)
     * @param activity 该时间的活动
     * @return this 用于链式调用
     */
    Schedule& add(i32 dayTime, const Activity& activity);

    /**
     * @brief 构建日程(链式调用结束)
     * @return 构建完成的日程
     */
    Schedule build();

    /**
     * @brief 获取指定时间应该进行的活动
     * @param dayTime 一天中的时间(0-23999 ticks)
     * @return 应该进行的活动
     */
    [[nodiscard]] Activity getScheduledActivity(i32 dayTime) const;

    // 预定义日程
    static Schedule EMPTY;
    static Schedule SIMPLE;
    static Schedule VILLAGER_BABY;
    static Schedule VILLAGER_DEFAULT;

    /**
     * @brief 初始化所有预定义日程
     */
    static void initialize();

protected:
    friend class ScheduleBuilder;

    void createDutiesFor(const Activity& activity);
    ScheduleDuties* getDutiesFor(const Activity& activity);
    std::vector<ScheduleDuties*> getAllDutiesExcept(const Activity& activity);

private:
    std::unordered_map<Activity, std::unique_ptr<ScheduleDuties>> m_duties;
};

/**
 * @brief 日程构建器
 */
class ScheduleBuilder {
public:
    explicit ScheduleBuilder(Schedule& schedule);

    ScheduleBuilder& add(i32 dayTime, const Activity& activity);
    Schedule& build();

private:
    struct ActivityEntry {
        ActivityEntry(i32 dayTime, const Activity& activity)
            : m_dayTime(dayTime)
            , m_activity(activity)
        {}

        i32 m_dayTime;
        Activity m_activity;
    };

    Schedule& m_schedule;
    std::vector<ActivityEntry> m_entries;
};

/**
 * @brief 单个活动的时段管理
 */
class ScheduleDuties {
public:
    ScheduleDuties& addDutyTime(i32 dayTime, f32 weight);
    [[nodiscard]] f32 getWeightAt(i32 dayTime) const;

private:
    void rebuildDutyTimes();

    std::vector<DutyTime> m_dutyTimes;
    mutable std::size_t m_lastIndex = 0;
};

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
