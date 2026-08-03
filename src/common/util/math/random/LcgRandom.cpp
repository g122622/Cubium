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

#include "LcgRandom.hpp"
#include "common/core/Types.hpp"

namespace mc::math {

LcgRandom::LcgRandom(u64 seed)
    : m_state(seed)
{
    // 确保初始状态不为零（零会导致所有输出都相同）
    if (m_state == 0) {
        m_state = 1;
    }
}

void LcgRandom::setSeed(u64 seed)
{
    m_state = seed;
    if (m_state == 0) {
        m_state = 1;
    }
    m_hasGaussian = false;
}

u64 LcgRandom::nextU64()
{
    // 线性同余生成器
    // X_{n+1} = (a * X_n + c) mod 2^64
    m_state = A * m_state + C;
    return m_state;
}

} // namespace mc::math
