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
