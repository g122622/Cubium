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
#include <thread>

namespace mc::client {

/**
 * @brief 内存追踪线程
 *
 * 在独立线程中定期采样进程内存使用量并写入 Perfetto 追踪。
 * 避免内存采样阻塞主渲染循环。
 *
 * 采样频率：每秒 100 次（每 10ms 一次）。
 */
class MemoryTraceThread {
public:
    MemoryTraceThread();
    ~MemoryTraceThread();

    // 禁止拷贝
    MemoryTraceThread(const MemoryTraceThread&) = delete;
    MemoryTraceThread& operator=(const MemoryTraceThread&) = delete;

    /**
     * @brief 启动追踪线程
     */
    void start();

    /**
     * @brief 停止追踪线程
     */
    void stop();

    /**
     * @brief 检查线程是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(std::memory_order::acquire); }

private:
    /**
     * @brief 线程主循环
     */
    void run();

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
};

} // namespace mc::client
