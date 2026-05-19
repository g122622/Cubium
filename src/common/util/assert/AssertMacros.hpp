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

#include "Assert.hpp"

// ============================================================================
// 核心断言宏
// ============================================================================

/**
 * @file AssertMacros.hpp
 * @brief 断言宏定义
 *
 * 提供多种断言宏：
 * - MC_ASSERT(cond)              - 基本断言，仅 Debug 模式
 * - MC_ASSERT_MSG(cond, msg)     - 带消息的断言，仅 Debug 模式
 * - MC_ASSERT_RELEASE(cond)      - Release 模式也启用的断言
 * - MC_ASSERT_FATAL(cond)        - 致命断言，始终启用
 * - MC_ASSERT_DEBUG(cond)        - 明确标记为 Debug 级别
 *
 * 特殊断言：
 * - MC_ASSERT_NULL(ptr)          - 检查指针为空
 * - MC_ASSERT_NOT_NULL(ptr)      - 检查指针非空
 * - MC_ASSERT_RANGE(val, min, max) - 检查值在范围内
 * - MC_ASSERT_INDEX(idx, size)   - 检查索引有效
 * - MC_ASSERT_UNREACHABLE()      - 标记不可达代码
 * - MC_ASSERT_FAIL(msg)          - 总是失败
 *
 * 比较断言（带值输出）：
 * - MC_ASSERT_EQ(a, b)           - 相等断言
 * - MC_ASSERT_NE(a, b)           - 不相等断言
 * - MC_ASSERT_LT(a, b)           - 小于断言
 * - MC_ASSERT_LE(a, b)           - 小于等于断言
 * - MC_ASSERT_GT(a, b)           - 大于断言
 * - MC_ASSERT_GE(a, b)           - 大于等于断言
 */

// ============================================================================
// 实现细节
// ============================================================================

