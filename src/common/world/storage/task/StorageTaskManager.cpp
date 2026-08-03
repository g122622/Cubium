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

#include "StorageTaskManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/storage/task/StorageTask.hpp"
#include <atomic>
#include <memory>
#include <utility>

namespace mc::world::storage {

StorageTaskManager::StorageTaskManager(util::UniversalWorkerPool& workerPool) noexcept
    : m_workerPool(&workerPool)
{}

void StorageTaskManager::setWorkerPool(util::UniversalWorkerPool* workerPool) noexcept
{
    m_workerPool = workerPool;
}

u64 StorageTaskManager::submit(std::unique_ptr<StorageTask> task,
    util::TaskPriority priority,
    util::TaskCallback callback,
    std::shared_ptr<std::atomic<bool>> abortSignal)
{
    // 空指针检查：作为公共接口，需要验证外部输入
    if (!m_workerPool || !task) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    return m_workerPool->submit(std::move(task), std::move(callback), priority, std::move(abortSignal));
}

bool StorageTaskManager::cancel(u64 taskId)
{
    return m_workerPool ? m_workerPool->cancel(taskId) : false;
}

void StorageTaskManager::waitForCompletion()
{
    if (m_workerPool) {
        m_workerPool->waitForCompletion();
    }
}

} // namespace mc::world::storage