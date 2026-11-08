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

namespace mc {
class BlockState;
class BlockPos;

/** @brief 可由多个方块共享的状态判定，实例地址在注册表生命周期内保持稳定。 */
class StatePredicate {
public:
    using Fn = bool (*)(const BlockState& state, const BlockPos& pos);

    /**
     * @param fn     判定函数；参数依次为状态与查询位置
     * @param source 该实例来源的可读名字，仅在注册形态改动时用于核对，不参与任何判定
     */
    constexpr StatePredicate(Fn fn, const char* source) noexcept
        : m_fn(fn)
        , m_source(source)
    {}

    /** @brief 求值。 */
    [[nodiscard]] bool test(const BlockState& state, const BlockPos& pos) const
    {
        return m_fn(state, pos);
    }

    /** @brief 来源名字（诊断用）。 */
    [[nodiscard]] const char* source() const noexcept
    {
        return m_source;
    }

private:
    Fn m_fn;
    const char* m_source;
};
} // namespace mc
