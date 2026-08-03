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

#include "MinecraftUIContext.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/runtime/TemplateInstance.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc::client::ui::minecraft {

MinecraftUIContext::MinecraftUIContext(Font& font,
    renderer::trident::gui::GuiRenderer& renderer,
    kagero::state::StateStore& stateStore,
    kagero::event::EventBus& eventBus)
    : m_font(font)
    , m_renderer(renderer)
    , m_stateStore(stateStore)
    , m_eventBus(eventBus)
    , m_bindingContext(stateStore, eventBus)
    , m_resources(font, renderer)
{
    _setupStateBindings();
    _setupDefaultResources();
}

std::unique_ptr<kagero::tpl::runtime::TemplateInstance> MinecraftUIContext::createScreen(
    const std::string& templatePath)
{
    kagero::tpl::compiler::TemplateCompiler compiler;
    auto compiled = compiler.compileFile(templatePath);
    if (!compiled) {
        return nullptr;
    }

    auto instance = std::make_unique<kagero::tpl::runtime::TemplateInstance>(std::move(compiled), m_bindingContext);
    instance->registerDefaultFactories();
    instance->registerDefaultAttributeSetters();
    instance->registerDefaultEventBinders();
    return instance;
}

kagero::tpl::binder::BindingContext& MinecraftUIContext::bindingContext()
{
    return m_bindingContext;
}

const kagero::tpl::binder::BindingContext& MinecraftUIContext::bindingContext() const
{
    return m_bindingContext;
}

void MinecraftUIContext::_setupStateBindings()
{
    m_bindingContext.exposeWritable("player.health", &m_playerHealth);
    m_bindingContext.exposeWritable("player.hunger", &m_playerHunger);
    m_bindingContext.exposeWritable("player.xp", &m_playerXP);
    m_bindingContext.expose("player.name", &m_playerName);

    m_bindingContext.exposeSimpleCallback("onClose", []() {});       // TODO: 实现关闭屏幕回调
    m_bindingContext.exposeSimpleCallback("onSlotClick", []() {});   // TODO: 实现物品栏槽位点击回调
    m_bindingContext.exposeSimpleCallback("onHotbarClick", []() {}); // TODO: 实现快捷栏点击回调
}

void MinecraftUIContext::_setupDefaultResources()
{
    // TODO: 默认资源加载已移至 ResourceProvider，后续需要在此注册更多默认资源
}

} // namespace mc::client::ui::minecraft
