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

#include "CreateWorldScreen.hpp"

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/util/StringUtils.hpp"
#include "common/world/WorldConfig.hpp"

#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

namespace {

// 可选的游戏模式名称（不含 NotSet）
const char* GAME_MODE_NAMES[] = {"Survival", "Creative", "Adventure", "Spectator"};

// 世界类型名称
const char* WORLD_TYPE_NAMES[] = {"Default", "Flat", "Large Biomes", "Amplified", "Debug"};

// 游戏模式循环数量，排除 NotSet
constexpr i32 GAME_MODE_COUNT = 4;

// 世界类型循环数量
constexpr i32 WORLD_TYPE_COUNT = 5;

} // namespace

CreateWorldScreen::CreateWorldScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "createWorld")
{
    loadTemplateFile("src/client/ui/minecraft/templates/create_world.tpl");
    _cacheWidgets();
    _registerCallbacks();
    _updateGameModeText();
    _updateWorldTypeText();
}

void CreateWorldScreen::onOpen()
{
    TemplateScreen::onOpen();
    if (m_nameField) {
        _focusField(m_nameField);
    }
}

void CreateWorldScreen::_registerCallbacks()
{
    exposeSimpleCallback("onCycleGameMode", [this]() { _cycleGameMode(); });

    exposeSimpleCallback("onCycleWorldType", [this]() { _cycleWorldType(); });

    exposeSimpleCallback("onCreate", [this]() {
        if (_validateInput() && m_onCreate) {
            m_onCreate(buildRequest());
        }
    });

    exposeSimpleCallback("onCancel", [this]() {
        if (m_onCancel) {
            m_onCancel();
        }
    });
}

void CreateWorldScreen::_cacheWidgets()
{
    m_titleText = dynamic_cast<kagero::widget::TextWidget*>(findWidget("title"));
    m_nameLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("nameLabel"));
    m_nameField = dynamic_cast<kagero::widget::TextFieldWidget*>(findWidget("nameField"));
    m_seedLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("seedLabel"));
    m_seedField = dynamic_cast<kagero::widget::TextFieldWidget*>(findWidget("seedField"));
    m_gameModeLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("gameModeLabel"));
    m_gameModeButton = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("gameModeBtn"));
    m_worldTypeLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("worldTypeLabel"));
    m_worldTypeButton = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("worldTypeBtn"));
    m_createButton = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("btn_create"));
    m_cancelButton = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("btn_cancel"));
}

void CreateWorldScreen::_focusField(kagero::widget::TextFieldWidget* field)
{
    if (!field) {
        return;
    }

    // 取消其他输入框的聚焦状态，确保同一时间只有一个输入框获得焦点
    if (m_nameField && m_nameField != field) {
        m_nameField->setFocused(false);
    }
    if (m_seedField && m_seedField != field) {
        m_seedField->setFocused(false);
    }
    field->setFocused(true);
}

void CreateWorldScreen::_cycleGameMode()
{
    i32 mode = static_cast<i32>(m_gameMode);
    mode = (mode + 1) % GAME_MODE_COUNT;
    m_gameMode = static_cast<mc::GameMode>(mode);
    _updateGameModeText();
}

void CreateWorldScreen::_cycleWorldType()
{
    i32 type = static_cast<i32>(m_worldType);
    type = (type + 1) % WORLD_TYPE_COUNT;
    m_worldType = static_cast<mc::WorldType>(type);
    _updateWorldTypeText();
}

void CreateWorldScreen::_updateGameModeText()
{
    if (m_gameModeButton) {
        m_gameModeButton->setText(GAME_MODE_NAMES[static_cast<i32>(m_gameMode)]);
    }
}

void CreateWorldScreen::_updateWorldTypeText()
{
    if (m_worldTypeButton) {
        m_worldTypeButton->setText(WORLD_TYPE_NAMES[static_cast<i32>(m_worldType)]);
    }
}

bool CreateWorldScreen::_validateInput()
{
    return m_nameField != nullptr && !m_nameField->text().empty();
}

world::storage::CreateWorldRequest CreateWorldScreen::buildRequest() const
{
    u64 seed = 0;
    if (m_seedField && !m_seedField->text().empty()) {
        const std::string seedText = m_seedField->text();
        if (util::isNumeric(seedText, true)) {
            try {
                seed = std::stoull(seedText);
            }
            catch (...) {
                seed = 0;
            }
        } else {
            // 非数字种子：使用哈希值作为种子
            seed = std::hash<std::string>{}(seedText);
        }
    }

    // TODO: 视距、难度、是否允许作弊等选项目前使用硬编码默认值，后续应提供界面让用户配置
    return world::storage::CreateWorldRequest(m_nameField ? m_nameField->text() : "New World",
        "",
        seed,
        m_worldType,
        m_gameMode,
        mc::Difficulty::Normal,
        false,
        m_allowCommands,
        defaults::client::renderDistance);
}

bool CreateWorldScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    // ESC 键取消创建
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onCancel) {
            m_onCancel();
        }
        return true;
    }

    // Tab 键在输入框之间切换焦点
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        if (m_nameField && m_nameField->isFocused()) {
            _focusField(m_seedField);
        } else if (m_seedField && m_seedField->isFocused()) {
            _focusField(m_nameField);
        }
        return true;
    }

    // 将键盘事件转发给当前聚焦的输入框
    if (m_nameField && m_nameField->isFocused()) {
        return m_nameField->onKey(key, scanCode, action, mods);
    }
    if (m_seedField && m_seedField->isFocused()) {
        return m_seedField->onKey(key, scanCode, action, mods);
    }

    return Screen::onKey(key, scanCode, action, mods);
}

bool CreateWorldScreen::onChar(u32 codePoint)
{
    // 将字符输入事件转发给当前聚焦的输入框
    if (m_nameField && m_nameField->isFocused()) {
        return m_nameField->onChar(codePoint);
    }
    if (m_seedField && m_seedField->isFocused()) {
        return m_seedField->onChar(codePoint);
    }

    return Screen::onChar(codePoint);
}

} // namespace mc::client::ui::minecraft
