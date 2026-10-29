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

#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

namespace mc::server::console {

/// 补全查询等待 tick 线程响应的超时。超时后返回空候选，避免 UI 线程被卡死。
constexpr auto COMPLETION_TIMEOUT = std::chrono::milliseconds(500);

/**
 * @brief UI 线程 → tick 线程的同步补全查询
 *
 * 命令树与命令源（ServerCommandSource）只能在 tick 线程访问：补全时会调用命令节点的
 * requirement（读取 IServer），与执行命令同属一个线程约束。因此 UI 线程发起补全时把
 * 请求挂到本队列，tick 线程在 post-tick 回调里结算。
 *
 * 同一时刻最多挂起一个请求（补全只由输入框触发，UI 线程调用是串行的）。
 */
class CompletionRequestQueue {
public:
    /**
     * @brief 一次补全请求的结果
     */
    struct Response {
        bool completed = false;               ///< tick 线程是否已结算
        mc::command::Suggestions suggestions; ///< 结算后的候选（可能为空）
    };

    /**
     * @brief 提交请求并阻塞等待结果（UI 线程调用）
     * @param input 当前输入
     * @return 候选；超时或 UI 退出时为空
     */
    [[nodiscard]] mc::command::Suggestions query(const std::string& input);

    /**
     * @brief 取出挂起的请求（tick 线程调用），并通知 UI 线程"已开始服务"。
     *
     * UI 线程在 query 中分两段等待（0.5s + 0.5s）：第一段超时后，只有确认请求已被
     * tick 线程取走，才继续等待结算，避免服务端卡死时 UI 崩溃。
     *
     * @param started 输出：是否有请求被取走
     * @return 请求的输入文本（无请求时为 nullopt）
     */
    [[nodiscard]] std::optional<std::string> takePending(bool& started);

    /**
     * @brief 结算请求（tick 线程调用）
     * @param suggestions 候选结果
     */
    void respond(mc::command::Suggestions suggestions);

    /**
     * @brief 放弃当前挂起请求（停止 UI 时调用，避免 UI 线程一直等到超时）
     */
    void cancel();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::optional<std::string> m_pendingInput;
    Response m_response;
    bool m_taken = false; ///< 当前请求是否已被 tick 线程取走
};

} // namespace mc::server::console
