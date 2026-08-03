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

#include "StorageTask.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/storage/db/SectionKey.hpp"
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <fmt/format.h>

using namespace mc::trace;

namespace mc::world::storage {

std::unique_ptr<StorageTask> StorageTask::createLoadTask(const SectionKey& key, Executor executor)
{
    return std::unique_ptr<StorageTask>(new StorageTask(StorageTaskType::SectionLoad,
        fmt::format("SectionLoad({}, {}, {}, {})",
            key.chunkX,
            key.chunkZ,
            static_cast<i32>(key.sectionY),
            static_cast<i32>(key.dimension)),
        "storage.task.load",
        std::move(executor)));
}

std::unique_ptr<StorageTask> StorageTask::createSaveTask(const SectionKey& key, bool immediate, Executor executor)
{
    return std::unique_ptr<StorageTask>(new StorageTask(StorageTaskType::SectionSave,
        fmt::format("SectionSave({}, {}, {}, {}, immediate={})",
            key.chunkX,
            key.chunkZ,
            static_cast<i32>(key.sectionY),
            static_cast<i32>(key.dimension),
            immediate),
        "storage.task.save",
        std::move(executor)));
}

std::unique_ptr<StorageTask> StorageTask::createFlushTask(DimensionId dimension, size_t count, Executor executor)
{
    return std::unique_ptr<StorageTask>(new StorageTask(StorageTaskType::SectionFlush,
        fmt::format("SectionFlush(dim={}, count={})", static_cast<i32>(dimension), count),
        "storage.task.flush",
        std::move(executor)));
}

StorageTask::StorageTask(StorageTaskType type, std::string description, const char* traceCategory, Executor executor)
    : m_type(type)
    , m_description(std::move(description))
    , m_traceCategory(traceCategory)
    , m_executor(std::move(executor))
{}

StorageTask::StorageTask(StorageTask&& other) noexcept
    : m_type(other.m_type)
    , m_description(std::move(other.m_description))
    , m_traceCategory(other.m_traceCategory)
    , m_executor(std::move(other.m_executor))
{}

StorageTask& StorageTask::operator=(StorageTask&& other) noexcept
{
    if (this != &other) {
        m_type = other.m_type;
        m_description = std::move(other.m_description);
        m_traceCategory = other.m_traceCategory;
        m_executor = std::move(other.m_executor);
    }
    return *this;
}

bool StorageTask::execute(const std::atomic<bool>& abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "StorageTask::execute", "description", m_description);
    return m_executor ? m_executor(abortSignal) : false;
}

void StorageTask::onCancel() {}

util::TaskType StorageTask::type() const
{
    return m_type == StorageTaskType::SectionLoad ? util::TaskType::ChunkLoad
        : m_type == StorageTaskType::SectionSave  ? util::TaskType::ChunkSave
        : m_type == StorageTaskType::SectionFlush ? util::TaskType::DBWrite
                                                  : util::TaskType::Custom;
}

std::string StorageTask::description() const
{
    return m_description;
}

const char* StorageTask::traceCategory() const
{
    return m_traceCategory;
}

} // namespace mc::world::storage