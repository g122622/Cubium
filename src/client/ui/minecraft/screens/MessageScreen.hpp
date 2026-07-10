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

#include "TemplateScreen.hpp"
#include <functional>
#include <string>

namespace mc::client::ui::minecraft {

/**
 * @brief 通用单按钮通知对话框
 *
 * 对应 MC Java 的 AlertScreen，显示标题、消息和一个 OK 按钮。
 * 用户点击 OK 按钮或按 ESC 时调用回调（不区分两种来源）。
 *
 * 与 ConfirmScreen 的区别：
 * - ConfirmScreen 是双按钮（确认/取消），回调带 bool 参数
 * - MessageScreen 是单按钮（OK），回调无参数
 *
 * 典型用途：操作失败提示、错误通知、不可恢复操作的提示等。
 */
class MessageScreen : public TemplateScreen {
public:
    /**
     * @brief OK 按钮回调类型
     *
     * 用户点击 OK 按钮或按 ESC 关闭时调用，无参数。
     */
    using OkCallback = std::function<void()>;

    /**
     * @brief 构造通知对话框
     *
     * @param title 标题文本
     * @param message 消息文本（可包含具体错误信息等详细内容）
     * @param okText OK 按钮文本（如 "OK"、"Back"、"确认" 等）
     * @param callback 用户点击 OK 按钮或按 ESC 关闭后的回调（可为空）
     */
    MessageScreen(const std::string& title, const std::string& message, const std::string& okText, OkCallback callback);

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void _registerCallbacks();

    std::string m_title;
    std::string m_message;
    std::string m_okText;
    OkCallback m_callback;
};

} // namespace mc::client::ui::minecraft
