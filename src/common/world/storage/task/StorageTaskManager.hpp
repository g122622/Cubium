#pragma once

#include "common/util/thread/ServerWorkerPool.hpp"
#include "StorageTask.hpp"
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