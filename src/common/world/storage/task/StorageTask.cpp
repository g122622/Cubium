#include "StorageTask.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <fmt/format.h>

namespace mc::world::storage {

std::unique_ptr<StorageTask> StorageTask::createLoadTask(const SectionKey& key, Executor executor)
{
    return std::unique_ptr<StorageTask>(new StorageTask(
        StorageTaskType::SectionLoad,
        fmt::format("SectionLoad({}, {}, {}, {})", key.chunkX, key.chunkZ, static_cast<i32>(key.sectionY), static_cast<i32>(key.dimension)),
        "storage.task.load",
        std::move(executor)));
}

std::unique_ptr<StorageTask> StorageTask::createSaveTask(const SectionKey& key, bool immediate, Executor executor)
{
    return std::unique_ptr<StorageTask>(new StorageTask(
        StorageTaskType::SectionSave,
        fmt::format("SectionSave({}, {}, {}, {}, immediate={})", key.chunkX, key.chunkZ, static_cast<i32>(key.sectionY), static_cast<i32>(key.dimension), immediate),
        "storage.task.save",
        std::move(executor)));
}

std::unique_ptr<StorageTask> StorageTask::createFlushTask(DimensionId dimension, size_t count, Executor executor)
{
    return std::unique_ptr<StorageTask>(new StorageTask(
        StorageTaskType::SectionFlush,
        fmt::format("SectionFlush(dim={}, count={})", static_cast<i32>(dimension), count),
        "storage.task.flush",
        std::move(executor)));
}

StorageTask::StorageTask(StorageTaskType type, std::string description, const char* traceCategory, Executor executor)
    : m_type(type)
    , m_description(std::move(description))
    , m_traceCategory(traceCategory)
    , m_executor(std::move(executor))
{
}

bool StorageTask::execute(const std::atomic<bool>& cancelSignal)
{
    MC_TRACE_EVENT("storage.task", "StorageTask::execute", "description", m_description);
    return m_executor ? m_executor(cancelSignal) : false;
}

void StorageTask::onCancel()
{
}

util::TaskType StorageTask::type() const
{
    return m_type == StorageTaskType::SectionLoad ? util::TaskType::ChunkLoad
         : m_type == StorageTaskType::SectionSave ? util::TaskType::ChunkSave
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