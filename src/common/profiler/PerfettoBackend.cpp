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
 * @file PerfettoBackend.cpp
 * @brief Perfetto 后端实现
 *
 * 封装 Perfetto SDK 的全部交互：TracingSession 生命周期、root track
 * descriptor（uuid=0，启用显式线程排序）、写文件、进程/线程命名。
 * 由 ProfilerManager 门面持有，仅在 MC_ENABLE_TRACING=1 时编译。
 */

#include "PerfettoBackend.hpp"

#if MC_ENABLE_TRACING

#include "TraceCategories.hpp"

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <fstream>
#include <stdexcept>
#include <string_view>

#include <spdlog/spdlog.h>

namespace mc {
namespace profiler {

namespace {
/**
 * @brief 获取线程的 sibling_order_rank
 *
 * 返回指定线程名称的排序 rank，用于在 Perfetto UI 中固定线程显示顺序。
 * 值越小越靠前；新机制未设默认 0（排最前），故命名线程显式给 1-100，
 * 未命名线程因不调用 setThreadName 不会拿到默认 0，避免意外排前。
 *
 * 固定顺序：
 * 1. MemoryTrace (ProcessMemory 计数器所在线程)
 * 2. ClientMainThread (FPS 计数器所在线程)
 * 3. IntegratedServerThread (ServerTickTime 计数器所在线程)
 * 4. AudioEngineWorker
 * 5. ServerMainThread (独立服务器)
 * 其他线程使用默认值 100，显示在固定线程之后
 */
[[nodiscard]] constexpr int getThreadSortIndex(std::string_view name) noexcept
{
    if (name == "MemoryTrace") return 1;
    if (name == "ClientMainThread") return 2;
    if (name == "IntegratedServerThread") return 3;
    if (name == "AudioEngineWorker") return 4;
    if (name == "ServerMainThread") return 5;
    return 100;
}
} // namespace

/**
 * @brief PerfettoBackend 的实现细节
 *
 * 使用 Pimpl 模式隐藏 Perfetto 特定类型。
 */
class PerfettoBackend::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    std::unique_ptr<::perfetto::TracingSession> tracingSession;
};

PerfettoBackend::PerfettoBackend()
    : m_impl(std::make_unique<Impl>())
{}

PerfettoBackend::~PerfettoBackend()
{
    if (m_initialized && m_tracing) {
        stopTracing();
    }
    if (m_initialized) {
        shutdown();
    }
}

void PerfettoBackend::initialize(const TraceConfig& config)
{
    if (m_initialized) {
        spdlog::warn("[Perfetto] Already initialized, skipping");
        return;
    }

    m_config = config;

    // 初始化 Perfetto
    ::perfetto::TracingInitArgs args;
    args.backends = ::perfetto::kInProcessBackend;
    ::perfetto::Tracing::Initialize(args);
    ::perfetto::TrackEvent::Register();

    m_initialized = true;

    spdlog::info("[Perfetto] Initialized with buffer size {} KB", m_config.bufferSizeKb);
    spdlog::info("[Perfetto] Output file: {}", m_config.outputPath);
}

void PerfettoBackend::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (m_tracing) {
        stopTracing();
    }

    m_initialized = false;
    spdlog::info("[Perfetto] Shutdown complete");
}

