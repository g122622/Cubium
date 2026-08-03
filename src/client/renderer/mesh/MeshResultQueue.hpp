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

#include "MeshWorkerTypes.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>

namespace mc::client {

/**
 * @brief 线程安全的网格构建结果队列
 *
 * 迁移到 UniversalWorkerPool 后取代原 MeshWorkerPool 的 m_completedQueue。
 * worker 线程（MeshBuildTask::execute 内）push 结果，主线程（ClientWorld::processMeshBuildResults）
 * drain 并按 scheduler 的过滤逻辑消费。
 *
 * 生命周期由 ClientWorld 通过 shared_ptr 持有，MeshBuildTask 持 weak_ptr，
 * 保证池晚于 scheduler 析构时晚到的回调安全（lock 失败即丢弃）。
 */
class MeshResultQueue {
public:
    MeshResultQueue();
    ~MeshResultQueue();

    MeshResultQueue(const MeshResultQueue&) = delete;
    MeshResultQueue& operator=(const MeshResultQueue&) = delete;

    void push(MeshWorkerResult&& result);

    /**
     * @brief 排空结果队列，对每个结果调用 callback（最多 maxCount 个）。
     */
    void drain(const std::function<void(MeshWorkerResult&&)>& callback, u32 maxCount);

    [[nodiscard]] size_t size() const;

private:
    mutable std::mutex m_mutex;
    std::queue<MeshWorkerResult> m_queue;
};

} // namespace mc::client
