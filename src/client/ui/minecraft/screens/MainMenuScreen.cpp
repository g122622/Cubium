#include "MainMenuScreen.hpp"
#include "../../kagero/event/EventBus.hpp"
#include "../../kagero/state/StateStore.hpp"
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

MainMenuScreen::MainMenuScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "mainMenu")
{
    loadTemplateFile("src/client/ui/minecraft/templates/main_menu.tpl");
    registerCallbacks();
}

void MainMenuScreen::registerCallbacks()
{
    exposeSimpleCallback("onSinglePlayer", [this]() {
        if (m_onSinglePlayer) {
            m_onSinglePlayer();
        }
    });

    exposeSimpleCallback("onMultiPlayer", [this]() {
        if (m_onMultiPlayer) {
            m_onMultiPlayer();
        }
    });

    exposeSimpleCallback("onOptions", [this]() {
        if (m_onOptions) {
            m_onOptions();
        }
    });

    exposeSimpleCallback("onQuit", [this]() {
        if (m_onQuit) {
            m_onQuit();
        }
    });
}

bool MainMenuScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onQuit) {
            m_onQuit();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

} // namespace mc::client::ui::minecraft
