/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "OptionsScreen.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/ui/minecraft/screens/TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

namespace {

/// 难度枚举到显示名的映射（对齐 Java 难度按钮文案）
[[nodiscard]] std::string_view difficultyName(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::Peaceful:
            return "Peaceful";
        case Difficulty::Easy:
            return "Easy";
        case Difficulty::Normal:
            return "Normal";
        case Difficulty::Hard:
            return "Hard";
        default:
            return "Normal";
    }
}

} // namespace

OptionsScreen::OptionsScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "options")
{
    loadTemplateFile("src/client/ui/minecraft/templates/options.tpl");
    _registerCallbacks();
}

bool OptionsScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onClose) {
            m_onClose();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

void OptionsScreen::setDifficulty(Difficulty difficulty)
{
    m_difficulty = difficulty;
    _refreshDifficultyButton();
}

void OptionsScreen::setDifficultyLocked(bool locked)
{
    m_difficultyLocked = locked;
    _refreshDifficultyButton();
    // 锁定按钮自身：锁定后禁用（对齐 Java 锁定后两按钮均 inactive）
    if (auto* lockBtn = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("lockDifficulty"))) {
        lockBtn->setActive(m_difficultyControlsEnabled && !locked);
    }
}

void OptionsScreen::setDifficultyControlsEnabled(bool enabled)
{
    m_difficultyControlsEnabled = enabled;
    _refreshDifficultyButton();
    if (auto* lockBtn = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("lockDifficulty"))) {
        lockBtn->setActive(enabled && !m_difficultyLocked);
    }
}

void OptionsScreen::_refreshDifficultyButton()
{
    auto* btn = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("difficulty"));
    if (btn == nullptr) {
        // 模板尚未实例化（构造期/未 onOpen），状态已缓存，待后续刷新。
        return;
    }
    btn->setText("Difficulty: " + std::string(difficultyName(m_difficulty)));
    // 难度按钮启用条件：控件启用 且 未锁定
    btn->setActive(m_difficultyControlsEnabled && !m_difficultyLocked);
}

void OptionsScreen::_registerCallbacks()
{
    exposeSimpleCallback("onClose", [this]() {
        if (m_onClose) {
            m_onClose();
        }
    });
    exposeSimpleCallback("onCycleDifficulty", [this]() {
        // 难度被锁定或控件禁用时按钮已 inactive，不会触发；仍防御性校验。
        if (m_difficultyLocked || !m_difficultyControlsEnabled) {
            return;
        }
        // 本地推进到下一难度并刷新文案，再通知调用方发包（对齐 Java 客户端立即循环并发包）。
        const i32 next = (static_cast<i32>(m_difficulty) + 1) % 4;
        setDifficulty(static_cast<Difficulty>(next));
        if (m_onCycleDifficulty) {
            m_onCycleDifficulty(m_difficulty);
        }
    });
    exposeSimpleCallback("onLockDifficulty", [this]() {
        if (!m_difficultyControlsEnabled || m_difficultyLocked) {
            return;
        }
        // 锁定不可逆（对齐 Java 仅发 locked=true），本地立即置锁定状态。
        setDifficultyLocked(true);
        if (m_onLockDifficulty) {
            m_onLockDifficulty();
        }
    });
}

} // namespace mc::client::ui::minecraft
