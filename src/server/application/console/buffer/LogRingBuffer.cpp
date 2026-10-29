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

#include "LogRingBuffer.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <utility>

namespace mc::server::console {

LogRingBuffer::LogRingBuffer(usize capacity)
    : m_capacity(capacity)
{
    MC_ASSERT_RELEASE_MSG(capacity > 0, "LogRingBuffer capacity must be positive");
}

void LogRingBuffer::push(LogLine line)
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    m_lines.push_back(std::move(line));
    ++m_pushedCount;
    while (m_lines.size() > m_capacity) {
        m_lines.pop_front();
    }
}

void LogRingBuffer::pushMany(std::vector<LogLine> lines)
{
    if (lines.empty()) {
        return;
    }

    const std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& line : lines) {
        m_lines.push_back(std::move(line));
        ++m_pushedCount;
    }
    while (m_lines.size() > m_capacity) {
        m_lines.pop_front();
    }
}

std::vector<LogLine> LogRingBuffer::snapshot() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return {m_lines.begin(), m_lines.end()};
}

usize LogRingBuffer::size() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_lines.size();
}

u64 LogRingBuffer::pushedCount() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_pushedCount;
}

} // namespace mc::server::console
