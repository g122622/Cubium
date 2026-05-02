#include "MinecraftUIContext.hpp"

namespace mc::client::ui::minecraft {

MinecraftUIContext::MinecraftUIContext(
    Font& font,
    renderer::trident::gui::GuiRenderer& renderer,
    kagero::state::StateStore& stateStore,
    kagero::event::EventBus& eventBus
) : m_font(font)
  , m_renderer(renderer)
  , m_stateStore(stateStore)
  , m_eventBus(eventBus)
  , m_bindingContext(stateStore, eventBus)
  , m_resources(font, renderer) {
    setupStateBindings();
    setupDefaultResources();
}

std::unique_ptr<kagero::tpl::runtime::TemplateInstance> MinecraftUIContext::createScreen(const String& templatePath) {
    kagero::tpl::compiler::TemplateCompiler compiler;
    auto compiled = compiler.compileFile(templatePath);
    if (!compiled) {
        return nullptr;
    }

    auto instance = std::make_unique<kagero::tpl::runtime::TemplateInstance>(
        std::move(compiled), m_bindingContext);
    instance->registerDefaultFactories();
    instance->registerDefaultAttributeSetters();
    instance->registerDefaultEventBinders();
    return instance;
}

kagero::tpl::binder::BindingContext& MinecraftUIContext::bindingContext() {
    return m_bindingContext;
}

const kagero::tpl::binder::BindingContext& MinecraftUIContext::bindingContext() const {
    return m_bindingContext;
}

void MinecraftUIContext::setupStateBindings() {
    m_bindingContext.exposeWritable("player.health", &m_playerHealth);
    m_bindingContext.exposeWritable("player.hunger", &m_playerHunger);
    m_bindingContext.exposeWritable("player.xp", &m_playerXP);
    m_bindingContext.expose("player.name", &m_playerName);

    m_bindingContext.exposeSimpleCallback("onClose", []() {});
    m_bindingContext.exposeSimpleCallback("onSlotClick", []() {});
    m_bindingContext.exposeSimpleCallback("onHotbarClick", []() {});
}

void MinecraftUIContext::setupDefaultResources() {
    // 默认资源加载已移至 ResourceProvider
}

} // namespace mc::client::ui::minecraft
