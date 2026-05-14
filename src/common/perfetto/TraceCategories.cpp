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
namespace perfetto {

void initTraceCategories()
{
    // 分类已在 PERFETTO_DEFINE_CATEGORIES 中静态定义
    // 此函数预留给未来可能的动态分类注册需求
}

} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
