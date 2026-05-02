#pragma once

#include "TemplateScreen.hpp"

namespace mc::client::ui::minecraft {

class PauseScreen : public TemplateScreen {
public:
    using Callback = std::function<void()>;

    PauseScreen();

    void setOnResume(Callback callback) { m_onResume = std::move(callback); }
    void setOnOptions(Callback callback) { m_onOptions = std::move(callback); }
    void setOnSaveAndQuit(Callback callback) { m_onSaveAndQuit = std::move(callback); }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void registerCallbacks();

    Callback m_onResume;
    Callback m_onOptions;
    Callback m_onSaveAndQuit;
};

} // namespace mc::client::ui::minecraft
