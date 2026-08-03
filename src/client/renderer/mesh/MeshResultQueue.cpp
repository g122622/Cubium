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

#include "MeshResultQueue.hpp"
#include "client/renderer/mesh/MeshWorkerTypes.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

namespace mc::client {

MeshResultQueue::MeshResultQueue() = default;

MeshResultQueue::~MeshResultQueue() = default;

void MeshResultQueue::push(MeshWorkerResult&& result)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(std::move(result));
}

void MeshResultQueue::drain(const std::function<void(MeshWorkerResult&&)>& callback, u32 maxCount)
{
    if (!callback || maxCount == 0) {
        return;
    }

    u32 drained = 0;

    while (drained < maxCount) {
        MeshWorkerResult result;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                break;
            }
            result = std::move(m_queue.front());
            m_queue.pop();
        }

        callback(std::move(result));
        ++drained;
    }
}

size_t MeshResultQueue::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

} // namespace mc::client
