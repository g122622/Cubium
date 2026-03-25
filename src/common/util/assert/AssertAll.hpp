#pragma once

/**
 * @file AssertAll.hpp
 * @brief 断言库统一入口
 *
 * 包含断言库的所有组件：
 * - Assert.hpp: 核心断言类和管理器
 * - AssertMacros.hpp: 断言宏定义
 *
 * 使用示例：
 * @code
 * // 基本断言
 * MC_ASSERT(ptr != nullptr);
 * MC_ASSERT_MSG(size > 0, "Size must be positive");
 *
 * // Release 模式断言
 * MC_ASSERT_RELEASE(index < capacity);
 *
 * // 致命断言
 * MC_ASSERT_FATAL(state == State::Ready);
 *
 * // 比较断言
 * MC_ASSERT_EQ(expected, actual);
 * MC_ASSERT_NE(ptr, nullptr);
 * MC_ASSERT_LT(value, max);
 *
 * // 指针断言
 * MC_ASSERT_NOT_NULL(obj);
 * MC_ASSERT_NULL(optional);
 *
 * // 范围断言
 * MC_ASSERT_RANGE(index, 0, size - 1);
 * MC_ASSERT_INDEX(row, height);
 *
 * // 特殊断言
 * MC_ASSERT_UNREACHABLE();
 * MC_ASSERT_FAIL("This should never happen");
 * MC_ASSERT_NOT_IMPLEMENTED();
 *
 * // 前置/后置条件
 * MC_PRECONDITION(size > 0);
 * MC_POSTCONDITION(result != nullptr);
 * MC_INVARIANT(m_count >= 0);
 *
 * // 仅 Debug 模式代码
 * MC_DEBUG_ONLY(debugLog("Checking..."));
 *
 * // 标记未使用变量
 * MC_UNUSED(unusedParam);
 * @endcode
 */

#include "Assert.hpp"
#include "AssertMacros.hpp"