void PerfettoBackend::startTracing()
{
    if (!m_initialized) {
        spdlog::error("[Perfetto] Cannot start tracing: not initialized");
        return;
    }

    if (m_tracing) {
        spdlog::warn("[Perfetto] Tracing already started");
        return;
    }

    // 配置追踪会话
    ::perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(static_cast<uint32_t>(m_config.bufferSizeKb));

    // 配置数据源
    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    // 配置分类过滤
    ::perfetto::protos::gen::TrackEventConfig te_cfg;

    // 如果指定了启用的分类
    if (!m_config.enabledCategories.empty()) {
        for (const auto& cat : m_config.enabledCategories) {
            te_cfg.add_enabled_categories(cat);
        }
    }

    // 如果指定了禁用的分类
    if (!m_config.disabledCategories.empty()) {
        for (const auto& cat : m_config.disabledCategories) {
            te_cfg.add_disabled_categories(cat);
        }
    }

    ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString());

    // 创建并启动追踪会话（不使用 Perfetto 内置文件写入，改为手动写入）
    m_impl->tracingSession = ::perfetto::Tracing::NewTrace();
    m_impl->tracingSession->Setup(cfg);
    m_impl->tracingSession->StartBlocking();

    m_tracing = true;

    // 建立/刷新根 track descriptor（uuid=0），启用显式线程排序。
    // thread_ordering=EXPLICIT 告知 trace processor 按 sibling_order_rank 排序线程。
    // 必须在 session 启动后、第一个线程事件/setThreadName 之前发出。
    //
    // 关键：不能只用 TrackEvent::SetTrackDescriptor——它对“从未发过事件的 track”会
    // defer（见 SDK track_event_data_source.h SetTrackDescriptor 的 seen_tracks 检查），
    // uuid=0 根 track 永远不会发事件，故其 descriptor 永不落盘，trace processor 也就
    // 读不到 thread_ordering，排序失效。这里改用 TrackEvent::Trace + TraceContext::
    // NewTracePacket 直接写一个 track_descriptor packet 进 buffer，绕过 defer。
    // 注意：必须用 Track::Global(0) 而非 Track(0)——后者会与 per-process cookie 异或得非 0 uuid。
    {
        ::perfetto::Track rootTrack = ::perfetto::Track::Global(0);
        ::perfetto::TrackEvent::Trace([rootTrack](::perfetto::TrackEvent::TraceContext ctx) {
            auto packet = ctx.NewTracePacket();
            auto* td = packet->set_track_descriptor();
            rootTrack.Serialize(td); // 写 uuid=0、parent_uuid=0
            td->set_thread_ordering(::perfetto::protos::pbzero::TrackDescriptor::THREAD_ORDERING_EXPLICIT);
        });
    }

    spdlog::info("[Perfetto] Tracing started, buffer size: {} KB", m_config.bufferSizeKb);
    spdlog::info("[Perfetto] Output will be written to: {}", m_config.outputPath);
}

void PerfettoBackend::stopTracing()
{
    if (!m_initialized || !m_tracing) {
        return;
    }

    spdlog::info("[Perfetto] Stopping tracing and writing to file...");

    // 刷新 TrackEvent 数据源
    ::perfetto::TrackEvent::Flush();

    // 停止追踪会话
    if (m_impl->tracingSession) {
        m_impl->tracingSession->StopBlocking();

        // 手动读取追踪数据并写入文件
        std::vector<char> trace_data = m_impl->tracingSession->ReadTraceBlocking();
        spdlog::info("[Perfetto] Read {} bytes of trace data", trace_data.size());

        if (!trace_data.empty() && !m_config.outputPath.empty()) {
            std::ofstream output(m_config.outputPath, std::ios::binary);
            if (output.is_open()) {
                output.write(trace_data.data(), trace_data.size());
                output.close();
                spdlog::info("[Perfetto] Trace written to: {} ({} bytes)", m_config.outputPath, trace_data.size());
            } else {
                spdlog::error("[Perfetto] Failed to open output file: {}", m_config.outputPath);
            }
        } else if (trace_data.empty()) {
            spdlog::warn("[Perfetto] No trace data captured");
        }

        m_impl->tracingSession.reset();
    }

    m_tracing = false;
    spdlog::info("[Perfetto] Tracing stopped");
}

void PerfettoBackend::flush()
{
    if (!m_initialized || !m_tracing) {
        return;
    }

    ::perfetto::TrackEvent::Flush();
}

void PerfettoBackend::setProcessName(const std::string& name)
{
    if (!m_initialized) {
        return;
    }

    auto desc = ::perfetto::ProcessTrack::Current().Serialize();
    desc.mutable_process()->set_process_name(name);
    ::perfetto::TrackEvent::SetTrackDescriptor(::perfetto::ProcessTrack::Current(), desc);
}

void PerfettoBackend::setThreadName(const std::string& name)
{
    if (!m_initialized) {
        return;
    }

    setThreadName(name, getThreadSortIndex(name));
}

void PerfettoBackend::setThreadName(const std::string& name, int siblingOrderRank)
{
    if (!m_initialized) {
        return;
    }

    auto desc = ::perfetto::ThreadTrack::Current().Serialize();
    desc.mutable_thread()->set_thread_name(name);
    desc.set_sibling_order_rank(siblingOrderRank);
    ::perfetto::TrackEvent::SetTrackDescriptor(::perfetto::ThreadTrack::Current(), desc);
}

} // namespace profiler
} // namespace mc

#endif // MC_ENABLE_TRACING
