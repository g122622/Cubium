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

/**
 * @file RandomRanges.cpp
 * @brief 随机值范围工具类实现
 */

#include "RandomRanges.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace math {

i32 BinomialRange::generateInt(Random& random) const
{
    // 使用二项分布生成随机值
    // 进行n次试验，每次有p的概率成功
    i32 successes = 0;
    for (i32 i = 0; i < m_n; ++i) {
        if (random.nextFloat() < m_p) {
            ++successes;
        }
    }
    return successes;
}

} // namespace math
} // namespace mc
