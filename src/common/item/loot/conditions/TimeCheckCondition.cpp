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

#include "common/item/loot/conditions/TimeCheckCondition.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace loot {

TimeCheckCondition::TimeCheckCondition(i64 period, RandomValueRange value)
    : m_period(period)
    , m_value(std::move(value))
    , m_hasPeriod(period > 0)
{}

bool TimeCheckCondition::test(LootContext& context) const
{
    IWorld& world = context.getWorld();
    // 使用 dayTimeOfDay() 获取一天内的时间 (0-23999)
    // 如果有周期(period)，则对周期取模
    i64 dayTime = world.dayTimeOfDay();

    if (m_hasPeriod && m_period > 0) {
        dayTime = dayTime % m_period;
    }

    i32 timeValue = static_cast<i32>(dayTime);
    f32 floatTime = static_cast<f32>(timeValue);
    return floatTime >= m_value.getMin() && floatTime <= m_value.getMax();
}

std::unique_ptr<LootCondition> TimeCheckCondition::clone() const noexcept
{
    return std::make_unique<TimeCheckCondition>(m_period, m_value);
}

} // namespace loot
} // namespace mc
