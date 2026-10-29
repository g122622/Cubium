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
#include <memory>
#include <string>
#include <ftxui/component/component_base.hpp>

namespace mc::server::console {

/**
 * @brief FTXUI 终端会话的驱动
 *
 * 负责：进入全屏备用屏、驱动事件循环（有事件就处理并重绘，空闲时阻塞等待，另以
 * 10 FPS 补帧刷新运行时长等随时间变化的字段）、把跨线程的"有新数据"通知转成唤醒、
 * 退出时恢复终端与光标。
 *
 * 线程模型：实例由 ServerConsole 在**专用 UI 线程**上构造并调用 run()，主线程继续
 * 轮询服务端运行状态。其他线程只通过 thread-safe 的静态接口（requestRedraw /
 * requestExit / copySelection）与本类交互。
 */
class TerminalUi {
public:
    TerminalUi(const TerminalUi&) = delete;
    TerminalUi& operator=(const TerminalUi&) = delete;

    /**
     * @brief 是否允许在本进程启动 TUI
     *
     * 显式要求交互模式时，stdin 与 stdout 必须都是终端；否则调用方报错退出
     * （错误文案与退出码由入口决定）。
     *
     * @param outReason 不允许启动的原因（英文，供日志使用）
     */
    [[nodiscard]] static bool isAvailable(std::string& outReason);

    /**
     * @brief UI 循环是否已被请求退出（任意线程可读）
     */
    [[nodiscard]] static bool exitRequested() noexcept
    {
        return s_exitRequested.load(std::memory_order_acquire);
    }

    /**
     * @brief 在调用线程上阻塞运行 TUI，直到 requestExit() 被调用
     *
     * 必须在专用线程上调用：FTXUI 的 App 只能由一个线程安装/卸载终端。
     *
     * @param component 顶层组件（通常是布局外再包一层快捷键拦截）
     */
    static void run(const std::shared_ptr<ftxui::ComponentBase>& component);

    /**
     * @brief 请求重绘（任意线程可调用）
     *
     * 用于日志/事件/指标有新数据时唤醒 UI；内部做合并，调用频率高也不会拖慢渲染。
     */
    static void requestRedraw();

    /**
     * @brief 请求退出 UI 循环（任意线程可调用）
     */
    static void requestExit();

    /**
     * @brief 把当前终端的选中文本写入系统剪贴板
     *
     * 选择由 FTXUI 内置的鼠标拖选产生；本方法在 UI 线程调用。
     *
     * @return 是否写入了非空文本
     */
    static bool copySelectionToClipboard();

private:
    /// 退出请求标志（requestExit 置位，UI 循环与 ServerConsole::stop 读取）。
    static std::atomic<bool> s_exitRequested;
};

} // namespace mc::server::console
