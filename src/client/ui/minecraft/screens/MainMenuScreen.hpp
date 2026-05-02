#pragma once

#include "TemplateScreen.hpp"

namespace mc::client::ui::minecraft {

class MainMenuScreen : public TemplateScreen {
public:
    using Callback = std::function<void()>;

    MainMenuScreen();

    void setOnSinglePlayer(Callback callback) { m_onSinglePlayer = std::move(callback); }
    void setOnMultiPlayer(Callback callback) { m_onMultiPlayer = std::move(callback); }
    void setOnOptions(Callback callback) { m_onOptions = std::move(callback); }
    void setOnQuit(Callback callback) { m_onQuit = std::move(callback); }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void registerCallbacks();

    Callback m_onSinglePlayer;
    Callback m_onMultiPlayer;
    Callback m_onOptions;
    Callback m_onQuit;
};

} // namespace mc::client::ui::minecraft
