#pragma once

#include "common/util/thread/ITask.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include "common/core/Result.hpp"
#include <functional>
#include <memory>
#include <string>

namespace mc::world::storage {

/**
 * @brief 存储任务类型
 */
enum class StorageTaskType : u8 {
    SectionLoad,
    SectionSave,
    SectionFlush,
    SnapshotCreate,
    SnapshotRestore
};

/**
 * @brief 存储任务包装
 *
 * 这是一个轻量级任务封装，只负责描述、追踪和执行一个已准备好的 I/O 工作单元。
 */
class StorageTask : public util::ITask {
public:
    using Executor = std::function<bool(const std::atomic<bool>&)>;

    /**
     * @brief 创建加载任务
     */
    static std::unique_ptr<StorageTask> createLoadTask(const SectionKey& key, Executor executor);

    /**
     * @brief 创建保存任务
     */
    static std::unique_ptr<StorageTask> createSaveTask(const SectionKey& key, bool immediate, Executor executor);

    /**
     * @brief 创建刷盘任务
     */
    static std::unique_ptr<StorageTask> createFlushTask(DimensionId dimension, size_t count, Executor executor);

    bool execute(const std::atomic<bool>& cancelSignal) override;
    void onCancel() override;
    util::TaskType type() const override;
    std::string description() const override;
    const char* traceCategory() const override;

private:
    StorageTask(StorageTaskType type, std::string description, const char* traceCategory, Executor executor);

    StorageTaskType m_type;
    std::string m_description;
    const char* m_traceCategory;
    Executor m_executor;
};

} // namespace mc::world::storage