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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HAVING BEEN CLAIMED FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <iterator>
#include <mutex>
#include <utility>
#include <vector>

namespace mc::mod::bedrock::addon {

ScriptScheduler::RunId ScriptScheduler::run(Callback callback)
{
    std::lock_guard lock(m_mutex);
    RunId id = _nextId();
    m_pendingRunCallbacks.push_back(std::move(callback));
    return id;
}

ScriptScheduler::RunId ScriptScheduler::runTimeout(Callback callback, u32 tickDelay)
{
    std::lock_guard lock(m_mutex);
    RunId id = _nextId();
    // 一次性回调：nextTick=0表示首次tick时设置，recurring=false
    m_entries.emplace(id, ScheduledEntry{id, std::move(callback), 0, tickDelay, false, false});
    return id;
}

ScriptScheduler::RunId ScriptScheduler::runInterval(Callback callback, u32 tickInterval)
{
    std::lock_guard lock(m_mutex);
    RunId id = _nextId();
    // 周期性回调：recurring=true
    m_entries.emplace(id, ScheduledEntry{id, std::move(callback), 0, tickInterval, true, false});
    return id;
}

bool ScriptScheduler::clearRun(RunId runId)
{
    std::lock_guard lock(m_mutex);

    // 检查定时/周期回调
    auto it = m_entries.find(runId);
    if (it != m_entries.end()) {
        it->second.cancelled = true;
        m_entries.erase(it);
        return true;
    }

    // 下一tick回调无法取消（已经在待执行队列中）
    return false;
}

void ScriptScheduler::tick(u64 currentTick)
{
    // 收集待执行的回调
    std::vector<Callback> callbacksToRun;

    {
        std::lock_guard lock(m_mutex);

        // 1. 处理system.run()的即时回调
        callbacksToRun.insert(callbacksToRun.end(),
            std::make_move_iterator(m_pendingRunCallbacks.begin()),
            std::make_move_iterator(m_pendingRunCallbacks.end()));
        m_pendingRunCallbacks.clear();

        // 2. 处理定时和周期回调
        for (auto it = m_entries.begin(); it != m_entries.end();) {
            auto& entry = it->second;
            if (entry.cancelled) {
                it = m_entries.erase(it);
                continue;
            }

            // 首次tick时设置nextTick
            if (entry.nextTick == 0) {
                entry.nextTick = currentTick + entry.interval;
            }

            // 检查是否到期
            if (currentTick >= entry.nextTick) {
                callbacksToRun.push_back(entry.callback);

                if (entry.recurring) {
                    // 周期性回调：更新下次执行时间
                    entry.nextTick = currentTick + entry.interval;
                    ++it;
                } else {
                    // 一次性回调：移除
                    it = m_entries.erase(it);
                }
            } else {
                ++it;
            }
        }
    }

    // 3. 在锁外执行回调（避免回调中调用调度方法导致死锁）
    for (auto& callback : callbacksToRun) {
        if (callback) {
            callback();
        }
    }
}

void ScriptScheduler::clearAll()
{
    std::lock_guard lock(m_mutex);
    m_entries.clear();
    m_pendingRunCallbacks.clear();
}

size_t ScriptScheduler::pendingCount() const
{
    std::lock_guard lock(m_mutex);
    return m_entries.size() + m_pendingRunCallbacks.size();
}

ScriptScheduler::RunId ScriptScheduler::_nextId()
{
    return m_nextId.fetch_add(1);
}

} // namespace mc::mod::bedrock::addon
