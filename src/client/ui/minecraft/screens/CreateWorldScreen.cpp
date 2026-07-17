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

// 难度名称
const char* DIFFICULTY_NAMES[] = {"Peaceful", "Easy", "Normal", "Hard"};

// 游戏模式循环数量，排除 NotSet
constexpr i32 GAME_MODE_COUNT = 4;

// 世界类型循环数量
constexpr i32 WORLD_TYPE_COUNT = 5;

// 难度循环数量
constexpr i32 DIFFICULTY_COUNT = 4;

// 视距滑块的最小/最大值
constexpr i32 VIEW_DISTANCE_MIN = 3;
constexpr i32 VIEW_DISTANCE_MAX = 32;

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
    _updateDifficultyText();
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

    exposeSimpleCallback("onCycleDifficulty", [this]() { _cycleDifficulty(); });

    exposeSimpleCallback("onToggleAllowCommands", [this]() { _toggleAllowCommands(); });

    exposeSimpleCallback("onViewDistanceChanged", [this]() { _onViewDistanceChanged(); });

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

    // 滑块显示配置：整数步长 + 整数格式化，让滑块上直接显示当前视距值
    // 事件回调通过模板的 on:change 绑定到 onViewDistanceChanged，无需在此处 setOnValueChanged
    if (m_viewDistanceSlider) {
        m_viewDistanceSlider->setStepSize(1.0);
        m_viewDistanceSlider->setFormatCallback([](f64 value) { return std::to_string(static_cast<i32>(value)); });
    }
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
    m_difficultyLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("difficultyLabel"));
    m_difficultyButton = dynamic_cast<kagero::widget::ButtonWidget*>(findWidget("difficultyBtn"));
    m_allowCommandsLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("allowCommandsLabel"));
    m_allowCommandsCheckbox = dynamic_cast<kagero::widget::CheckboxWidget*>(findWidget("allowCommandsCheck"));
    m_viewDistanceLabel = dynamic_cast<kagero::widget::TextWidget*>(findWidget("viewDistanceLabel"));
    m_viewDistanceSlider = dynamic_cast<kagero::widget::SliderWidget*>(findWidget("viewDistanceSlider"));
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

void CreateWorldScreen::_cycleDifficulty()
{
    i32 difficulty = static_cast<i32>(m_difficulty);
    difficulty = (difficulty + 1) % DIFFICULTY_COUNT;
    m_difficulty = static_cast<mc::Difficulty>(difficulty);
    _updateDifficultyText();
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

void CreateWorldScreen::_updateDifficultyText()
{
    if (m_difficultyButton) {
        m_difficultyButton->setText(DIFFICULTY_NAMES[static_cast<i32>(m_difficulty)]);
    }
}

void CreateWorldScreen::_toggleAllowCommands()
{
    // 由模板的 on:change 事件回调触发，复选框状态已由 CheckboxWidget 内部更新
    if (m_allowCommandsCheckbox) {
        m_allowCommands = m_allowCommandsCheckbox->isChecked();
    }
}

void CreateWorldScreen::_onViewDistanceChanged()
{
    // 由模板的 on:change 事件回调触发，滑块值已由 SliderWidget 内部更新
    if (m_viewDistanceSlider) {
        m_viewDistance = static_cast<i32>(m_viewDistanceSlider->value());
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

    // 限制视距到滑块范围，避免极端值
    i32 viewDistance = m_viewDistance;
    if (viewDistance < VIEW_DISTANCE_MIN) {
        viewDistance = VIEW_DISTANCE_MIN;
    } else if (viewDistance > VIEW_DISTANCE_MAX) {
        viewDistance = VIEW_DISTANCE_MAX;
    }

    // worldPresetId 按 UI 选定的 WorldType 推导（对齐原版 world_preset 资源位置）
    resource::ResourceLocation worldPresetId = [worldType = m_worldType]() -> resource::ResourceLocation {
        switch (worldType) {
            case WorldType::Flat:
                return resource::ResourceLocation("minecraft", "flat");
            case WorldType::LargeBiomes:
                return resource::ResourceLocation("minecraft", "large_biomes");
            case WorldType::Amplified:
                return resource::ResourceLocation("minecraft", "amplified");
            case WorldType::Debug:
                return resource::ResourceLocation("minecraft", "debug_all_block_states");
            case WorldType::Default:
            default:
                return resource::ResourceLocation("minecraft", "default");
        }
    }();

    return world::storage::CreateWorldRequest(m_nameField ? m_nameField->text() : "New World",
        "",
        seed,
        m_worldType,
        std::move(worldPresetId),
        m_gameMode,
        m_difficulty,
        false,
        m_allowCommands,
        viewDistance);
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
