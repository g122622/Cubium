#include "common/mod/bedrock/addon/engine/QuickJSRuntime.hpp"
#include "common/mod/bedrock/addon/engine/QuickJSContext.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <spdlog/spdlog.h>

// QuickJS 前向声明和头文件
#include <quickjs.h>

namespace mc::mod::bedrock::addon {

QuickJSRuntime::QuickJSRuntime() = default;

QuickJSRuntime::~QuickJSRuntime()
{
    if (m_runtime) {
        // QuickJS运行时会在JS_FreeRuntime时自动释放所有上下文
        JS_FreeRuntime(m_runtime);
        m_runtime = nullptr;
        spdlog::info("[BedrockAddon] QuickJS runtime destroyed");
    }
}

bool QuickJSRuntime::initialize()
{
    spdlog::info("[BedrockAddon] Creating QuickJS runtime");

    m_runtime = JS_NewRuntime();
    if (!m_runtime) {
        spdlog::error("[BedrockAddon] Failed to create QuickJS runtime");
        return false;
    }

    // 设置内存限制（默认64MB）
    if (m_memoryLimit > 0) {
        JS_SetMemoryLimit(m_runtime, m_memoryLimit);
    }

    // 设置最大栈大小（4MB）
    JS_SetMaxStackSize(m_runtime, 4 * 1024 * 1024);

    spdlog::info("[BedrockAddon] QuickJS runtime created successfully");
    return true;
}

std::unique_ptr<IScriptContext> QuickJSRuntime::createContext(const ContextConfig& config)
{
    if (!m_runtime) {
        spdlog::error("[BedrockAddon] Cannot create context: runtime not initialized");
        return nullptr;
    }

    spdlog::info("[BedrockAddon] Creating QuickJS context");

    auto context = std::make_unique<QuickJSContext>(*this, config);
    if (!context->initialize()) {
        spdlog::error("[BedrockAddon] Failed to initialize QuickJS context");
        return nullptr;
    }

    m_contextCount++;
    return context;
}

void QuickJSRuntime::destroyContext(IScriptContext* context)
{
    if (!context) {
        return;
    }

    spdlog::info("[BedrockAddon] Destroying QuickJS context");
    delete context;
    if (m_contextCount > 0) {
        m_contextCount--;
    }
}

void QuickJSRuntime::executePendingJobs()
{
    if (!m_runtime) {
        return;
    }

    // 执行QuickJS事件循环中的待处理任务
    // JS_ExecutePendingJob 返回0表示没有待处理任务，1表示执行了任务，-1表示异常
    JSContext* pendingJobContext = nullptr;
    int ret;
    do {
        // QuickJS会无条件写入pctx，因此这里必须传入有效指针。
        ret = JS_ExecutePendingJob(m_runtime, &pendingJobContext);
        if (ret != 0) {
            MC_ASSERT_RELEASE(pendingJobContext != nullptr);
        }
        if (ret < 0) {
            spdlog::error("[BedrockAddon] QuickJS pending job execution failed");
        }
    } while (ret == 1);
}

bool QuickJSRuntime::hasPendingJobs() const
{
    if (!m_runtime) {
        return false;
    }
    return JS_IsJobPending(m_runtime);
}

RuntimeStats QuickJSRuntime::computeStats() const
{
    RuntimeStats stats;
    if (m_runtime) {
        JSMemoryUsage memUsage;
        JS_ComputeMemoryUsage(m_runtime, &memUsage);
        stats.heapSize = memUsage.memory_used_size;
        stats.heapLimit = m_memoryLimit;
        stats.contextCount = m_contextCount;
        stats.pendingJobsCount = hasPendingJobs() ? 1 : 0;
    }
    return stats;
}

void QuickJSRuntime::setMemoryLimit(u64 limitBytes)
{
    m_memoryLimit = limitBytes;
    if (m_runtime) {
        JS_SetMemoryLimit(m_runtime, static_cast<size_t>(limitBytes));
    }
}

} // namespace mc::mod::bedrock::addon
