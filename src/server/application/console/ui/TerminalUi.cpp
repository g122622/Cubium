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

#include "TerminalUi.hpp"

#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace mc::server::console {

using namespace mc::trace;

std::atomic<bool> TerminalUi::s_exitRequested{false};

namespace {

#if defined(_WIN32)

/**
 * @brief 把 UTF-8 文本写入 Windows 剪贴板
 *
 * 使用 CF_UNICODETEXT（UTF-16）。非 Windows 平台返回 false（交由调用方提示）。
 */
bool _writeClipboardUtf8(std::string_view text)
{
    if (text.empty()) {
        return false;
    }

    const int wcharCount = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (wcharCount <= 0) {
        return false;
    }
    std::wstring wide(static_cast<usize>(wcharCount), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), wcharCount);

    if (OpenClipboard(nullptr) == FALSE) {
        return false;
    }

    const bool opened = EmptyClipboard() != FALSE;
    bool copied = false;
    if (opened) {
        const usize bytes = (wide.size() + 1) * sizeof(wchar_t);
        HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (handle != nullptr) {
            void* target = GlobalLock(handle);
            if (target != nullptr) {
                std::memcpy(target, wide.c_str(), bytes);
                GlobalUnlock(handle);
                copied = SetClipboardData(CF_UNICODETEXT, handle) != nullptr;
            }
            if (!copied) {
                GlobalFree(handle);
            }
        }
    }

    CloseClipboard();
    return copied;
}

#else

/// 非 Windows 平台暂未实现剪贴板写入；终端原生选择（Shift+拖选）仍可用。
bool _writeClipboardUtf8(std::string_view /*text*/)
{
    return false;
}

#endif

} // namespace

bool TerminalUi::isAvailable(std::string& outReason)
{
#if defined(_WIN32)
    const bool stdinTty = _isatty(_fileno(stdin)) != 0;
    const bool stdoutTty = _isatty(_fileno(stdout)) != 0;
    const DWORD stdinMode = []() -> DWORD {
        DWORD mode = 0;
        return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != FALSE ? mode : 0;
    }();
    // 标准流重定向成管道时 _isatty 已为假；这里再挡一层"输入被重定向到管道/文件"的
    // 控制台句柄（典型场景：`echo x | minecraft-server --interactive`）。
    if (stdinMode != 0) {
        constexpr DWORD PIPED_INPUT = 0x0002;     // ENABLE_LINE_INPUT
        constexpr DWORD PROCESSED_INPUT = 0x0001; // ENABLE_PROCESSED_INPUT
        if ((stdinMode & (PIPED_INPUT | PROCESSED_INPUT)) == 0) {
            outReason = "stdin console handle is not an interactive terminal";
            return false;
        }
    }
#else
    const bool stdinTty = isatty(fileno(stdin)) != 0;
    const bool stdoutTty = isatty(fileno(stdout)) != 0;
#endif

    if (!stdoutTty) {
        outReason = "stdout is not a terminal";
        return false;
    }
    if (!stdinTty) {
        outReason = "stdin is not a terminal";
        return false;
    }
    return true;
}

void TerminalUi::run(const std::shared_ptr<ftxui::ComponentBase>& component)
{
    MC_ASSERT_RELEASE_MSG(component != nullptr, "TerminalUi::run requires a component");

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "TerminalUi::run");
    // 让 trace 里的 UI 线程自带名字，便于在 Perfetto 中与其他线程区分。
    MC_TRACE_SET_THREAD_NAME("ServerConsoleUi");

    // Fullscreen 使用备用屏：进入时清屏，退出时原样恢复启动阶段打印的 banner 与日志。
    ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();
    // Ctrl+C 由我们自己的按键处理（CommandLineComponent 转发为 requestExit），不要让
    // FTXUI 在组件未消费时把它转成 SIGINT（那会与本服务端的信号处理互相覆盖）。
    screen.ForceHandleCtrlC(false);

    // UI 线程常驻循环：每帧处理事件并重绘。用长期作用域事件包住整个循环，trace 中
    // 可直接看到 UI 线程的生命周期；帧内细分见下方 RunOnce / 空闲等待。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "TerminalUi::loop");

    ftxui::Loop loop(&screen, component);

    // 有数据变化（日志/事件/命令回执/指标）时，由其他线程 Post(Event::Custom) 唤醒。
    // 没有唤醒就静止等待（RunOnceBlocking 在信号量上阻塞），不空转重绘：
    // 周期性重绘会产生持续的整屏 ANSI 输出，在没有变化的终端里表现为"闪烁"。
    //
    // 定时重绘只用于随时间自然变化的内容（运行时长、TPS 平滑值），约 10 FPS 足矣；
    // 它在 RunOnceBlocking 返回后检查一次是否到点，因此不会带来额外的唤醒。
    constexpr auto ANIMATION_INTERVAL = std::chrono::milliseconds(100);
    auto lastAnimated = std::chrono::steady_clock::now();

    u64 frames = 0;
    while (!loop.HasQuitted()) {
        {
            // RunOnceBlocking 处理已到达的全部任务与事件（含跨线程 Post 的重绘请求），
            // 并整屏重绘；没有事件时在这里睡眠，不占 CPU。
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Console, "TerminalUi::frame");
            loop.RunOnceBlocking();
        }
        ++frames;

        // 退出请求可能来自其他线程（/stop、服务端停止、Ctrl+Q）。
        if (s_exitRequested.load(std::memory_order_acquire)) {
            screen.Exit();
            break;
        }

        // 到点补一帧：运行时长与 TPS 平滑值等"无事件但仍会变"的内容。
        const auto now = std::chrono::steady_clock::now();
        if (now - lastAnimated >= ANIMATION_INTERVAL) {
            lastAnimated = now;
            screen.PostEvent(ftxui::Event::Custom);
        }
    }

    MC_TRACE_COUNTER(TraceEvents.Server.Console, "ConsoleUiFrames", static_cast<i64>(frames));

    // Loop/屏幕析构时卸载终端：恢复控制台模式、鼠标追踪与光标。
}

void TerminalUi::requestRedraw()
{
    auto* screen = ftxui::ScreenInteractive::Active();
    if (screen == nullptr) {
        return;
    }
    // PostEvent 线程安全：投递一个自定义事件，把阻塞在 RunOnceBlocking 里的 UI 线程唤醒。
    // 事件本身会走一遍组件链（无组件消费 Custom），这一帧随后整屏重绘。
    screen->PostEvent(ftxui::Event::Custom);
}

void TerminalUi::requestExit()
{
    s_exitRequested.store(true, std::memory_order_release);

    auto* screen = ftxui::ScreenInteractive::Active();
    if (screen == nullptr) {
        return;
    }
    // Exit() 内部是 Post(ExitNow)：投递到 UI 线程执行，异步生效。这样关服流程里的
    // "先记一条日志，再停终端"能正常执行，而不是在调用方线程里直接抛出退出。
    screen->Exit();
}

bool TerminalUi::copySelectionToClipboard()
{
    auto* screen = ftxui::ScreenInteractive::Active();
    if (screen == nullptr) {
        return false;
    }

    const std::string selection = screen->GetSelection();
    return _writeClipboardUtf8(selection);
}

} // namespace mc::server::console
