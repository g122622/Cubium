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

#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

namespace mc::mod::bedrock::addon {

ScriptScheduler::RunId ScriptScheduler::run(Callback callback)
{
    return runTimeout(std::move(callback), 1);
}

ScriptScheduler::RunId ScriptScheduler::runTimeout(Callback callback, u32 tickDelay)
{
    std::lock_guard lock(m_mutex);
    const RunId id = _nextId();
    m_entries.emplace(id, ScheduledEntry{std::move(callback), m_currentTick + tickDelay, 0});
    return id;
}

ScriptScheduler::RunId ScriptScheduler::runInterval(Callback callback, u32 tickInterval)
{
    std::lock_guard lock(m_mutex);
    const RunId id = _nextId();
    const u32 interval = std::max(tickInterval, 1u);
    m_entries.emplace(id, ScheduledEntry{std::move(callback), m_currentTick + interval, interval});
    return id;
}

bool ScriptScheduler::clearRun(RunId runId)
{
    std::lock_guard lock(m_mutex);
    return m_entries.erase(runId) != 0;
}

ScriptScheduler::RunId ScriptScheduler::runJob(JobStep step)
{
    std::lock_guard lock(m_mutex);
    const RunId id = _nextId();
    m_jobs.emplace(id, std::move(step));
    return id;
}

bool ScriptScheduler::clearJob(RunId jobId)
{
    std::lock_guard lock(m_mutex);
    return m_jobs.erase(jobId) != 0;
}

void ScriptScheduler::tick(u64 currentTick)
{
    std::vector<RunId> ready;
    {
        std::lock_guard lock(m_mutex);
        MC_ASSERT_RELEASE(currentTick >= m_currentTick);
        m_currentTick = currentTick;
        ready.reserve(m_entries.size());
        for (const auto& [id, entry] : m_entries) {
            if (entry.nextTick <= currentTick) {
                ready.push_back(id);
            }
        }
    }

    // 执行前重新查表，使前面的回调能够取消本轮尚未执行的任务。
    for (const RunId id : ready) {
        Callback callback;
        {
            std::lock_guard lock(m_mutex);
            const auto it = m_entries.find(id);
            if (it == m_entries.end()) {
                continue;
            }
            callback = it->second.callback;
            if (it->second.interval == 0) {
                m_entries.erase(it);
            } else {
                it->second.nextTick = currentTick + it->second.interval;
            }
        }
        callback();
    }
    _tickJobs();
}

void ScriptScheduler::_tickJobs()
{
    std::vector<RunId> active;
    {
        std::lock_guard lock(m_mutex);
        active.reserve(m_jobs.size());
        for (const auto& [id, step] : m_jobs) {
            active.push_back(id);
        }
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    do {
        for (auto it = active.begin(); it != active.end();) {
            JobStep step;
            {
                std::lock_guard lock(m_mutex);
                const auto job = m_jobs.find(*it);
                if (job != m_jobs.end()) {
                    step = job->second;
                }
            }
            if (!step || step()) {
                clearJob(*it);
                it = active.erase(it);
            } else {
                ++it;
            }
        }
    } while (!active.empty() && std::chrono::steady_clock::now() < deadline);
}

void ScriptScheduler::clearAll()
{
    std::lock_guard lock(m_mutex);
    m_entries.clear();
    m_jobs.clear();
}

size_t ScriptScheduler::pendingCount() const
{
    std::lock_guard lock(m_mutex);
    return m_entries.size() + m_jobs.size();
}

ScriptScheduler::RunId ScriptScheduler::_nextId()
{
    MC_ASSERT_RELEASE(m_nextId < std::numeric_limits<RunId>::max());
    return m_nextId++;
}

} // namespace mc::mod::bedrock::addon
