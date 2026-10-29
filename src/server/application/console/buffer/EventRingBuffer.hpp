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
#include <string_view>
#include <vector>

namespace mc::server::console {

/**
 * @brief 终端事件分类
 *
 * 分类决定事件在终端中的着色与标记，不参与过滤（当前所有分类均展示）。
 */
enum class TerminalEventKind : u8 {
    Info = 0,    ///< 一般信息（服务器状态等）
    Chat = 1,    ///< 玩家聊天
    Player = 2,  ///< 玩家加入/离开
    Command = 3, ///< 控制台命令回执（成功）
    Error = 4,   ///< 命令失败/错误
    Game = 5,    ///< 业务事件（死亡、进度、天气等）
};

/**
 * @brief 终端事件
 */
struct TerminalEvent {
    TerminalEventKind kind = TerminalEventKind::Info; ///< 分类（决定着色）
    std::string text;                                 ///< 展示文本（已去除 § 样式码，不含换行）
};

/**
 * @brief 线程安全的定长事件环形缓冲
 *
 * 命令结果在 tick 线程写入，玩家/业务事件在事件总线发布线程写入，UI 线程读取。
 * 容量满时丢弃最旧事件。
 */
class EventRingBuffer {
public:
    /**
     * @brief 构造缓冲
     * @param capacity 保留的最大事件数（必须大于 0）
     */
    explicit EventRingBuffer(usize capacity);

    /**
     * @brief 追加一条事件（线程安全）
     * @param kind 分类
     * @param text 展示文本
     */
    void push(TerminalEventKind kind, std::string text);

    /**
     * @brief 拷贝当前全部事件（线程安全）
     * @return 由旧到新排列的事件副本
     */
    [[nodiscard]] std::vector<TerminalEvent> snapshot() const;

    /**
     * @brief 当前保存的事件数（线程安全）
     */
    [[nodiscard]] usize size() const;

    /**
     * @brief 累计接收过的事件数（线程安全），供 UI 判断是否有新事件
     */
    [[nodiscard]] u64 pushedCount() const;

private:
    const usize m_capacity;
    mutable std::mutex m_mutex;
    std::deque<TerminalEvent> m_events;
    u64 m_pushedCount = 0;
};

/**
 * @brief 去除 Minecraft § 样式码
 *
 * 聊天、命令回执等文本可能带 §c 之类的颜色码，纯文本终端必须剥离，否则会显示为乱码。
 *
 * @param text 带样式码的文本
 * @return 纯文本（仅保留可见字符）
 */
[[nodiscard]] std::string stripFormattingCodes(std::string_view text);

} // namespace mc::server::console
