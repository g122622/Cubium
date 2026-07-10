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
 * @file ProfilerConfig.hpp
 * @brief 性能追踪编译时配置（Perfetto + Tracy 双轨）
 *
 * 此文件定义了双轨 profiler 的编译时开关和配置选项。
 * 两个后端可独立开关、同时启用：
 *   - MC_ENABLE_TRACING : Perfetto 后端（进程内录制到 .perfetto-trace 文件）
 *   - MC_ENABLE_TRACY   : Tracy 后端（in-memory 采集，tracy GUI 连接 8086 查看）
 * 同时启用时，MC_TRACE_* 宏会同时向两套后端发事件（双轨录制）。
 * 两者皆关时，所有宏展开为空操作，无任何性能开销。
 *
 * 不允许手动修改这些宏！应当通过 CMake 选项 MC_ENABLE_TRACING / MC_ENABLE_TRACY 启用或禁用。
 */

#pragma once

// ============================================================================
// Perfetto 后端总开关
// ============================================================================

/**
 * @brief Perfetto 后端开关
 *
 * 当禁用时，Perfetto 侧宏展开为空操作，无任何性能开销。
 * 通过 CMake 选项 MC_ENABLE_TRACING 控制。
 */
#ifndef MC_ENABLE_TRACING
#define MC_ENABLE_TRACING 1
#endif

// ============================================================================
// Tracy 后端总开关
// ============================================================================

/**
 * @brief Tracy 后端开关
 *
 * 当禁用时，Tracy 侧宏展开为空操作，无任何性能开销。
 * 通过 CMake 选项 MC_ENABLE_TRACY 控制。
 *
 * Tracy 走 in-memory 采集：client 自动监听 8086 端口，需用 tracy GUI 或
 * tracy-capture 工具连接拉取数据，不支持进程内直接写文件。
 */
#ifndef MC_ENABLE_TRACY
#define MC_ENABLE_TRACY 0
#endif

// ============================================================================
// 缓冲区配置（仅 Perfetto 用）
// ============================================================================

/**
 * @brief Perfetto 追踪缓冲区大小 (KB)
 *
 * 默认 65536 KB = 64 MB，可支持约 16-64 秒的高负载追踪。
 * 追踪数据典型速率为 1-4 MB/s。
 */
#ifndef MC_TRACE_BUFFER_SIZE_KB
#define MC_TRACE_BUFFER_SIZE_KB 65536
#endif

// ============================================================================
// 文件输出配置（仅 Perfetto 用）
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
// 便捷宏：检查后端是否启用
// ============================================================================

#if MC_ENABLE_TRACING
#define MC_TRACING_ENABLED 1
#else
#define MC_TRACING_ENABLED 0
#endif

#if MC_ENABLE_TRACY
#define MC_TRACY_ENABLED 1
#else
#define MC_TRACY_ENABLED 0
#endif

// 至少一个后端启用时，profiler 系统整体有效
#define MC_PROFILER_ENABLED (MC_TRACING_ENABLED || MC_TRACY_ENABLED)
