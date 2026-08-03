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

#include "server/stats/Stat.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/stats/StatType.hpp"
#include <limits>
#include <utility>

namespace mc {
namespace server {
namespace stats {

Stat::Stat(StatType type, ResourceLocation id) noexcept
    : m_type(type)
    , m_id(std::move(id))
    , m_value(0)
{}

ResourceLocation Stat::getFullLocation() const
{
    return buildStatLocation(m_type, m_id);
}

void Stat::setValue(ValueType value) noexcept
{
    m_value = value;
}

void Stat::increment(ValueType delta) noexcept
{
    // 防止溢出
    if (delta > 0 && m_value > std::numeric_limits<ValueType>::max() - delta) {
        m_value = std::numeric_limits<ValueType>::max();
    } else if (delta < 0 && m_value < std::numeric_limits<ValueType>::min() - delta) {
        m_value = std::numeric_limits<ValueType>::min();
    } else {
        m_value += delta;
    }
}

} // namespace stats
} // namespace server
} // namespace mc
