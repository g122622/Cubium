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

#include "StorageTask.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include <atomic>
#include <memory>

namespace mc::world::storage {

/**
 * @brief 存储任务管理器
 *
 * 负责把存储任务提交到指定的 Worker 池，并保留任务 ID 以便后续取消。
 */
class StorageTaskManager {
public:
    explicit StorageTaskManager(util::ServerWorkerPool& workerPool);

    /**
     * @brief 设置 Worker 池
     */
    void setWorkerPool(util::ServerWorkerPool* workerPool);

    /**
     * @brief 提交任务
     */
    u64 submit(std::unique_ptr<StorageTask> task,
        util::TaskPriority priority,
        util::TaskCallback callback = nullptr,
        std::shared_ptr<std::atomic<bool>> cancelToken = nullptr);

    /**
     * @brief 取消任务
     */
    bool cancel(u64 taskId);

    /**
     * @brief 等待所有任务完成
     */
    void waitForCompletion();

private:
    util::ServerWorkerPool* m_workerPool = nullptr;
};

} // namespace mc::world::storage