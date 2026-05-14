#pragma once

#include "../../kagero/state/ReactiveState.hpp"
#include "TemplateScreen.hpp"
#include <memory>

namespace mc::client::ui::minecraft {

class LoadingScreen : public TemplateScreen {
public:
    LoadingScreen();

    void setStage(const std::string& stage);
    void setProgress(f32 progress);
    void setTitle(const std::string& title);

private:
    kagero::state::Reactive<std::string> m_titleValue;
    kagero::state::Reactive<std::string> m_stageValue;
    kagero::state::Reactive<i32> m_progressWidth;

    static constexpr i32 PROGRESS_BAR_WIDTH = 300;
};

} // namespace mc::client::ui::minecraft
