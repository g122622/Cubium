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

// macOS系统头文件<mach/boolean.h>定义了TRUE/FALSE宏，与BooleanOp方法名冲突
#ifdef __APPLE__
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#undef TRUE
#undef FALSE
#endif

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include <functional>
#include <utility>

namespace mc {

/**
 * @brief 布尔操作接口
 *
 * 用于 VoxelShape 之间的布尔运算（并集、交集、差集等）。
 *
 * 操作类型：
 * - FALSE: 始终返回false
 * - NOT_OR: 非或运算 (!a && !b)
 * - ONLY_SECOND: 仅第二个 (b && !a)
 * - NOT_FIRST: 非第一个 (!a)
 * - ONLY_FIRST: 仅第一个 (a && !b)
 * - NOT_SECOND: 非第二个 (!b)
 * - NOT_SAME: 不等 (a != b)
 * - NOT_AND: 与非 (!a || !b)
 * - AND: 与运算 (a && b)
 * - SAME: 相等 (a == b)
 * - SECOND: 第二个 (b)
 * - CAUSES: 蕴含 (!a || b)
 * - FIRST: 第一个 (a)
 * - CAUSED_BY: 被蕴含 (a || !b)
 * - OR: 或运算 (a || b)
 * - TRUE: 始终返回true
 */
class BooleanOp {
public:
    using OpFunc = std::function<bool(bool, bool)>;

    BooleanOp() noexcept
        : m_func(nullptr)
    {}
    explicit BooleanOp(OpFunc func)
        : m_func(std::move(func))
    {}

    bool apply(bool a, bool b) const { return m_func ? m_func(a, b) : false; }

    explicit operator bool() const noexcept { return m_func != nullptr; }

    // 预定义操作
    static BooleanOp FALSE();
    static BooleanOp NOT_OR();
    static BooleanOp ONLY_SECOND();
    static BooleanOp NOT_FIRST();
    static BooleanOp ONLY_FIRST();
    static BooleanOp NOT_SECOND();
    static BooleanOp NOT_SAME();
    static BooleanOp NOT_AND();
    static BooleanOp AND();
    static BooleanOp SAME();
    static BooleanOp SECOND();
    static BooleanOp CAUSES();
    static BooleanOp FIRST();
    static BooleanOp CAUSED_BY();
    static BooleanOp OR();
    static BooleanOp TRUE();

private:
    OpFunc m_func;
};

// ============================================================================
// 预定义布尔操作实例
// ============================================================================

namespace BooleanOps {

/// 始终返回false
inline BooleanOp False()
{
    return BooleanOp();
}

/// 非或运算 (!a && !b)
inline BooleanOp NotOr()
{
    return BooleanOp([](bool a, bool b) { return !a && !b; });
}

/// 仅第二个 (b && !a)
inline BooleanOp OnlySecond()
{
    return BooleanOp([](bool a, bool b) { return b && !a; });
}

/// 非第一个 (!a)
inline BooleanOp NotFirst()
{
    return BooleanOp([](bool a, bool b) {
        (void)b;
        return !a;
    });
}

/// 仅第一个 (a && !b)
inline BooleanOp OnlyFirst()
{
    return BooleanOp([](bool a, bool b) { return a && !b; });
}

/// 非第二个 (!b)
inline BooleanOp NotSecond()
{
    return BooleanOp([](bool a, bool b) {
        (void)a;
        return !b;
    });
}

/// 不等 (a != b)
inline BooleanOp NotSame()
{
    return BooleanOp([](bool a, bool b) { return a != b; });
}

/// 与非 (!a || !b)
inline BooleanOp NotAnd()
{
    return BooleanOp([](bool a, bool b) { return !a || !b; });
}

/// 与运算 (a && b)
inline BooleanOp And()
{
    return BooleanOp([](bool a, bool b) { return a && b; });
}

/// 相等 (a == b)
inline BooleanOp Same()
{
    return BooleanOp([](bool a, bool b) { return a == b; });
}

/// 第二个 (b)
inline BooleanOp Second()
{
    return BooleanOp([](bool a, bool b) {
        (void)a;
        return b;
    });
}

/// 蕴含 (!a || b)
inline BooleanOp Causes()
{
    return BooleanOp([](bool a, bool b) { return !a || b; });
}

/// 第一个 (a)
inline BooleanOp First()
{
    return BooleanOp([](bool a, bool b) {
        (void)b;
        return a;
    });
}

/// 被蕴含 (a || !b)
inline BooleanOp CausedBy()
{
    return BooleanOp([](bool a, bool b) { return a || !b; });
}

/// 或运算 (a || b)
inline BooleanOp Or()
{
    return BooleanOp([](bool a, bool b) { return a || b; });
}

/// 始终返回true
inline BooleanOp True()
{
    return BooleanOp([](bool a, bool b) {
        (void)a;
        (void)b;
        return true;
    });
}

} // namespace BooleanOps

} // namespace mc

#ifdef __APPLE__
#pragma pop_macro("FALSE")
#pragma pop_macro("TRUE")
#endif
