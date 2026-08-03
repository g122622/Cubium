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
 * LIABILITY, WHETHER IN THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <string>

namespace mc::client::ui::minecraft {

/**
 * @brief 通用确认对话框
 *
 * 对应 MC Java 的 ConfirmScreen，显示标题、消息和两个按钮（确认/取消）。
 * 用户点击确认按钮时回调传入 true，点击取消按钮或按 ESC 时传入 false。
 *
 * 典型用途：删除世界确认、退出游戏确认等危险操作。
 */
class ConfirmScreen : public TemplateScreen {
public:
    /**
     * @brief 确认结果回调类型
     *
     * 参数为 true 表示用户确认操作，false 表示取消。
     */
    using ConfirmCallback = std::function<void(bool)>;

    /**
     * @brief 构造确认对话框
     *
     * @param title 标题文本
     * @param message 消息文本（可包含具体名称等详细信息）
     * @param confirmText 确认按钮文本（如 "Delete"）
     * @param cancelText 取消按钮文本（如 "Cancel"）
     * @param callback 用户选择后的回调，true=确认，false=取消
     */
    ConfirmScreen(const std::string& title,
        const std::string& message,
        const std::string& confirmText,
        const std::string& cancelText,
        ConfirmCallback callback);

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void _registerCallbacks();

    std::string m_title;
    std::string m_message;
    std::string m_confirmText;
    std::string m_cancelText;
    ConfirmCallback m_callback;
};

} // namespace mc::client::ui::minecraft
