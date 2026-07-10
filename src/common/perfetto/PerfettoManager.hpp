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
 * @file PerfettoManager.hpp
 * @brief Perfetto 追踪管理器
 *
 * 单例管理器，负责 Perfetto 追踪系统的生命周期管理：
 * - 初始化和关闭追踪系统
 * - 启动和停止追踪会话
 * - 切换追踪数据到文件
 * - 运行时启用/禁用追踪
 *
 * 使用方法：
 * @code
 * // 应用启动时初始化
 * mc::perfetto::TraceConfig config;
 * config.outputPath = "trace.perfetto-trace";
 * mc::perfetto::PerfettoManager::instance().initialize(config);
 * mc::perfetto::PerfettoManager::instance().startTracing();
 *
 * // 应用运行中记录事件
 * MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Frame");
 *
 * // 应用关闭时清理
 * mc::perfetto::PerfettoManager::instance().stopTracing();
 * mc::perfetto::PerfettoManager::instance().shutdown();
 * @endcode
 *
 * 注意事项：
 * - 必须在使用任何 MC_TRACE_* 宏之前调用 initialize()
 * - 必须在程序退出前调用 shutdown() 确保数据写入文件
 * - 追踪禁用时（MC_ENABLE_TRACING=0），此类展开为空操作
 */

#pragma once

#include "../core/Types.hpp"
#include "PerfettoConfig.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {
namespace perfetto {

/**
 * @brief 追踪配置选项
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

#if MC_ENABLE_TRACING

/**
 * @brief Perfetto 追踪管理器（启用追踪时的实现）
 *
 * 单例模式，管理追踪系统的完整生命周期。
 */
class PerfettoManager {
public:
    /**
     * @brief 获取单例实例
     *
     * @return PerfettoManager& 单例引用
     */
    static PerfettoManager& instance();

    /**
     * @brief 初始化追踪系统
     *
     * 必须在任何追踪事件之前调用。
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
     * 开始记录追踪事件。
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
     * @return true 如果追踪系统已初始化且正在记录
     */
    [[nodiscard]] bool isEnabled() const noexcept;

    /**
     * @brief 检查追踪系统是否已初始化
     *
     * @return true 如果已调用 initialize()
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
     * @brief 设置当前进程名称
     *
     * 在 Perfetto UI 中显示有意义的进程名称，便于分析。
     * 应在进程启动后、追踪开始前调用。
     *
     * @param name 进程名称
     */
    void setProcessName(const std::string& name);

    /**
     * @brief 设置当前线程名称
     *
     * 在 Perfetto UI 中显示有意义的线程名称，便于分析。
     * 应在线程启动后尽早调用。
     *
     * @param name 线程名称
     */
    void setThreadName(const std::string& name);

    /**
     * @brief 设置当前线程名称和排序 rank
     *
     * 在 Perfetto UI 中显示有意义的线程名称，并控制显示顺序。
     * 基于 PR #6219：根 track uuid=0 设 thread_ordering=EXPLICIT 后，
     * 线程按 sibling_order_rank 升序排列，值越小越靠前。
     * 未设 rank 默认 0（排最前），故命名线程应显式给 1-100 避免意外排前。
     *
     * @param name 线程名称
     * @param siblingOrderRank 排序 rank（值越小越靠前，未设默认 0）
     */
    void setThreadName(const std::string& name, int siblingOrderRank);

private:
    PerfettoManager();
    ~PerfettoManager();

    PerfettoManager(const PerfettoManager&) = delete;
    PerfettoManager& operator=(const PerfettoManager&) = delete;

    TraceConfig m_config;
    bool m_initialized = false;
    bool m_enabled = true;
    bool m_tracing = false;

    /** Pimpl 模式隐藏实现细节 */
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#else // MC_ENABLE_TRACING == 0

/**
 * @brief Perfetto 追踪管理器（禁用追踪时的存根实现）
 *
 * 所有方法展开为空操作，无任何开销。
 */
class PerfettoManager {
public:
    static PerfettoManager& instance() noexcept
    {
        static PerfettoManager instance;
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

private:
    PerfettoManager() = default;
    ~PerfettoManager() = default;

    PerfettoManager(const PerfettoManager&) = delete;
    PerfettoManager& operator=(const PerfettoManager&) = delete;
};

#endif // MC_ENABLE_TRACING

} // namespace perfetto
} // namespace mc
