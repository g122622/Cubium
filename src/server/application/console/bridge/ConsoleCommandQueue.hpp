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

namespace mc::server::console {

/**
 * @brief UI 线程 → tick 线程的控制台命令队列
 *
 * 命令回调会直接改写世界状态，只能在 tick 线程执行（post-tick 回调里 drain 本队列）。
 * UI 线程提交后立即返回，结果由 tick 线程写回 EventRingBuffer。
 */
class ConsoleCommandQueue {
public:
    /**
     * @brief 提交一条命令（UI 线程调用，线程安全）
     * @param command 不含前导 '/' 的命令文本
     */
    void submit(std::string command);

    /**
     * @brief 取出全部待执行命令并清空队列（tick 线程调用）
     * @return 按提交顺序排列的命令
     */
    [[nodiscard]] std::vector<std::string> drain();

    /**
     * @brief 当前待执行命令数（线程安全，供 UI 显示）
     */
    [[nodiscard]] usize pendingCount() const;

private:
    mutable std::mutex m_mutex;
    std::deque<std::string> m_pending;
};

} // namespace mc::server::console
