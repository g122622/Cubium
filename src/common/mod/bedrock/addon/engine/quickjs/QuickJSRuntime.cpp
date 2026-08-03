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

#include "common/mod/bedrock/addon/engine/quickjs/QuickJSRuntime.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSContext.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <spdlog/spdlog.h>

// QuickJS 前向声明和头文件
#include <cstddef>
#include <memory>
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

    // 设置内存限制（如果已通过 setMemoryLimit 指定）
    if (m_memoryLimit > 0) {
        JS_SetMemoryLimit(m_runtime, m_memoryLimit);
    }

    spdlog::info("[BedrockAddon] QuickJS runtime created successfully");
    return true;
}

std::unique_ptr<IScriptContext> QuickJSRuntime::createContext(const ContextConfig& config)
{
    if (!m_runtime) {
        spdlog::error("[BedrockAddon] Cannot create context: runtime not initialized");
        return nullptr;
    }

    spdlog::info("[BedrockAddon] Creating QuickJS context (memory={}, stack={})",
        config.maxMemoryBytes,
        config.maxStackSizeBytes);

    // 应用上下文级别的内存限制
    JS_SetMemoryLimit(m_runtime, config.maxMemoryBytes);
    m_memoryLimit = config.maxMemoryBytes;

    // 应用上下文级别的栈大小限制
    JS_SetMaxStackSize(m_runtime, config.maxStackSizeBytes);

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
