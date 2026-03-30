#include "Schedule.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

// 静态成员初始化
Schedule Schedule::EMPTY;
Schedule Schedule::SIMPLE;
Schedule Schedule::VILLAGER_BABY;
Schedule Schedule::VILLAGER_DEFAULT;

Schedule::Schedule() {}

Schedule& Schedule::add(i32 dayTime, const Activity& activity) {
    createDutiesFor(activity);
    auto* duties = getDutiesFor(activity);
    if (duties) {
        duties->addDutyTime(dayTime);
    }
    return *this;
}

Schedule Schedule::build() {
    return std::move(*this);
}

Activity Schedule::getScheduledActivity(i32 dayTime) const {
    // 规范化时间到一天的范围
    dayTime = dayTime % 24000;

    const Activity* bestActivity = nullptr;
    f32 maxWeight = -1.0f;

    for (const auto& pair : m_duties) {
        const Activity& activity = pair.first;
        const auto& duties = pair.second;
        f32 weight = duties->getWeightAt(dayTime);
        if (weight > maxWeight) {
            maxWeight = weight;
            bestActivity = &activity;
        }
    }

    return bestActivity ? *bestActivity : Activity::IDLE;
}

void Schedule::createDutiesFor(const Activity& activity) {
    if (m_duties.find(activity) == m_duties.end()) {
        m_duties[activity] = std::make_unique<ScheduleDuties>();
    }
}

ScheduleDuties* Schedule::getDutiesFor(const Activity& activity) {
    auto it = m_duties.find(activity);
    if (it != m_duties.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<ScheduleDuties*> Schedule::getAllDutiesExcept(const Activity& activity) {
    std::vector<ScheduleDuties*> result;
    for (auto& pair : m_duties) {
        if (pair.first != activity) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

void Schedule::initialize() {
    // EMPTY - 空日程
    EMPTY = Schedule().add(0, Activity::IDLE).build();

    // SIMPLE - 简单日程
    SIMPLE = Schedule()
        .add(0, Activity::IDLE)
        .add(5000, Activity::WORK)
        .add(11000, Activity::REST)
        .build();

    // VILLAGER_BABY - 村民婴儿日程
    VILLAGER_BABY = Schedule()
        .add(0, Activity::IDLE)
        .add(10, Activity::IDLE)
        .add(3000, Activity::PLAY)
        .add(6000, Activity::IDLE)
        .add(10000, Activity::PLAY)
        .add(12000, Activity::REST)
        .build();

    // VILLAGER_DEFAULT - 村民默认日程
    VILLAGER_DEFAULT = Schedule()
        .add(0, Activity::IDLE)
        .add(10, Activity::IDLE)
        .add(2000, Activity::WORK)
        .add(9000, Activity::MEET)
        .add(11000, Activity::IDLE)
        .add(12000, Activity::REST)
        .build();
}

// ScheduleBuilder
ScheduleBuilder::ScheduleBuilder(Schedule& schedule)
    : m_schedule(schedule) {}

ScheduleBuilder& ScheduleBuilder::add(i32 dayTime, const Activity& activity) {
    m_schedule.add(dayTime, activity);
    return *this;
}

Schedule ScheduleBuilder::build() {
    return std::move(m_schedule);
}

// ScheduleDuties
void ScheduleDuties::addDutyTime(i32 dayTime, f32 weight) {
    m_dutyTimes[dayTime] = weight;
}

f32 ScheduleDuties::getWeightAt(i32 dayTime) const {
    if (m_dutyTimes.empty()) {
        return 0.0f;
    }

    // 找到最近的时间点
    auto it = m_dutyTimes.lower_bound(dayTime);
    if (it == m_dutyTimes.end()) {
        // 时间大于所有时间点，使用最后一个
        return m_dutyTimes.rbegin()->second;
    }
    if (it == m_dutyTimes.begin()) {
        return it->second;
    }

    // 线性插值
    auto prev = std::prev(it);
    i32 prevTime = prev->first;
    i32 nextTime = it->first;
    f32 prevWeight = prev->second;
    f32 nextWeight = it->second;

    if (nextTime == prevTime) {
        return nextWeight;
    }

    f32 t = static_cast<f32>(dayTime - prevTime) / static_cast<f32>(nextTime - prevTime);
    return prevWeight + t * (nextWeight - prevWeight);
}

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
