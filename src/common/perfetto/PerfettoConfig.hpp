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
 * @file PerfettoConfig.hpp
 * @brief Perfetto 性能追踪编译时配置
 *
 * 此文件定义了 Perfetto 追踪系统的编译时开关和配置选项。
 * 所有追踪功能默认禁用，能且只能通过 CMake 选项启用。
 *
 * 使用方法：
 * 在 CMake 配置时启用：cmake -DMC_ENABLE_TRACING=ON ..
 */

#pragma once

// ============================================================================
// 总开关
// ============================================================================

/**
 * @brief 追踪系统总开关
 *
 * 当禁用时，所有 MC_TRACE_* 宏展开为空操作，无任何性能开销。
 * 不允许手动修改这个宏！应当通过 CMake 选项 MC_ENABLE_TRACING 启用或禁用追踪。
 */
#ifndef MC_ENABLE_TRACING
#define MC_ENABLE_TRACING 1
#endif

// ============================================================================
// 缓冲区配置
// ============================================================================

/**
 * @brief 追踪缓冲区大小 (KB)
 *
 * 默认 65536 KB = 64 MB，可支持约 16-64 秒的高负载追踪。
 * 追踪数据典型速率为 1-4 MB/s。
 */
#ifndef MC_TRACE_BUFFER_SIZE_KB
#define MC_TRACE_BUFFER_SIZE_KB 65536
#endif

// ============================================================================
// 文件输出配置
// ============================================================================

/**
 * @brief 默认追踪输出文件路径
 */
#ifndef MC_TRACE_DEFAULT_OUTPUT
#define MC_TRACE_DEFAULT_OUTPUT "trace.perfetto-trace"
#endif

/**
 * @brief 客户端追踪默认输出路径
 */
#ifndef MC_TRACE_CLIENT_OUTPUT
#define MC_TRACE_CLIENT_OUTPUT "client_trace.perfetto-trace"
#endif

/**
 * @brief 服务端追踪默认输出路径
 */
#ifndef MC_TRACE_SERVER_OUTPUT
#define MC_TRACE_SERVER_OUTPUT "server_trace.perfetto-trace"
#endif

// ============================================================================
// 便捷宏：检查追踪是否启用
// ============================================================================

#if MC_ENABLE_TRACING
#define MC_TRACING_ENABLED 1
#else
#define MC_TRACING_ENABLED 0
#endif
