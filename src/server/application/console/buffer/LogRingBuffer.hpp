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

#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <spdlog/common.h>

namespace mc::server::console {

/**
 * @brief 终端日志区的一条日志
 */
struct LogLine {
    spdlog::level::level_enum level = spdlog::level::info; ///< 级别（决定着色）
    std::string text;                                      ///< 已格式化文本（不含换行）
};

/**
 * @brief 线程安全的定长日志环形缓冲
 *
 * 生产方是 spdlog 的异步消费线程（ConsoleLogConsumer），消费方是 UI 线程。容量满时丢弃
 * 最旧的一行（与 LogManager 的 overrun_oldest 策略保持一致：崩溃前最后几行最有用）。
 *
 * "跟随最新"与回看偏移由 UI 侧维护（见 LogViewComponent），缓冲本身只负责按序保存与查询。
 */
class LogRingBuffer {
public:
    /**
     * @brief 构造缓冲
     * @param capacity 保留的最大行数（必须大于 0）
     */
    explicit LogRingBuffer(usize capacity);

    /**
     * @brief 追加一行日志（线程安全）
     * @param line 日志内容
     */
    void push(LogLine line);

    /**
     * @brief 批量追加（线程安全，单次加锁）
     * @param lines 日志内容
     */
    void pushMany(std::vector<LogLine> lines);

    /**
     * @brief 拷贝当前全部日志（线程安全）
     * @return 由旧到新排列的日志副本
     */
    [[nodiscard]] std::vector<LogLine> snapshot() const;

    /**
     * @brief 当前保存的行数（线程安全）
     */
    [[nodiscard]] usize size() const;

    /**
     * @brief 容量（不可变，无需加锁）
     */
    [[nodiscard]] usize capacity() const noexcept
    {
        return m_capacity;
    }

    /**
     * @brief 累计接收过的行数（含已被挤出缓冲的行，线程安全）
     *
     * UI 用它判断是否有新行到达（避免每帧整份拷贝比较）。
     */
    [[nodiscard]] u64 pushedCount() const;

private:
    const usize m_capacity;
    mutable std::mutex m_mutex;
    std::deque<LogLine> m_lines;
    u64 m_pushedCount = 0;
};

} // namespace mc::server::console
