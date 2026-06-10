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

#include "EnvironmentScanPredicate.hpp"

namespace mc::world::gen::feature::predicate {

bool EnvironmentScanPredicate::scan(const IWorld& world, BlockPos& startPos) const
{
    BlockPos current = startPos;
    for (i32 i = 0; i < m_maxSteps; ++i) {
        // 先检查终止条件
        if (m_abortCondition && m_abortCondition->test(world, current)) {
            return false;
        }
        // 检查目标条件
        if (m_targetCondition->test(world, current)) {
            startPos = current;
            return true;
        }
        // 沿方向前进一步
        current = current.offset(m_direction);
    }
    return false;
}

} // namespace mc::world::gen::feature::predicate
