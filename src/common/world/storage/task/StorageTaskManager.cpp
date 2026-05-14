#include "StorageTaskManager.hpp"

namespace mc::world::storage {

StorageTaskManager::StorageTaskManager(util::ServerWorkerPool& workerPool)
    : m_workerPool(&workerPool)
{}

void StorageTaskManager::setWorkerPool(util::ServerWorkerPool* workerPool)
{
    m_workerPool = workerPool;
}

u64 StorageTaskManager::submit(std::unique_ptr<StorageTask> task,
    util::TaskPriority priority,
    util::TaskCallback callback,
    std::shared_ptr<std::atomic<bool>> cancelToken)
{
    if (!m_workerPool || !task) {
        if (callback) {
            callback(false, nullptr);
        }
        return 0;
    }

    return m_workerPool->submit(std::move(task), std::move(callback), priority, std::move(cancelToken));
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