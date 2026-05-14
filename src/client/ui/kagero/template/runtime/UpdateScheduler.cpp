#include "UpdateScheduler.hpp"
#include <algorithm>

namespace mc::client::ui::kagero::tpl::runtime {

// ========== UpdateScheduler实现 ==========

UpdateScheduler::UpdateScheduler()
    : m_batchDelayMs(16)
    , m_maxBatchSize(100)
    , m_deferredUpdate(true)
{}

UpdateScheduler::~UpdateScheduler()
{
    cancelAll();
}

u64 UpdateScheduler::schedule(const std::string& path, Priority priority)
{
    auto task = std::make_unique<UpdateTask>(path, priority, m_nextTimestamp++);
    u64 taskId = reinterpret_cast<u64>(task.get());
    m_pathToTasks[path].push_back(taskId);
    m_tasks.push_back(std::move(task));
    return taskId;
}

void UpdateScheduler::cancel(u64 taskId)
{
    for (auto& task : m_tasks) {
        if (reinterpret_cast<u64>(task.get()) == taskId) {
            task->cancelled = true;
            // 从路径映射中移除
            auto it = m_pathToTasks.find(task->path);
            if (it != m_pathToTasks.end()) {
                auto& ids = it->second;
                ids.erase(std::remove(ids.begin(), ids.end(), taskId), ids.end());
            }
            break;
        }
    }
}

void UpdateScheduler::cancelByPath(const std::string& path)
{
    auto it = m_pathToTasks.find(path);
    if (it != m_pathToTasks.end()) {
        for (u64 taskId : it->second) {
            for (auto& task : m_tasks) {
                if (reinterpret_cast<u64>(task.get()) == taskId) {
                    task->cancelled = true;
                    break;
                }
            }
        }
        m_pathToTasks.erase(it);
    }
}

void UpdateScheduler::cancelAll()
{
    m_tasks.clear();
    m_pathToTasks.clear();
}

u32 UpdateScheduler::executePending()
{
    deduplicatePaths();

    u32 count = 0;
    count += executeHighPriority();
    count += executeNormalPriority();
    count += executeLowPriority();

    // 清理已完成的任务
    m_tasks.erase(
        std::remove_if(
            m_tasks.begin(), m_tasks.end(), [](const std::unique_ptr<UpdateTask>& task) { return task->cancelled; }),
        m_tasks.end());

    return count;
}

u32 UpdateScheduler::executeHighPriority()
{
    return executePriority(Priority::High);
}

u32 UpdateScheduler::executeNormalPriority()
{
    return executePriority(Priority::Normal);
}

u32 UpdateScheduler::executeLowPriority()
{
    return executePriority(Priority::Low);
}

u32 UpdateScheduler::executeBatch()
{
    if (!m_updateCallback) return 0;

    deduplicatePaths();

    u32 count = 0;
    std::set<std::string> processedPaths;

    for (const auto& task : m_tasks) {
        if (task->cancelled) continue;
        if (processedPaths.count(task->path) > 0) continue;

        m_updateCallback(task->path);
        processedPaths.insert(task->path);
        task->cancelled = true;
        ++count;

        if (processedPaths.size() >= m_maxBatchSize) break;
    }

    return count;
}

u32 UpdateScheduler::pendingCount() const
{
    u32 count = 0;
    for (const auto& task : m_tasks) {
        if (!task->cancelled) ++count;
    }
    return count;
}

u32 UpdateScheduler::pendingCount(Priority priority) const
{
    u32 count = 0;
    for (const auto& task : m_tasks) {
        if (!task->cancelled && task->priority == priority) ++count;
    }
    return count;
}

u64 UpdateScheduler::currentTimestamp() const
{
    return m_nextTimestamp;
}

u32 UpdateScheduler::executePriority(Priority priority)
{
    if (!m_updateCallback) return 0;

    // 先收集要执行的任务，避免在迭代过程中修改容器
    std::vector<std::pair<u64, std::string>> tasksToExecute;
    for (const auto& task : m_tasks) {
        if (!task->cancelled && task->priority == priority) {
            tasksToExecute.emplace_back(reinterpret_cast<u64>(task.get()), task->path);
        }
    }

    // 执行任务
    u32 count = 0;
    for (const auto& [taskId, path] : tasksToExecute) {
        // 检查任务是否仍有效（可能在回调中被取消）
        bool stillValid = false;
        for (const auto& task : m_tasks) {
            if (reinterpret_cast<u64>(task.get()) == taskId && !task->cancelled) {
                stillValid = true;
                break;
            }
        }
        if (!stillValid) continue;

        m_updateCallback(path);

        // 标记为已完成
        for (auto& task : m_tasks) {
            if (reinterpret_cast<u64>(task.get()) == taskId) {
                task->cancelled = true;
                break;
            }
        }
        ++count;
    }

    return count;
}

void UpdateScheduler::deduplicatePaths()
{
    // 对每个路径只保留最新任务
    for (auto& [path, taskIds] : m_pathToTasks) {
        if (taskIds.size() <= 1) continue;

        // 找到最新任务（最高时间戳）
        u64 latestTaskId = 0;
        u64 latestTimestamp = 0;
        for (u64 taskId : taskIds) {
            for (const auto& task : m_tasks) {
                if (reinterpret_cast<u64>(task.get()) == taskId && task->timestamp > latestTimestamp) {
                    latestTaskId = taskId;
                    latestTimestamp = task->timestamp;
                }
            }
        }

        // 取消其他任务
        for (u64 taskId : taskIds) {
            if (taskId != latestTaskId) {
                for (auto& task : m_tasks) {
                    if (reinterpret_cast<u64>(task.get()) == taskId) {
                        task->cancelled = true;
                        break;
                    }
                }
            }
        }
    }
}

} // namespace mc::client::ui::kagero::tpl::runtime