#define MC_ASSERT_IMPL(expr, msg, level)                                                                            \
    do {                                                                                                            \
        if (!(expr)) [[unlikely]] {                                                                                 \
            ::mc::assert::AssertManager::instance().handleFailure(#expr, msg, __FILE__, __LINE__, __func__, level); \
        }                                                                                                           \
    } while (false)

#define MC_ASSERT_RECOVERABLE_IMPL(expr, msg)                                 \
    do {                                                                      \
        if (!(expr)) [[unlikely]] {                                           \
            ::mc::assert::AssertManager::instance().handleRecoverableFailure( \
                #expr, msg, __FILE__, __LINE__, __func__);                    \
        }                                                                     \
    } while (false)

// ============================================================================
// 基本断言宏
// ============================================================================

#ifdef NDEBUG

// Release 模式：基本断言被禁用
#define MC_ASSERT(cond) ((void)0)
#define MC_ASSERT_MSG(cond, msg) ((void)0)
#define MC_ASSERT_DEBUG(cond) ((void)0)
#define MC_ASSERT_DEBUG_MSG(cond, msg) ((void)0)

#else

// Debug 模式：启用断言
#define MC_ASSERT(cond) MC_ASSERT_IMPL(cond, nullptr, ::mc::assert::AssertLevel::Debug)
#define MC_ASSERT_MSG(cond, msg) MC_ASSERT_IMPL(cond, msg, ::mc::assert::AssertLevel::Debug)
#define MC_ASSERT_DEBUG(cond) MC_ASSERT_IMPL(cond, nullptr, ::mc::assert::AssertLevel::Debug)
#define MC_ASSERT_DEBUG_MSG(cond, msg) MC_ASSERT_IMPL(cond, msg, ::mc::assert::AssertLevel::Debug)

#endif // NDEBUG

// 始终启用的断言
#define MC_ASSERT_RELEASE(cond) MC_ASSERT_IMPL(cond, nullptr, ::mc::assert::AssertLevel::Release)
#define MC_ASSERT_RELEASE_MSG(cond, msg) MC_ASSERT_IMPL(cond, msg, ::mc::assert::AssertLevel::Release)

// 致命断言（始终启用）
#define MC_ASSERT_FATAL(cond) MC_ASSERT_IMPL(cond, nullptr, ::mc::assert::AssertLevel::Fatal)
#define MC_ASSERT_FATAL_MSG(cond, msg) MC_ASSERT_IMPL(cond, msg, ::mc::assert::AssertLevel::Fatal)

// ============================================================================
// 指针断言
// ============================================================================

#define MC_ASSERT_NULL(ptr) MC_ASSERT((ptr) == nullptr)
#define MC_ASSERT_NOT_NULL(ptr) MC_ASSERT((ptr) != nullptr)
#define MC_ASSERT_NULL_RELEASE(ptr) MC_ASSERT_RELEASE((ptr) == nullptr)
#define MC_ASSERT_NOT_NULL_RELEASE(ptr) MC_ASSERT_RELEASE((ptr) != nullptr)

// ============================================================================
// 范围断言
// ============================================================================

#define MC_ASSERT_RANGE(val, min, max) MC_ASSERT((val) >= (min) && (val) <= (max))
#define MC_ASSERT_RANGE_EXCLUSIVE(val, min, max) MC_ASSERT((val) > (min) && (val) < (max))
#define MC_ASSERT_INDEX(idx, size) MC_ASSERT((idx) >= 0 && (idx) < (size))
#define MC_ASSERT_INDEX_U(idx, size) MC_ASSERT((idx) < (size))

// ============================================================================
// 比较断言（带值输出）
// ============================================================================

#define MC_ASSERT_EQ(a, b)                                                                          \
    do {                                                                                            \
        const auto& _a = (a);                                                                       \
        const auto& _b = (b);                                                                       \
        if (!(_a == _b)) {                                                                          \
            ::mc::assert::AssertManager::instance().handleFailure(#a " == " #b,                     \
                ::mc::assert::detail::formatComparisonMessage("not equal", #a, _a, #b, _b).c_str(), \
                __FILE__,                                                                           \
                __LINE__,                                                                           \
                __func__,                                                                           \
                ::mc::assert::AssertLevel::Debug);                                                  \
        }                                                                                           \
    } while (false)

#define MC_ASSERT_NE(a, b)                                                                                            \
    do {                                                                                                              \
        const auto& _a = (a);                                                                                         \
        const auto& _b = (b);                                                                                         \
        if (_a == _b) {                                                                                               \
            ::mc::assert::AssertManager::instance().handleFailure(#a " != " #b,                                       \
                ::mc::assert::detail::formatComparisonMessage("equal (should be different)", #a, _a, #b, _b).c_str(), \
                __FILE__,                                                                                             \
                __LINE__,                                                                                             \
                __func__,                                                                                             \
                ::mc::assert::AssertLevel::Debug);                                                                    \
        }                                                                                                             \
    } while (false)

#define MC_ASSERT_LT(a, b)                                                                              \
    do {                                                                                                \
        const auto& _a = (a);                                                                           \
        const auto& _b = (b);                                                                           \
        if (!(_a < _b)) {                                                                               \
            ::mc::assert::AssertManager::instance().handleFailure(#a " < " #b,                          \
                ::mc::assert::detail::formatComparisonMessage("not less than", #a, _a, #b, _b).c_str(), \
                __FILE__,                                                                               \
                __LINE__,                                                                               \
                __func__,                                                                               \
                ::mc::assert::AssertLevel::Debug);                                                      \
        }                                                                                               \
    } while (false)

#define MC_ASSERT_LE(a, b)                                                                                       \
    do {                                                                                                         \
        const auto& _a = (a);                                                                                    \
        const auto& _b = (b);                                                                                    \
        if (!(_a <= _b)) {                                                                                       \
            ::mc::assert::AssertManager::instance().handleFailure(#a " <= " #b,                                  \
                ::mc::assert::detail::formatComparisonMessage("not less than or equal", #a, _a, #b, _b).c_str(), \
                __FILE__,                                                                                        \
                __LINE__,                                                                                        \
                __func__,                                                                                        \
                ::mc::assert::AssertLevel::Debug);                                                               \
        }                                                                                                        \
    } while (false)

#define MC_ASSERT_GT(a, b)                                                                                 \
    do {                                                                                                   \
        const auto& _a = (a);                                                                              \
        const auto& _b = (b);                                                                              \
        if (!(_a > _b)) {                                                                                  \
            ::mc::assert::AssertManager::instance().handleFailure(#a " > " #b,                             \
                ::mc::assert::detail::formatComparisonMessage("not greater than", #a, _a, #b, _b).c_str(), \
                __FILE__,                                                                                  \
                __LINE__,                                                                                  \
                __func__,                                                                                  \
                ::mc::assert::AssertLevel::Debug);                                                         \
        }                                                                                                  \
    } while (false)

#define MC_ASSERT_GE(a, b)                                                                                          \
    do {                                                                                                            \
        const auto& _a = (a);                                                                                       \
        const auto& _b = (b);                                                                                       \
        if (!(_a >= _b)) {                                                                                          \
            ::mc::assert::AssertManager::instance().handleFailure(#a " >= " #b,                                     \
                ::mc::assert::detail::formatComparisonMessage("not greater than or equal", #a, _a, #b, _b).c_str(), \
                __FILE__,                                                                                           \
                __LINE__,                                                                                           \
                __func__,                                                                                           \
                ::mc::assert::AssertLevel::Debug);                                                                  \
        }                                                                                                           \
    } while (false)

// ============================================================================
// 特殊断言
// ============================================================================

/**
 * @brief 标记不可达代码路径
 *
 * 如果执行到此断言，表示程序逻辑错误
 */
#define MC_ASSERT_UNREACHABLE()                            \
    ::mc::assert::AssertManager::instance().handleFailure( \
        "unreachable code", nullptr, __FILE__, __LINE__, __func__, ::mc::assert::AssertLevel::Fatal)

/**
 * @brief 带消息的不可达断言
 */
#define MC_ASSERT_UNREACHABLE_MSG(msg)                     \
    ::mc::assert::AssertManager::instance().handleFailure( \
        "unreachable code", msg, __FILE__, __LINE__, __func__, ::mc::assert::AssertLevel::Fatal)

/**
 * @brief 总是失败的断言
 */
#define MC_ASSERT_FAIL(msg)                                \
    ::mc::assert::AssertManager::instance().handleFailure( \
        "assertion failed", msg, __FILE__, __LINE__, __func__, ::mc::assert::AssertLevel::Fatal)

/**
 * @brief 未实现功能断言
 */
#define MC_ASSERT_NOT_IMPLEMENTED()                        \
    ::mc::assert::AssertManager::instance().handleFailure( \
        "not implemented", __func__, __FILE__, __LINE__, __func__, ::mc::assert::AssertLevel::Fatal)

// ============================================================================
// 前置/后置条件断言
// ============================================================================

/**
 * @brief 前置条件断言（函数入口检查）
 */
#define MC_PRECONDITION(cond) MC_ASSERT(cond)
#define MC_PRECONDITION_MSG(cond, msg) MC_ASSERT_MSG(cond, msg)

/**
 * @brief 后置条件断言（函数出口检查）
 */
#define MC_POSTCONDITION(cond) MC_ASSERT(cond)
#define MC_POSTCONDITION_MSG(cond, msg) MC_ASSERT_MSG(cond, msg)

/**
 * @brief 不变量断言（对象状态检查）
 */
#define MC_INVARIANT(cond) MC_ASSERT(cond)
#define MC_INVARIANT_MSG(cond, msg) MC_ASSERT_MSG(cond, msg)

// ============================================================================
// 类型断言
// ============================================================================

/**
 * @brief 断言指针可以安全转换为目标类型
 *
 * 注意：这不能替代 dynamic_cast，仅用于已知类型的情况
 */
#define MC_ASSERT_CAST(ptr, Type) MC_ASSERT_NOT_NULL(ptr)

/**
 * @brief 断言枚举值有效
 */
#define MC_ASSERT_ENUM_VALID(val, min, max)                                  \
    MC_ASSERT_RANGE(static_cast<std::underlying_type_t<decltype(val)>>(val), \
        static_cast<std::underlying_type_t<decltype(val)>>(min),             \
        static_cast<std::underlying_type_t<decltype(val)>>(max))

// ============================================================================
// 调试辅助
// ============================================================================

#ifdef NDEBUG

#define MC_DEBUG_ONLY(expr) ((void)0)

#else

/**
 * @brief 仅在 Debug 模式执行的代码
 */
#define MC_DEBUG_ONLY(expr) expr

#endif // NDEBUG

/**
 * @brief 标记变量为未使用
 */
#define MC_UNUSED(var) ((void)(var))
