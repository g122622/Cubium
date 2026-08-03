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
 * AUTHORS OR COPYRIGHT HAVING BEEN CLAIMED FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/mod/bedrock/addon/lifecycle/ScriptWatchdog.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"

#include <chrono>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptWatchdog::ScriptWatchdog(Config config)
    : m_config(std::move(config))
{}
void ScriptWatchdog::tick(ScriptManager& manager)
{
    if (!m_config.enabled) {
        return;
    }

    ++m_totalTickCount;

    // 检查执行时间
    if (checkExecutionTime()) {
        ++m_timeoutCount;
        spdlog::warn("[BedrockAddon] Script watchdog: tick time exceeded limit ({}ms > {}ms)",
            m_lastTickDurationMs,
            m_config.tickTimeLimitMs);
        // TODO: 在完整实现中，可以终止超时的脚本上下文
    }

    // 检查内存限制
    if (checkMemoryLimit(manager)) {
        ++m_oomCount;
        spdlog::warn("[BedrockAddon] Script watchdog: memory limit exceeded");
    }
}

bool ScriptWatchdog::checkMemoryLimit(ScriptManager& manager) const
{
    // ScriptManager提供运行时统计信息
    // TODO: 完整实现需要从ScriptManager获取运行时统计
    (void)manager;
    return false;
}

bool ScriptWatchdog::checkExecutionTime() const noexcept
{
    return m_lastTickDurationMs > m_config.tickTimeLimitMs;
}

void ScriptWatchdog::beginTick() noexcept
{
    m_tickStartTime = std::chrono::steady_clock::now();
}

void ScriptWatchdog::endTick() noexcept
{
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_tickStartTime);
    m_lastTickDurationMs = static_cast<u64>(duration.count());
}

void ScriptWatchdog::reportStats() const
{
    spdlog::info("[BedrockAddon] Watchdog stats: {} ticks, {} timeouts, {} OOM events",
        m_totalTickCount,
        m_timeoutCount,
        m_oomCount);
}

const ScriptWatchdog::Config& ScriptWatchdog::config() const
{
    return m_config;
}

void ScriptWatchdog::setConfig(const Config& config)
{
    m_config = config;
}

u64 ScriptWatchdog::lastTickDurationMs() const
{
    return m_lastTickDurationMs;
}

} // namespace mc::mod::bedrock::addon
