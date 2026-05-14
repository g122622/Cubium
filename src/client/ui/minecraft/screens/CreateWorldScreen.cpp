#include "CreateWorldScreen.hpp"
#include "../../kagero/event/EventBus.hpp"
#include "../../kagero/state/StateStore.hpp"
#include "common/command/StringReader.hpp"
#include "common/core/Types.hpp"
#include "common/util/StringUtils.hpp"
#include "common/world/WorldConfig.hpp"
#include <algorithm>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {

static const char* GAME_MODE_NAMES[] = {"Survival", "Creative", "Adventure", "Spectator"};

static const char* WORLD_TYPE_NAMES[] = {"Default", "Flat", "Large Biomes", "Amplified", "Debug"};

CreateWorldScreen::CreateWorldScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "createWorld")
{
    loadTemplateFile("src/client/ui/minecraft/templates/create_world.tpl");
    cacheWidgets();
    registerCallbacks();
    updateGameModeText();
    updateWorldTypeText();
}

void CreateWorldScreen::onOpen()
{
    TemplateScreen::onOpen();
    if (m_nameField) {
        focusField(m_nameField);
    }
}

void CreateWorldScreen::registerCallbacks()
{
    exposeSimpleCallback("onCycleGameMode", [this]() { cycleGameMode(); });

    exposeSimpleCallback("onCycleWorldType", [this]() { cycleWorldType(); });

    exposeSimpleCallback("onCreate", [this]() {
        if (validateInput() && m_onCreate) {
            m_onCreate(buildRequest());
        }
    });

    exposeSimpleCallback("onCancel", [this]() {
        if (m_onCancel) {
            m_onCancel();
        }
    });
}

void CreateWorldScreen::cacheWidgets()
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

void CreateWorldScreen::focusField(kagero::widget::TextFieldWidget* field)
{
    if (!field) {
        return;
    }

    if (m_nameField && m_nameField != field) {
        m_nameField->setFocused(false);
    }
    if (m_seedField && m_seedField != field) {
        m_seedField->setFocused(false);
    }
    field->setFocused(true);
}

void CreateWorldScreen::cycleGameMode()
{
    i32 mode = static_cast<i32>(m_gameMode);
    mode = (mode + 1) % 4;
    m_gameMode = static_cast<mc::GameMode>(mode);
    updateGameModeText();
}

void CreateWorldScreen::cycleWorldType()
{
    i32 type = static_cast<i32>(m_worldType);
    type = (type + 1) % 5;
    m_worldType = static_cast<mc::WorldType>(type);
    updateWorldTypeText();
}

void CreateWorldScreen::updateGameModeText()
{
    if (m_gameModeButton) {
        m_gameModeButton->setText(GAME_MODE_NAMES[static_cast<i32>(m_gameMode)]);
    }
}

void CreateWorldScreen::updateWorldTypeText()
{
    if (m_worldTypeButton) {
        m_worldTypeButton->setText(WORLD_TYPE_NAMES[static_cast<i32>(m_worldType)]);
    }
}

bool CreateWorldScreen::validateInput()
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
            seed = std::hash<std::string>{}(seedText);
        }
    }

    return world::storage::CreateWorldRequest(m_nameField ? m_nameField->text() : "New World",
        "",
        seed,
        m_worldType,
        m_gameMode,
        mc::Difficulty::Normal,
        false,
        m_allowCommands,
        12);
}

bool CreateWorldScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onCancel) {
            m_onCancel();
        }
        return true;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        if (m_nameField && m_nameField->isFocused()) {
            focusField(m_seedField);
        } else if (m_seedField && m_seedField->isFocused()) {
            focusField(m_nameField);
        }
        return true;
    }

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
    if (m_nameField && m_nameField->isFocused()) {
        return m_nameField->onChar(codePoint);
    }
    if (m_seedField && m_seedField->isFocused()) {
        return m_seedField->onChar(codePoint);
    }

    return Screen::onChar(codePoint);
}

} // namespace mc::client::ui::minecraft
