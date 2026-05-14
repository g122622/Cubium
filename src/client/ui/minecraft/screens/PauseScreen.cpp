#include "PauseScreen.hpp"
#include "../../kagero/event/EventBus.hpp"
#include "../../kagero/state/StateStore.hpp"
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

PauseScreen::PauseScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "pause")
{
    loadTemplateFile("src/client/ui/minecraft/templates/pause_menu.tpl");
    registerCallbacks();
}

void PauseScreen::registerCallbacks()
{
    exposeSimpleCallback("onResume", [this]() {
        if (m_onResume) {
            m_onResume();
        }
    });

    exposeSimpleCallback("onOptions", [this]() {
        if (m_onOptions) {
            m_onOptions();
        }
    });

    exposeSimpleCallback("onSaveAndQuit", [this]() {
        if (m_onSaveAndQuit) {
            m_onSaveAndQuit();
        }
    });
}

bool PauseScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onResume) {
            m_onResume();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

} // namespace mc::client::ui::minecraft
