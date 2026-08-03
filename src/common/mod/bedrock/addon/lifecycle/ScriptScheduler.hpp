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

#include "common/core/Types.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本调度器
 *
 * 实现system.run()/runInterval()/runTimeout()/clearRun()的调度逻辑。
 * 在ScriptTickListener每tick驱动时执行到期的回调。
 *
 * 线程安全：所有公共方法通过mutex保护，可从任意线程调度回调。
 */
class ScriptScheduler {
public:
    /**
     * @brief 调度回调ID类型
     */
    using RunId = i32;

    /**
     * @brief 调度回调类型
     */
    using Callback = std::function<void()>;

    ScriptScheduler() = default;
    ~ScriptScheduler() = default;

    // 禁止拷贝
    ScriptScheduler(const ScriptScheduler&) = delete;
    ScriptScheduler& operator=(const ScriptScheduler&) = delete;

    // 禁止移动（因为包含std::mutex）
    ScriptScheduler(ScriptScheduler&&) = delete;
    ScriptScheduler& operator=(ScriptScheduler&&) = delete;

    /**
     * @brief 在下一tick执行回调（system.run）
     *
     * @param callback 要执行的回调
     * @return 调度ID，可用于clearRun取消
     */
    RunId run(Callback callback);

    /**
     * @brief 延迟指定tick后执行回调（system.runTimeout）
     *
     * @param callback 要执行的回调
     * @param tickDelay 延迟tick数
     * @return 调度ID
     */
    RunId runTimeout(Callback callback, u32 tickDelay);

    /**
     * @brief 每隔指定tick数执行回调（system.runInterval）
     *
     * @param callback 要执行的回调
     * @param tickInterval 间隔tick数
     * @return 调度ID
     */
    RunId runInterval(Callback callback, u32 tickInterval);

    /**
     * @brief 取消已注册的调度（system.clearRun）
     *
     * @param runId 调度ID
     * @return 是否成功取消
     */
    bool clearRun(RunId runId);

    /**
     * @brief 每tick调用，执行到期的回调
     *
     * @param currentTick 当前服务器tick
     */
    void tick(u64 currentTick);

    /**
     * @brief 清除所有调度
     */
    void clearAll();

    /**
     * @brief 获取待执行的调度数量（仅用于诊断）
     */
    [[nodiscard]] size_t pendingCount() const;

private:
    /**
     * @brief 定时回调条目
     */
    struct ScheduledEntry {
        RunId id;
        Callback callback;
        u64 nextTick = 0;       // 下次执行的tick
        u32 interval = 0;       // 周期性回调的间隔tick数
        bool recurring = false; // true=周期性回调，false=一次性回调
        bool cancelled = false;
    };

    /**
     * @brief 生成唯一调度ID
     */
    RunId _nextId();

    mutable std::mutex m_mutex;
    std::unordered_map<RunId, ScheduledEntry> m_entries;
    std::vector<Callback> m_pendingRunCallbacks; // 下一tick立即执行的回调
    std::atomic<RunId> m_nextId{1};
};

} // namespace mc::mod::bedrock::addon
