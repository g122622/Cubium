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

#pragma once
#include <chrono>
#include <deque>
#include <mutex>
#include <utility>

namespace mc::server {
/** @brief 按预算消费入口快照，旧项始终先于处理期间产生的新项。 */
template <typename T, typename Callback>
void drainChunkCompletions(
    std::deque<T>& queue, std::mutex& mutex, std::chrono::milliseconds budget, Callback&& callback)
{
    std::deque<T> pending;
    {
        std::lock_guard lock(mutex);
        pending.swap(queue);
    }
    const auto deadline = std::chrono::steady_clock::now() + budget;
    while (!pending.empty()) {
        T item = std::move(pending.front());
        pending.pop_front();
        callback(item);
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
    }
    if (!pending.empty()) {
        std::lock_guard lock(mutex);
        while (!pending.empty()) {
            queue.push_front(std::move(pending.back()));
            pending.pop_back();
        }
    }
}

/** @brief 完成接收的重入保护；内联存储回调只入队，不递归接收。 */
class ChunkCompletionGuard {
public:
    explicit ChunkCompletionGuard(bool& flag)
        : m_flag(flag)
    {
        m_flag = true;
    }

    ~ChunkCompletionGuard()
    {
        m_flag = false;
    }

    ChunkCompletionGuard(const ChunkCompletionGuard&) = delete;
    ChunkCompletionGuard& operator=(const ChunkCompletionGuard&) = delete;

private:
    bool& m_flag;
};
} // namespace mc::server
