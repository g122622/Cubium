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

#include "../../kagero/widget/ButtonWidget.hpp"
#include "../../kagero/widget/TextFieldWidget.hpp"
#include "../../kagero/widget/TextWidget.hpp"
#include "TemplateScreen.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include <functional>
#include <memory>

namespace mc::client::ui::minecraft {

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
    void registerCallbacks();
    void cacheWidgets();
    void cycleGameMode();
    void cycleWorldType();
    void updateGameModeText();
    void updateWorldTypeText();
    bool validateInput();
    void focusField(kagero::widget::TextFieldWidget* field);

    kagero::widget::TextWidget* m_titleText = nullptr;
    kagero::widget::TextWidget* m_nameLabel = nullptr;
    kagero::widget::TextFieldWidget* m_nameField = nullptr;
    kagero::widget::TextWidget* m_seedLabel = nullptr;
    kagero::widget::TextFieldWidget* m_seedField = nullptr;
    kagero::widget::TextWidget* m_gameModeLabel = nullptr;
    kagero::widget::ButtonWidget* m_gameModeButton = nullptr;
    kagero::widget::TextWidget* m_worldTypeLabel = nullptr;
    kagero::widget::ButtonWidget* m_worldTypeButton = nullptr;
    kagero::widget::ButtonWidget* m_createButton = nullptr;
    kagero::widget::ButtonWidget* m_cancelButton = nullptr;

    mc::GameMode m_gameMode = mc::GameMode::Survival;
    mc::WorldType m_worldType = mc::WorldType::Default;
    bool m_allowCommands = false;

    CreateCallback m_onCreate;
    Callback m_onCancel;
};

} // namespace mc::client::ui::minecraft
