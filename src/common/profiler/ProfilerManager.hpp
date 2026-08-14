/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 * @file ProfilerManager.hpp
 * @brief 性能追踪门面（Perfetto + Tracy 双轨）
 *
 * 单例门面，统一管理两套 profiler 后端的生命周期与进程/线程命名：
 * - Perfetto 后端（PerfettoBackend）：TracingSession 生命周期、root track、
 *   写 .perfetto-trace 文件，仅在 MC_ENABLE_TRACING=1 时存在。
 * - Tracy 后端：in-memory 采集（client 自动监听 8086，需 tracy GUI 连接拉取），
 *   不写文件、不做 start/stop/capture 管理，仅由本门面双写进程/线程名。
 *
 * setProcessName / setThreadName 永远同时写给两套后端（双轨命名）。
 * 其余生命周期方法（initialize/start/stop/flush）仅作用于 Perfetto 后端——
 * Tracy 的采集是 client 自动完成的，无需门面驱动。
 *
 * 使用方法：
 * @code
 * // 应用启动时初始化（配置仅 Perfetto 用）
 * mc::profiler::TraceConfig config;
 * config.outputPath = "trace.perfetto-trace";
 * mc::profiler::ProfilerManager::instance().initialize(config);
 * mc::profiler::ProfilerManager::instance().startTracing();
 *
 * // 应用运行中记录事件（双轨宏，见 TraceEvents.hpp）
 * MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Frame");
 *
 * // 应用关闭时清理
 * mc::profiler::ProfilerManager::instance().stopTracing();
 * mc::profiler::ProfilerManager::instance().shutdown();
 * @endcode
 *
 * 注意事项：
 * - 追踪相关生命周期方法仅在 Perfetto 后端启用时有效；Tracy 单独启用时
 *   这些方法为空操作，但 setProcessName/setThreadName 仍会写给 tracy。
 * - 两个后端都关闭时（MC_PROFILER_ENABLED=0），本类展开为空操作存根。
 */

#pragma once

#include "../core/Types.hpp"
#include "ProfilerConfig.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mc {
namespace profiler {

/**
 * @brief 追踪配置选项
 *
 * 这些字段当前仅被 Perfetto 后端使用（缓冲区、文件输出、分类过滤）。
 * Tracy 后端走 in-memory 采集，不消费此配置。
 */
struct TraceConfig {
    /** 是否启用追踪（运行时开关） */
    bool enabled = true;

    /** 是否输出到文件 */
    bool outputToFile = true;

    /** 输出文件路径 */
    std::string outputPath = MC_TRACE_DEFAULT_OUTPUT;

    /** 缓冲区大小 (KB) */
    u64 bufferSizeKb = MC_TRACE_BUFFER_SIZE_KB;

    /** 是否记录进程元数据 */
    bool recordProcessMetadata = true;

    /** 是否记录线程名称 */
    bool recordThreadNames = true;

    /**
     * @brief 要启用的分类列表
     *
     * 空列表表示启用所有分类。
     * 使用 "*" 作为通配符匹配所有分类。
     */
    std::vector<std::string> enabledCategories;

    /**
     * @brief 要禁用的分类列表
     *
     * 优先级高于 enabledCategories。
     */
    std::vector<std::string> disabledCategories;
};

// 前置声明 PerfettoBackend（仅在 MC_ENABLE_TRACING 时有完整定义），避免循环 include
class PerfettoBackend;

#if MC_PROFILER_ENABLED

/**
 * @brief 性能追踪门面（启用时的实现）
 *
 * 单例模式，持有 PerfettoBackend（若启用），并内联 Tracy 的命名双写逻辑。
 */
class ProfilerManager {
public:
    /**
     * @brief 获取单例实例
     *
     * @return ProfilerManager& 单例引用
     */
    static ProfilerManager& instance();

    /**
     * @brief 初始化追踪系统
     *
     * 必须在任何追踪事件之前调用。配置仅作用于 Perfetto 后端。
     *
     * @param config 配置选项
     * @throws std::runtime_error 如果初始化失败
     */
    void initialize(const TraceConfig& config = {});

    /**
     * @brief 关闭追踪系统
     *
     * 刷新所有待写入数据并释放资源。
     * 必须在程序退出前调用以确保数据完整性。
     */
    void shutdown();

    /**
     * @brief 启动追踪会话
     *
     * 开始记录追踪事件（仅 Perfetto 后端）。Tracy 自动采集，无需调用。
     */
    void startTracing();

    /**
     * @brief 停止追踪会话
     *
     * 停止记录追踪事件，但保持系统初始化状态。
     * 可以通过 startTracing() 重新启动。
     */
    void stopTracing();

    /**
     * @brief 刷新追踪数据
     *
     * 将缓冲区中的数据写入文件。
     * 通常在关键时刻手动调用，如场景切换。
     */
    void flush();

    /**
     * @brief 检查追踪是否已启用
     *
     * @return true 如果 Perfetto 后端已初始化且正在记录
     */
    [[nodiscard]] bool isEnabled() const noexcept;

