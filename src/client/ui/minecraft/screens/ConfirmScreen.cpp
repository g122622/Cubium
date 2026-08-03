/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the further conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ConfirmScreen.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/ui/minecraft/screens/TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <utility>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {

ConfirmScreen::ConfirmScreen(const std::string& title,
    const std::string& message,
    const std::string& confirmText,
    const std::string& cancelText,
    ConfirmCallback callback)
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "confirm")
    , m_title(title)
    , m_message(message)
    , m_confirmText(confirmText)
    , m_cancelText(cancelText)
    , m_callback(std::move(callback))
{
    setPauseScreen(true);

    // 暴露绑定值
    expose("confirm.title", &m_title);
    expose("confirm.message", &m_message);
    expose("confirm.yesText", &m_confirmText);
    expose("confirm.noText", &m_cancelText);

    if (!loadTemplateFile("src/client/ui/minecraft/templates/confirm_dialog.tpl")) {
        spdlog::error("[ConfirmScreen] Failed to load confirm_dialog.tpl template");
    }
    _registerCallbacks();
}

void ConfirmScreen::_registerCallbacks()
{
    exposeSimpleCallback("onConfirm", [this]() {
        if (m_callback) {
            m_callback(true);
        }
    });

    exposeSimpleCallback("onCancel", [this]() {
        if (m_callback) {
            m_callback(false);
        }
    });
}

bool ConfirmScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        // ESC 键视为取消
        if (m_callback) {
            m_callback(false);
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

} // namespace mc::client::ui::minecraft
