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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../core/Types.hpp"
#include <functional>
#include <string>

namespace mc::assert {

/**
 * @brief 崩溃清理回调
 *
 * 在崩溃信息输出之后、进程退出之前调用。
 * 用于执行紧急清理操作（如刷新跟踪数据、关闭文件等）。
 * 回调中不能使用任何可能死锁的操作（如获取互斥锁、分配内存等）。
 */
using CrashCleanupCallback = std::function<void()>;

/**
 * @brief 崩溃处理器
 *
 * 在程序崩溃时（空指针解引用、除零、栈溢出等）捕获异常，
 * 输出详细的调用栈信息（函数名、源文件、行号）到 stderr。
 *
 * 工作原理：
 * - Windows：注册 SetUnhandledExceptionFilter + 纯虚函数调用处理器 + std::terminate 处理器
 * - Linux/macOS：注册 SIGSEGV/SIGABRT/SIGFPE/SIGBUS 等信号处理器 + std::terminate 处理器
 *
 * 前提条件：
 * - RelWithDebInfo 或 Debug 构建模式（PDB/调试符号可用）
 * - Windows 上需要 dbghelp.lib（已自动链接）
 *
 * 局部变量说明：
 * - RelWithDebInfo 下部分局部变量会被编译器优化掉，无法保证所有变量都能枚举
 * - 每个栈帧会尽可能尝试输出局部变量名和类型，但信息可能不完整
 */
class CrashHandler {
public:
    /**
     * @brief 安装崩溃处理器
     *
     * 在程序启动时调用一次即可。会注册所有必要的信号处理器和异常过滤器。
     * 多次调用是安全的（只会安装一次）。
     */
    static void install();

    /**
     * @brief 卸载崩溃处理器
     *
     * 恢复默认的信号处理器和异常过滤器。
     * 通常不需要手动调用，仅在特殊场景下使用。
     */
    static void uninstall();

    /**
     * @brief 注册崩溃清理回调
     *
     * 在崩溃信息输出之后、进程退出之前调用。
     * 可用于刷新跟踪数据（如 Perfetto）、关闭文件等紧急操作。
     *
     * @param callback 清理回调函数，不能使用可能死锁的操作
     */
    static void setCleanupCallback(CrashCleanupCallback callback);

    /**
     * @brief 捕获当前线程的调用栈
     *
     * @param skipFrames 跳过最顶部的帧数（默认跳过 captureStackTrace 自身）
     * @param maxFrames 最大捕获帧数（默认 64）
     * @return 格式化的调用栈字符串
     */
    static std::string captureStackTrace(i32 skipFrames = 1, i32 maxFrames = 64);

    /**
     * @brief 是否已安装
     */
    static bool isInstalled();

    // 内部使用的静态成员（需要被平台特定的匿名命名空间回调访问）
    static CrashCleanupCallback s_cleanupCallback;

private:
    CrashHandler() = delete;

    static bool s_installed;
};

} // namespace mc::assert