    /**
     * @brief 检查追踪系统是否已初始化
     *
     * @return true 如果已调用 initialize() 且后端就绪
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /**
     * @brief 运行时启用/禁用追踪
     *
     * 可以在不停止追踪会话的情况下暂停/恢复事件记录。
     *
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }

    /**
     * @brief 获取当前配置
     *
     * @return const TraceConfig& 配置的常量引用
     */
    [[nodiscard]] const TraceConfig& config() const noexcept { return m_config; }

    /**
     * @brief 注册进程内存采样回调
     *
     * ProfilerManager 自身处于比 PlatformInfo 更底层的 mc_profiler 库，不能直接依赖
     * mc_common 的 PlatformInfo。故内存采样通过回调注入：调用方（client/server 初始化处，
     * 能访问 PlatformInfo）注入一个返回 {工作集MB, 提交量MB} 的 lambda，ProfilerManager
     * 的内存采样线程在 startTracing() 后周期性调用它并写入 TraceEvents.Memory.Usage 计数器。
     *
     * 必须在 startTracing() 之前调用：startTracing() 时若回调为空则不起采样线程。
     * 注入的实现须线程安全且轻量（每 10ms 调一次），避免阻塞采样线程。
     *
     * @param sampler 返回 {工作集MB, 提交量MB} 的回调；传空则停用内存采样
     */
    void setMemorySampler(std::function<std::pair<i64, i64>()> sampler) { m_memorySampler = std::move(sampler); }

    /**
     * @brief 设置当前进程名称（双轨）
     *
     * 同时写入 Perfetto 与 Tracy，便于两套工具分析。应在进程启动后调用。
     *
     * @param name 进程名称
     */
    void setProcessName(const std::string& name);

    /**
     * @brief 设置当前线程名称（双轨）
     *
     * 同时写入 Perfetto（带 sibling_order_rank 查表）与 Tracy。
     * 应在线程启动后尽早调用。
     *
     * @param name 线程名称
     */
    void setThreadName(const std::string& name);

    /**
     * @brief 设置当前线程名称和排序 rank
     *
     * Perfetto 侧按 PR #6219：根 track uuid=0 设 thread_ordering=EXPLICIT 后，
     * 线程按 sibling_order_rank 升序排列，值越小越靠前。
     * Tracy 侧仅记录线程名（不参与排序）。
     *
     * @param name 线程名称
     * @param siblingOrderRank 排序 rank（值越小越靠前，未设默认 0）
     */
    void setThreadName(const std::string& name, int siblingOrderRank);

private:
    ProfilerManager();
    ~ProfilerManager();

    ProfilerManager(const ProfilerManager&) = delete;
    ProfilerManager& operator=(const ProfilerManager&) = delete;

    TraceConfig m_config;
    bool m_initialized = false;
    bool m_enabled = true;
    bool m_tracing = false;

    /**
     * @brief 内存采样后台线程（随 startTracing 起、stopTracing/shutdown 停）。
     *
     * 定期调用 m_memorySampler 采样进程内存（工作集 + 提交量）写入
     * TraceEvents.Memory.Usage 计数器，供 Perfetto/Tracy 分析。仅 MC_PROFILER_ENABLED
     * 时存在；profiler 全关时本类展开为存根，无此线程。
     */
    std::thread m_memoryThread;
    std::atomic<bool> m_memoryStop{false};

    /// 进程内存采样回调（返回 {工作集MB, 提交量MB}），由上层注入；为空时不起采样线程。
    std::function<std::pair<i64, i64>()> m_memorySampler;

    /// 内存采样线程主循环（_runMemoryTrace）。
    void _runMemoryTrace();

#if MC_ENABLE_TRACING
    /** @brief Perfetto 后端（仅 MC_ENABLE_TRACING 时持有实例） */
    std::unique_ptr<PerfettoBackend> m_perfetto;
#endif
};

#else // MC_PROFILER_ENABLED == 0

/**
 * @brief 性能追踪门面（两套后端都关闭时的存根实现）
 *
 * 所有方法展开为空操作，无任何开销。
 */
class ProfilerManager {
public:
    static ProfilerManager& instance() noexcept
    {
        static ProfilerManager instance;
        return instance;
    }

    void initialize(const TraceConfig& = {}) noexcept {}
    void shutdown() noexcept {}
    void startTracing() noexcept {}
    void stopTracing() noexcept {}
    void flush() noexcept {}

    [[nodiscard]] bool isEnabled() const noexcept { return false; }
    [[nodiscard]] bool isInitialized() const noexcept { return false; }
    void setEnabled(bool) noexcept {}

    [[nodiscard]] TraceConfig config() const noexcept { return {}; }
    void setProcessName(const std::string&) noexcept {}
    void setThreadName(const std::string&) noexcept {}
    void setThreadName(const std::string&, int) noexcept {}
    void setMemorySampler(std::function<std::pair<i64, i64>()>) noexcept {}

private:
    ProfilerManager() = default;
    ~ProfilerManager() = default;

    ProfilerManager(const ProfilerManager&) = delete;
    ProfilerManager& operator=(const ProfilerManager&) = delete;
};

#endif // MC_PROFILER_ENABLED

} // namespace profiler
} // namespace mc
