#pragma once

#include "../kagero/event/EventBus.hpp"
#include "../kagero/state/StateStore.hpp"
#include "../kagero/template/binder/BindingContext.hpp"
#include "../kagero/template/compiler/TemplateCompiler.hpp"
#include "../kagero/template/runtime/TemplateInstance.hpp"
#include "resources/ResourceProvider.hpp"
#include <memory>

namespace mc::client {
class Font;
}

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client::ui::minecraft {

/**
 * @brief Minecraft UI业务上下文
 *
 * 提供Minecraft UI系统的业务逻辑支持，包括：
 * - 状态绑定（玩家生命值、饥饿值等）
 * - 事件绑定（点击、关闭等）
 * - 资源管理（纹理图集、字体等）
 *
 * 不再依赖 IRenderBackend，改为直接使用 Font 和 GuiRenderer。
 */
class MinecraftUIContext {
public:
    /**
     * @brief 构造函数
     * @param font 字体对象
     * @param renderer GUI渲染器
     * @param stateStore 状态存储
     * @param eventBus 事件总线
     */
    MinecraftUIContext(Font& font,
        renderer::trident::gui::GuiRenderer& renderer,
        kagero::state::StateStore& stateStore,
        kagero::event::EventBus& eventBus);

    /**
     * @brief 从模板创建屏幕
     * @param templatePath 模板文件路径
     * @return 模板实例，失败返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<kagero::tpl::runtime::TemplateInstance> createScreen(const std::string& templatePath);

    [[nodiscard]] kagero::tpl::binder::BindingContext& bindingContext();
    [[nodiscard]] const kagero::tpl::binder::BindingContext& bindingContext() const;

    /**
     * @brief 获取字体
     */
    [[nodiscard]] Font& font() { return m_font; }
    [[nodiscard]] const Font& font() const { return m_font; }

    /**
     * @brief 获取GUI渲染器
     */
    [[nodiscard]] renderer::trident::gui::GuiRenderer& renderer() { return m_renderer; }
    [[nodiscard]] const renderer::trident::gui::GuiRenderer& renderer() const { return m_renderer; }

private:
    void setupStateBindings();
    void setupDefaultResources();

    Font& m_font;
    renderer::trident::gui::GuiRenderer& m_renderer;
    kagero::state::StateStore& m_stateStore;
    kagero::event::EventBus& m_eventBus;
    kagero::tpl::binder::BindingContext m_bindingContext;
    ResourceProvider m_resources;

    i32 m_playerHealth = 20;
    i32 m_playerHunger = 20;
    i32 m_playerXP = 0;
    std::string m_playerName = "Steve";
};

} // namespace mc::client::ui::minecraft
