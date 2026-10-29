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

#include "common/application/LogManager.hpp"
#include "common/core/Types.hpp"
#include "server/application/console/buffer/LogRingBuffer.hpp"

#include <atomic>
#include <functional>
#include <string_view>

namespace mc::server::console {

/**
 * @brief 把 LogManager 的日志行转发到终端日志区的消费者
 *
 * 由 spdlog 异步消费线程调用 OnLogLine，因此这里只做一次环形缓冲写入；UI 侧的唤醒
 * 由 onLineAvailable 回调完成（回调同样运行在日志消费线程，实现方须保证线程安全，
 * 典型实现是 App::Post —— FTXUI 文档明确其为线程安全）。
 *
 * 安装时机在 UI 启动之前，所以启动期的日志也会进入缓冲：UI 首帧绘制时即可看到完整
 * 启动历史（"先正常打印 banner，进入 TUI 后清屏重绘"）。
 */
class ConsoleLogConsumer final : public ::mc::application::ILogLineConsumer {
public:
    /**
     * @brief 构造消费者
     * @param buffer 目标环形缓冲（须比本对象存活更久）
     * @param onLineAvailable 每收到一行日志时的唤醒回调（不可为空）
     */
    ConsoleLogConsumer(LogRingBuffer& buffer, std::function<void()> onLineAvailable);

    void OnLogLine(spdlog::level::level_enum level, std::string_view line) override;

    /**
     * @brief 累计转发的行数（线程安全）
     */
    [[nodiscard]] u64 linesForwarded() const noexcept
    {
        return m_linesForwarded.load(std::memory_order_relaxed);
    }

private:
    LogRingBuffer& m_buffer;
    std::function<void()> m_onLineAvailable;
    std::atomic<u64> m_linesForwarded{0};
};

} // namespace mc::server::console
