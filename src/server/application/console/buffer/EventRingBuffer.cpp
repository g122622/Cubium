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

#include "EventRingBuffer.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <utility>

namespace mc::server::console {

namespace {

/// Minecraft 样式码前缀（U+00A7），UTF-8 编码为 0xC2 0xA7。
constexpr char SECTION_SIGN_FIRST_BYTE = static_cast<char>(0xC2);
constexpr char SECTION_SIGN_SECOND_BYTE = static_cast<char>(0xA7);

} // namespace

EventRingBuffer::EventRingBuffer(usize capacity)
    : m_capacity(capacity)
{
    MC_ASSERT_RELEASE_MSG(capacity > 0, "EventRingBuffer capacity must be positive");
}

void EventRingBuffer::push(TerminalEventKind kind, std::string text)
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    m_events.push_back(TerminalEvent{kind, std::move(text)});
    ++m_pushedCount;
    while (m_events.size() > m_capacity) {
        m_events.pop_front();
    }
}

std::vector<TerminalEvent> EventRingBuffer::snapshot() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return {m_events.begin(), m_events.end()};
}

usize EventRingBuffer::size() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_events.size();
}

u64 EventRingBuffer::pushedCount() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_pushedCount;
}

std::string stripFormattingCodes(std::string_view text)
{
    // 样式码在 UTF-8 中为两字节（0xC2 0xA7）+ 一个 ASCII 格式字符。逐字节扫描即可：
    // 命中前缀时跳过三字节，其余原样拷贝（多字节字符的非首字节不会等于 0xC2）。
    std::string result;
    result.reserve(text.size());
    usize index = 0;
    while (index < text.size()) {
        const char current = text[index];
        if (current == SECTION_SIGN_FIRST_BYTE && index + 2 < text.size() &&
            text[index + 1] == SECTION_SIGN_SECOND_BYTE) {
            index += 3;
            continue;
        }

        result.push_back(current);
        ++index;
    }

    return result;
}

} // namespace mc::server::console
