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
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/CheckboxWidget.hpp"
#include "client/ui/kagero/widget/SliderWidget.hpp"
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "client/ui/kagero/widget/TextWidget.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace mc::client::ui::minecraft {

namespace test {
class CreateWorldScreenTestAccessor;
} // namespace test

/**
 * @brief 创建世界界面
 *
 * 提供世界名称、种子、游戏模式、世界类型、难度、是否允许作弊、视距等配置选项，
 * 用于创建新的游戏世界。
 */
class CreateWorldScreen : public TemplateScreen {
public:
    using Callback = std::function<void()>;
    using CreateCallback = std::function<void(const world::storage::CreateWorldRequest&)>;

    CreateWorldScreen();

    void onOpen() override;
    void setOnCreate(CreateCallback callback) { m_onCreate = std::move(callback); }
    void setOnCancel(Callback callback) { m_onCancel = std::move(callback); }

    [[nodiscard]] world::storage::CreateWorldRequest buildRequest() const;

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;
    bool onChar(u32 codePoint) override;

private:
    void _registerCallbacks();
    void _cacheWidgets();
    void _cycleGameMode();
    void _cycleWorldType();
    void _cycleDifficulty();
    void _updateGameModeText();
    void _updateWorldTypeText();
    void _updateDifficultyText();
    void _toggleAllowCommands();
    void _onViewDistanceChanged();
    bool _validateInput();
    void _focusField(kagero::widget::TextFieldWidget* field);

    // 界面控件
    kagero::widget::TextWidget* m_titleText = nullptr;
    kagero::widget::TextWidget* m_nameLabel = nullptr;
    kagero::widget::TextFieldWidget* m_nameField = nullptr;
    kagero::widget::TextWidget* m_seedLabel = nullptr;
    kagero::widget::TextFieldWidget* m_seedField = nullptr;
    kagero::widget::TextWidget* m_gameModeLabel = nullptr;
    kagero::widget::ButtonWidget* m_gameModeButton = nullptr;
    kagero::widget::TextWidget* m_worldTypeLabel = nullptr;
    kagero::widget::ButtonWidget* m_worldTypeButton = nullptr;
    kagero::widget::TextWidget* m_difficultyLabel = nullptr;
    kagero::widget::ButtonWidget* m_difficultyButton = nullptr;
    kagero::widget::TextWidget* m_allowCommandsLabel = nullptr;
    kagero::widget::CheckboxWidget* m_allowCommandsCheckbox = nullptr;
    kagero::widget::TextWidget* m_viewDistanceLabel = nullptr;
    kagero::widget::SliderWidget* m_viewDistanceSlider = nullptr;
    kagero::widget::ButtonWidget* m_createButton = nullptr;
    kagero::widget::ButtonWidget* m_cancelButton = nullptr;

    // 世界配置状态
    mc::GameMode m_gameMode = mc::GameMode::Survival;
    mc::WorldType m_worldType = mc::WorldType::Default;
    mc::Difficulty m_difficulty = mc::Difficulty::Normal;
    bool m_allowCommands = false;
    i32 m_viewDistance = defaults::client::renderDistance;

    // 回调
    CreateCallback m_onCreate;
    Callback m_onCancel;

    friend class test::CreateWorldScreenTestAccessor;
};

} // namespace mc::client::ui::minecraft
