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

#include "ConsoleLogConsumer.hpp"

#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <utility>

namespace mc::server::console {

using namespace mc::trace;

ConsoleLogConsumer::ConsoleLogConsumer(LogRingBuffer& buffer, std::function<void()> onLineAvailable)
    : m_buffer(buffer)
    , m_onLineAvailable(std::move(onLineAvailable))
{
    MC_ASSERT_RELEASE_MSG(m_onLineAvailable != nullptr, "ConsoleLogConsumer requires a wakeup callback");
}

void ConsoleLogConsumer::OnLogLine(spdlog::level::level_enum level, std::string_view line)
{
    // 运行在 spdlog 异步消费线程：用长期事件让 trace 能看到"日志消费线程"的负载，
    // 每条日志一个瞬时事件则可估算日志吞吐。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "ConsoleLogConsumer::OnLogLine");
    m_buffer.push(LogLine{level, std::string(line)});
    m_linesForwarded.fetch_add(1, std::memory_order_relaxed);
    m_onLineAvailable();
}

} // namespace mc::server::console
