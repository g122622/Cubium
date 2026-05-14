#pragma once

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include <functional>

namespace mc {

/**
 * @brief 布尔操作接口
 *
 * 用于 VoxelShape 之间的布尔运算（并集、交集、差集等）。
 * 参考MC BooleanOp接口。
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

    BooleanOp()
        : m_func(nullptr)
    {}
    explicit BooleanOp(OpFunc func)
        : m_func(std::move(func))
    {}

    bool apply(bool a, bool b) const { return m_func ? m_func(a, b) : false; }

    explicit operator bool() const { return m_func != nullptr; }

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
