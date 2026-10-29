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

#include "ConsoleCommandQueue.hpp"

#include <iterator>
#include <utility>

namespace mc::server::console {

void ConsoleCommandQueue::submit(std::string command)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_pending.push_back(std::move(command));
}

std::vector<std::string> ConsoleCommandQueue::drain()
{
    std::deque<std::string> pending;
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        pending.swap(m_pending);
    }

    // 在锁外搬移：命令文本可能较长，且 drain 之后不应再持锁。
    return {std::make_move_iterator(pending.begin()), std::make_move_iterator(pending.end())};
}

usize ConsoleCommandQueue::pendingCount() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_pending.size();
}

} // namespace mc::server::console
