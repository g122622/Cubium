/**
 * @file TraceCategories.hpp
 * @brief Perfetto 追踪分类定义
 *
 * 此文件定义 Perfetto 追踪分类。根据 Perfetto SDK 要求：
 * - PERFETTO_DEFINE_CATEGORIES 放在头文件中
 * - PERFETTO_TRACK_EVENT_STATIC_STORAGE() 放在一个 .cpp 文件中 (TraceCategories.cpp)
 *
 * 所有使用 MC_TRACE_EVENT 宏的文件都必须包含此头文件。
 */

#pragma once

#include "PerfettoConfig.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

// 定义追踪分类
PERFETTO_DEFINE_CATEGORIES(
    // === 客户端启动 ===
    perfetto::Category("client.initialization")
        .SetDescription("客户端初始化过程"),

    // === 渲染分类 ===
    perfetto::Category("rendering.initialization")
        .SetDescription("渲染系统初始化"),
    perfetto::Category("rendering.frame")
        .SetDescription("帧渲染生命周期事件"),
    perfetto::Category("rendering.vulkan")
        .SetDescription("Vulkan API 调用"),
    perfetto::Category("rendering.chunk_mesh")
        .SetDescription("区块网格生成和上传"),
    perfetto::Category("rendering.entity")
        .SetDescription("实体渲染"),

    // === 渲染子分类（细粒度分析）===
    perfetto::Category("rendering.begin_frame")
        .SetDescription("帧开始阶段（等待同步、获取图像）"),
    perfetto::Category("rendering.uniform_update")
        .SetDescription("Uniform缓冲区更新"),
    perfetto::Category("rendering.sky")
        .SetDescription("天空渲染（穹顶、太阳、月亮、星星）"),
    perfetto::Category("rendering.chunk_draw")
        .SetDescription("区块绘制（绑定管线、描述符、绘制调用）"),
    perfetto::Category("rendering.gui")
        .SetDescription("GUI渲染（HUD、聊天、菜单）"),
    perfetto::Category("rendering.end_frame")
        .SetDescription("帧结束阶段（提交命令、呈现）"),
    perfetto::Category("rendering.viewport")
        .SetDescription("视口和裁剪设置"),
    perfetto::Category("rendering.descriptor_bind")
        .SetDescription("描述符集绑定"),
    perfetto::Category("rendering.push_constants")
        .SetDescription("推送常量更新"),
    perfetto::Category("rendering.command_buffer")
        .SetDescription("命令缓冲区操作"),
    perfetto::Category("rendering.weather")
        .SetDescription("天气渲染（雨、雪）"),
    perfetto::Category("rendering.cloud")
        .SetDescription("云层渲染"),

    // === 游戏逻辑分类 ===
    perfetto::Category("game.tick")
        .SetDescription("游戏刻处理"),
    perfetto::Category("game.entity")
        .SetDescription("实体更新"),
    perfetto::Category("game.physics")
        .SetDescription("物理模拟"),
    perfetto::Category("game.ai")
        .SetDescription("AI 目标处理"),

    // === 世界分类 ===
    perfetto::Category("world.chunk")
        .SetDescription("区块操作"),
    perfetto::Category("world.chunk_gen")
        .SetDescription("区块生成各阶段"),
    perfetto::Category("world.chunk_load")
        .SetDescription("区块加载和卸载"),
    perfetto::Category("world.biome")
        .SetDescription("生物群系生成"),
    perfetto::Category("world.gen.nether")
        .SetDescription("下界区块生成"),
    perfetto::Category("world.gen.end")
        .SetDescription("末地区块生成"),

    // === 网络分类 ===
    perfetto::Category("network.packet")
        .SetDescription("网络包处理"),
    perfetto::Category("network.sync")
        .SetDescription("状态同步"),
    perfetto::Category("network.connection")
        .SetDescription("连接管理"),

    // === I/O 分类 ===
    perfetto::Category("io.file")
        .SetDescription("文件 I/O 操作"),
    perfetto::Category("io.resource")
        .SetDescription("资源加载"),

    // === 内存分类 ===
    perfetto::Category("memory.allocation")
        .SetDescription("内存分配追踪"),
    perfetto::Category("memory.cache")
        .SetDescription("缓存操作"),

    // === 服务端细粒度分类 ===
    perfetto::Category("server.initialization")
        .SetDescription("服务端初始化"),
    perfetto::Category("server.tick")
        .SetDescription("服务端游戏刻处理"),
    perfetto::Category("server.network")
        .SetDescription("服务端网络处理"),
    perfetto::Category("server.player")
        .SetDescription("服务端玩家管理"),
    perfetto::Category("server.world")
        .SetDescription("服务端世界更新"),
    perfetto::Category("server.chunk")
        .SetDescription("服务端区块处理"),
    perfetto::Category("server.entity")
        .SetDescription("服务端实体更新"),
    perfetto::Category("server.lighting")
        .SetDescription("服务端光照处理"),
    perfetto::Category("client.lighting")
        .SetDescription("客户端光照处理"),
    perfetto::Category("client.resource")
        .SetDescription("客户端资源包"),

    // === 客户端细粒度分类 ===
    perfetto::Category("client.entity")
        .SetDescription("客户端实体更新"),
    perfetto::Category("client.network")
        .SetDescription("客户端网络"),

    // === 挖掘 ===
    perfetto::Category("client.input.mining")
        .SetDescription("挖掘输入事件处理"),
    perfetto::Category("server.world.mining")
        .SetDescription("进一步的挖掘处理"),

    // === 声音系统 ===
    perfetto::Category("server.sound")
        .SetDescription("服务端声音事件处理"),
    perfetto::Category("client.sound")
        .SetDescription("客户端声音播放"),

    // === 流体 ===
    perfetto::Category("fluid.tick")
        .SetDescription("流体更新处理"),
    
    // === memory ===
    perfetto::Category("memory")
        .SetDescription("内存使用统计")
);

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace mc {
namespace perfetto {

/**
 * @brief 初始化追踪分类
 *
 * 必须在程序启动时调用，通常由 PerfettoManager::initialize() 自动调用。
 */
void initTraceCategories();

} // namespace perfetto
} // namespace mc

#else // MC_ENABLE_TRACING == 0

// 禁用追踪时的空声明

namespace mc {
namespace perfetto {

inline void initTraceCategories() {}

} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
