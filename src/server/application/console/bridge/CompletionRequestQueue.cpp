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

#include "CompletionRequestQueue.hpp"

#include <utility>

namespace mc::server::console {

mc::command::Suggestions CompletionRequestQueue::query(const std::string& input)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // 上一轮请求若还挂着（理论上不会），直接让位给新请求。
    m_response = Response{};
    m_taken = false;
    m_pendingInput = input;

    // 第一段等待：等待 tick 线程取走请求。服务端已停止时不会有人处理，0.5s 后放弃，
    // 避免 UI 线程长时间阻塞在补全上。
    const bool started = m_cv.wait_for(lock, COMPLETION_TIMEOUT, [this]() {
        return m_taken;
    });
    if (!started) {
        m_pendingInput.reset();
        return mc::command::Suggestions::empty();
    }

    // 第二段等待：请求已被取走，等结算结果（同样有上限，防止 tick 线程异常退出）。
    const bool answered = m_cv.wait_for(lock, COMPLETION_TIMEOUT, [this]() {
        return m_response.completed;
    });

    m_pendingInput.reset();
    m_taken = false;
    if (!answered) {
        return mc::command::Suggestions::empty();
    }
    return std::move(m_response.suggestions);
}

std::optional<std::string> CompletionRequestQueue::takePending(bool& started)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    started = false;
    if (!m_pendingInput.has_value()) {
        return std::nullopt;
    }

    m_taken = true;
    started = true;
    // takePending 返回后响应由 respond() 写回；UI 线程此时在等第二个条件。
    m_cv.notify_all();
    return m_pendingInput;
}

void CompletionRequestQueue::respond(mc::command::Suggestions suggestions)
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_response.completed = true;
        m_response.suggestions = std::move(suggestions);
    }
    m_cv.notify_all();
}

void CompletionRequestQueue::cancel()
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingInput.reset();
        m_taken = true;
        m_response.completed = true;
    }
    m_cv.notify_all();
}

} // namespace mc::server::console
