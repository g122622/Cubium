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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace mc::world::storage {

/**
 * @brief 存储任务类型
 */
enum class StorageTaskType : u8 { SectionLoad, SectionSave, SectionFlush, SnapshotCreate, SnapshotRestore };

/**
 * @brief 存储任务包装
 *
 * 这是一个轻量级任务封装，只负责描述、追踪和执行一个已准备好的 I/O 工作单元。
 */
class StorageTask : public util::ITask {
public:
    using Executor = std::function<bool(const std::atomic<bool>&)>;

    // 禁止拷贝
    StorageTask(const StorageTask&) = delete;
    StorageTask& operator=(const StorageTask&) = delete;

    // 移动操作
    StorageTask(StorageTask&& other) noexcept;
    StorageTask& operator=(StorageTask&& other) noexcept;

    ~StorageTask() noexcept override = default;

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

    bool execute(const std::atomic<bool>& abortSignal) override;
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