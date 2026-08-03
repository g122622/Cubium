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

#include "Schedule.hpp"

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/entity/ai/brain/schedule/DutyTime.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

namespace {

i32 normalizeDayTime(i32 dayTime)
{
    constexpr i32 dayLength = mc::game::DAY_LENGTH_TICKS;
    const i32 normalized = dayTime % dayLength;
    return normalized < 0 ? normalized + dayLength : normalized;
}

Schedule buildSchedule(std::initializer_list<std::pair<i32, Activity>> entries)
{
    Schedule schedule;
    ScheduleBuilder builder(schedule);
    for (const auto& [dayTime, activity] : entries) {
        builder.add(dayTime, activity);
    }
    builder.build();
    return schedule;
}

} // namespace

Schedule Schedule::EMPTY;
Schedule Schedule::SIMPLE;
Schedule Schedule::VILLAGER_BABY;
Schedule Schedule::VILLAGER_DEFAULT;

Schedule::Schedule() = default;

Schedule& Schedule::add(i32 dayTime, const Activity& activity)
{
    _createDutiesFor(activity);
    for (ScheduleDuties* duties : _getAllDutiesExcept(activity)) {
        duties->addDutyTime(dayTime, 0.0f);
    }

    ScheduleDuties* duties = _getDutiesFor(activity);
    MC_ASSERT_NOT_NULL(duties);
    duties->addDutyTime(dayTime, 1.0f);
    return *this;
}

Schedule Schedule::build()
{
    return std::move(*this);
}

Activity Schedule::getScheduledActivity(i32 dayTime) const
{
    const i32 normalizedDayTime = normalizeDayTime(dayTime);
    const Activity* bestActivity = nullptr;
    f32 bestWeight = -1.0f;

    for (const auto& [activity, duties] : m_duties) {
        const f32 weight = duties->getWeightAt(normalizedDayTime);
        if (weight > bestWeight) {
            bestWeight = weight;
            bestActivity = &activity;
        }
    }

    return bestActivity != nullptr ? *bestActivity : Activity::IDLE;
}

void Schedule::_createDutiesFor(const Activity& activity)
{
    if (m_duties.find(activity) == m_duties.end()) {
        m_duties.emplace(activity, std::make_unique<ScheduleDuties>());
    }
}

ScheduleDuties* Schedule::_getDutiesFor(const Activity& activity)
{
    const auto iterator = m_duties.find(activity);
    return iterator != m_duties.end() ? iterator->second.get() : nullptr;
}

std::vector<ScheduleDuties*> Schedule::_getAllDutiesExcept(const Activity& activity)
{
    std::vector<ScheduleDuties*> duties;
    duties.reserve(m_duties.size());
    for (auto& [otherActivity, otherDuties] : m_duties) {
        if (otherActivity != activity) {
            duties.push_back(otherDuties.get());
        }
    }
    return duties;
}

void Schedule::initialize()
{
    EMPTY = buildSchedule({
        {0, Activity::IDLE},
    });

    SIMPLE = buildSchedule({
        {5000, Activity::WORK},
        {11000, Activity::REST},
    });

    VILLAGER_BABY = buildSchedule({
        {10, Activity::IDLE},
        {3000, Activity::PLAY},
        {6000, Activity::IDLE},
        {10000, Activity::PLAY},
        {12000, Activity::REST},
    });

    VILLAGER_DEFAULT = buildSchedule({
        {10, Activity::IDLE},
        {2000, Activity::WORK},
        {9000, Activity::MEET},
        {11000, Activity::IDLE},
        {12000, Activity::REST},
    });
}

ScheduleBuilder::ScheduleBuilder(Schedule& schedule)
    : m_schedule(schedule)
{}

ScheduleBuilder& ScheduleBuilder::add(i32 dayTime, const Activity& activity)
{
    m_entries.emplace_back(dayTime, activity);
    return *this;
}

Schedule& ScheduleBuilder::build()
{
    std::unordered_set<Activity> activities;
    for (const ActivityEntry& entry : m_entries) {
        activities.insert(entry.m_activity);
    }

    for (const Activity& activity : activities) {
        m_schedule._createDutiesFor(activity);
    }

    for (const ActivityEntry& entry : m_entries) {
        for (ScheduleDuties* duties : m_schedule._getAllDutiesExcept(entry.m_activity)) {
            duties->addDutyTime(entry.m_dayTime, 0.0f);
        }

        ScheduleDuties* duties = m_schedule._getDutiesFor(entry.m_activity);
        MC_ASSERT_NOT_NULL(duties);
        duties->addDutyTime(entry.m_dayTime, 1.0f);
    }

    return m_schedule;
}

ScheduleDuties& ScheduleDuties::addDutyTime(i32 dayTime, f32 weight)
{
    m_dutyTimes.emplace_back(dayTime, weight);
    _rebuildDutyTimes();
    return *this;
}

f32 ScheduleDuties::getWeightAt(i32 dayTime) const
{
    if (m_dutyTimes.empty()) {
        return 0.0f;
    }

    const DutyTime& current = m_dutyTimes[m_lastIndex];
    const DutyTime& last = m_dutyTimes.back();
    const bool wrapsToLast = dayTime < current.getDayTime();
    std::size_t startIndex = wrapsToLast ? 0 : m_lastIndex;
    f32 value = wrapsToLast ? last.getValue() : current.getValue();

    for (std::size_t index = startIndex; index < m_dutyTimes.size(); ++index) {
        const DutyTime& dutyTime = m_dutyTimes[index];
        if (dutyTime.getDayTime() > dayTime) {
            break;
        }

        m_lastIndex = index;
        value = dutyTime.getValue();
    }

    return value;
}

void ScheduleDuties::_rebuildDutyTimes()
{
    std::sort(m_dutyTimes.begin(), m_dutyTimes.end(), [](const DutyTime& lhs, const DutyTime& rhs) {
        return lhs.getDayTime() < rhs.getDayTime();
    });

    std::vector<DutyTime> deduplicated;
    deduplicated.reserve(m_dutyTimes.size());
    for (const DutyTime& dutyTime : m_dutyTimes) {
        if (!deduplicated.empty() && deduplicated.back().getDayTime() == dutyTime.getDayTime()) {
            deduplicated.back() = dutyTime;
        } else {
            deduplicated.push_back(dutyTime);
        }
    }

    m_dutyTimes = std::move(deduplicated);
    m_lastIndex = 0;
}

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
