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
 * @file TraceCategories.cpp
 * @brief Perfetto 追踪分类静态存储定义
 *
 * 此文件定义 PERFETTO_TRACK_EVENT_STATIC_STORAGE，必须且只能在一个编译单元中定义。
 * 注意：perfetto.cc 编译为独立的静态库 perfetto_sdk。
 */

#include "TraceCategories.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

// 定义分类的静态存储（必须且只能在一个 .cpp 文件中）
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace mc {
namespace profiler {

void initTraceCategories()
{
    // 分类已在 PERFETTO_DEFINE_CATEGORIES 中静态定义
    // 此函数预留给未来可能的动态分类注册需求
}

} // namespace profiler
} // namespace mc

#endif // MC_ENABLE_TRACING
