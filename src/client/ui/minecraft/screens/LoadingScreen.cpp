#include "LoadingScreen.hpp"
#include "../../kagero/event/EventBus.hpp"
#include "../../kagero/state/StateStore.hpp"
#include "common/util/math/MathUtils.hpp"

namespace mc::client::ui::minecraft {

LoadingScreen::LoadingScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "loading")
{
    exposeReactive("loading.title", m_titleValue);
    exposeReactive("loading.stage", m_stageValue);
    exposeReactive("loading.progressWidth", m_progressWidth);

    m_titleValue.set("Loading World...");
    m_stageValue.set("Preparing world...");
    m_progressWidth.set(0);

    loadTemplateFile("src/client/ui/minecraft/templates/loading.tpl");
}

void LoadingScreen::setStage(const std::string& stage)
{
    m_stageValue.set(stage);
}

void LoadingScreen::setProgress(f32 progress)
{
    const f32 clamped = mc::math::clamp(progress, 0.0f, 1.0f);
    m_progressWidth.set(static_cast<i32>(PROGRESS_BAR_WIDTH * clamped));
}

void LoadingScreen::setTitle(const std::string& title)
{
    m_titleValue.set(title);
}

} // namespace mc::client::ui::minecraft
