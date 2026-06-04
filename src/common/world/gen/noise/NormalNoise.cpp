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
 */

#include "common/world/gen/noise/NormalNoise.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc::world::gen::noise {

NormalNoise::NormalNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
    : m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
    , m_first(seed, firstOctave, m_amplitudes)
    , m_second(seed ^ 0xDEADBEEFULL, firstOctave, m_amplitudes)
{
    // 查找非零振幅的范围
    const i32 octaveCount = static_cast<i32>(m_amplitudes.size());
    i32 minNonZero = std::numeric_limits<i32>::max();
    i32 maxNonZero = std::numeric_limits<i32>::min();

    for (i32 i = 0; i < octaveCount; ++i) {
        if (m_amplitudes[static_cast<size_t>(i)] != 0.0) {
            minNonZero = std::min(minNonZero, i);
            maxNonZero = std::max(maxNonZero, i);
        }
    }

    const i32 octaveRange = maxNonZero - minNonZero;
    m_valueFactor = VALUE_FACTOR_BASE / expectedDeviation(octaveRange);
    m_maxValue = (m_first.maxValue() + m_second.maxValue()) * m_valueFactor;
}

f64 NormalNoise::getValue(f64 x, f64 y, f64 z) const
{
    // 第二个噪声使用缩放坐标（乘以 INPUT_FACTOR）
    const f64 sx = x * INPUT_FACTOR;
    const f64 sy = y * INPUT_FACTOR;
    const f64 sz = z * INPUT_FACTOR;

    return (m_first.getValue(x, y, z) + m_second.getValue(sx, sy, sz)) * m_valueFactor;
}

f64 NormalNoise::expectedDeviation(i32 octaveRange)
{
    return 0.1 * (1.0 + 1.0 / static_cast<f64>(octaveRange + 1));
}

} // namespace mc::world::gen::noise
