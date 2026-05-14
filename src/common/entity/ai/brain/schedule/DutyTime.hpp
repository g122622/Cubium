#pragma once

#include "../../../../core/Types.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace schedule {

/**
 * @brief 单个日程时间片的权重记录
 *
 * 对齐 MC 1.16.5 DutyTime，用于表示某个时间点开始的活动权重。
 */
class DutyTime {
public:
    DutyTime(i32 dayTime, f32 value)
        : m_dayTime(dayTime)
        , m_value(value)
    {}

    [[nodiscard]] i32 getDayTime() const { return m_dayTime; }

    [[nodiscard]] f32 getValue() const { return m_value; }

private:
    i32 m_dayTime;
    f32 m_value;
};

} // namespace schedule
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
