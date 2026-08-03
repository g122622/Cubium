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

#include "UpdateScheduler.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::tpl::runtime {

// ========== UpdateScheduler实现 ==========

UpdateScheduler::~UpdateScheduler()
{
    cancelAll();
}

u64 UpdateScheduler::schedule(const std::string& path, Priority priority)
{
    const u64 dueTimeMs = _computeDueTime();
    auto task = std::make_unique<UpdateTask>(path, priority, m_nextTimestamp++, dueTimeMs);
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
                if (ids.empty()) {
                    m_pathToTasks.erase(it);
                }
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
    _deduplicatePaths();

    u32 count = 0;
    count += executeHighPriority();
    count += executeNormalPriority();
    count += executeLowPriority();

    // 清理已完成的任务
    _cleanupCancelled();

    return count;
}

u32 UpdateScheduler::executeHighPriority()
{
    return _executePriority(Priority::High, false);
}

u32 UpdateScheduler::executeNormalPriority()
{
    return _executePriority(Priority::Normal, false);
}

u32 UpdateScheduler::executeLowPriority()
{
    return _executePriority(Priority::Low, false);
}

u32 UpdateScheduler::executeBatch()
{
    if (!m_updateCallback) return 0;

    _deduplicatePaths();

    u32 count = 0;
    std::set<std::string> processedPaths;

    for (const auto& task : m_tasks) {
        if (task->cancelled) continue;
        if (!_isDue(*task, false)) continue;
        if (processedPaths.count(task->path) > 0) continue;

        m_updateCallback(task->path);
        processedPaths.insert(task->path);
        task->cancelled = true;
        ++count;

        if (processedPaths.size() >= m_maxBatchSize) break;
    }

    _cleanupCancelled();

    return count;
}

u32 UpdateScheduler::flush()
{
    _deduplicatePaths();

    u32 count = 0;
    count += _executePriority(Priority::High, true);
    count += _executePriority(Priority::Normal, true);
    count += _executePriority(Priority::Low, true);

    _cleanupCancelled();

    return count;
}

u32 UpdateScheduler::tick(u64 currentMs)
{
    m_nowMs = currentMs;
    return executePending();
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

u32 UpdateScheduler::dueCount(u64 currentMs) const
{
    // 禁用延迟更新时，所有未取消任务都视为已到期
    if (!m_deferredUpdate) {
        return pendingCount();
    }

    u32 count = 0;
    for (const auto& task : m_tasks) {
        if (task->cancelled) continue;
        if (task->dueTimeMs <= currentMs) ++count;
    }
    return count;
}

u64 UpdateScheduler::currentTimestamp() const
{
    return m_nextTimestamp;
}

u32 UpdateScheduler::_executePriority(Priority priority, bool forceDue)
{
    if (!m_updateCallback) return 0;

    // 先收集要执行的任务，避免在迭代过程中修改容器
    std::vector<std::pair<u64, std::string>> tasksToExecute;
    for (const auto& task : m_tasks) {
        if (task->cancelled) continue;
        if (task->priority != priority) continue;
        if (!_isDue(*task, forceDue)) continue;

        tasksToExecute.emplace_back(reinterpret_cast<u64>(task.get()), task->path);
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

bool UpdateScheduler::_isDue(const UpdateTask& task, bool forceDue) const
{
    if (forceDue) return true;
    if (!m_deferredUpdate) return true;
    return task.dueTimeMs <= m_nowMs;
}

void UpdateScheduler::_deduplicatePaths()
{
    // 对每个路径只保留最新任务
    for (auto& [path, taskIds] : m_pathToTasks) {
        if (taskIds.size() <= 1) continue;

        // 找到最新任务（最高 timestamp）
        u64 latestTaskId = 0;
        u64 latestTimestamp = 0;
        for (u64 taskId : taskIds) {
            for (const auto& task : m_tasks) {
                if (reinterpret_cast<u64>(task.get()) == taskId && !task->cancelled &&
                    task->timestamp > latestTimestamp) {
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

void UpdateScheduler::_cleanupCancelled()
{
    // 清理已取消的任务，并同步更新路径映射
    std::unordered_map<std::string, std::vector<u64>> newPathToTasks;

    m_tasks.erase(std::remove_if(m_tasks.begin(),
                      m_tasks.end(),
                      [&](const std::unique_ptr<UpdateTask>& task) {
                          if (!task->cancelled) {
                              // 保留任务：更新路径映射
                              auto it = m_pathToTasks.find(task->path);
                              if (it != m_pathToTasks.end()) {
                                  auto& ids = newPathToTasks[task->path];
                                  for (u64 id : it->second) {
                                      if (id == reinterpret_cast<u64>(task.get())) {
                                          ids.push_back(id);
                                          break;
                                      }
                                  }
                              }
                              return false;
                          }
                          return true;
                      }),
        m_tasks.end());

    m_pathToTasks = std::move(newPathToTasks);
}

u64 UpdateScheduler::_computeDueTime() const
{
    if (!m_deferredUpdate) {
        return 0;
    }
    return m_nowMs + m_batchDelayMs;
}

} // namespace mc::client::ui::kagero::tpl::runtime
