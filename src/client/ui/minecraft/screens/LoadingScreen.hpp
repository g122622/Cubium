#pragma once

#include "TemplateScreen.hpp"
#include "../../kagero/state/ReactiveState.hpp"
#include <memory>

namespace mc::client::ui::minecraft {

class LoadingScreen : public TemplateScreen {
public:
    LoadingScreen();

    void setStage(const String& stage);
    void setProgress(f32 progress);
    void setTitle(const String& title);

private:
    kagero::state::Reactive<String> m_titleValue;
    kagero::state::Reactive<String> m_stageValue;
    kagero::state::Reactive<i32> m_progressWidth;

    static constexpr i32 PROGRESS_BAR_WIDTH = 300;
};

} // namespace mc::client::ui::minecraft
