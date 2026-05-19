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
