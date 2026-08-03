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

#pragma once

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/runtime/TemplateInstance.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "resources/ResourceProvider.hpp"
#include <memory>
#include <string>

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
    void _setupStateBindings();
    void _setupDefaultResources();

    Font& m_font;
    renderer::trident::gui::GuiRenderer& m_renderer;
    kagero::state::StateStore& m_stateStore;
    kagero::event::EventBus& m_eventBus;
    kagero::tpl::binder::BindingContext m_bindingContext;
    ResourceProvider m_resources;

    i32 m_playerHealth = static_cast<i32>(mc::game::PLAYER_MAX_HEALTH);
    i32 m_playerHunger = mc::game::PLAYER_MAX_HUNGER;
    i32 m_playerXP = 0;
    std::string m_playerName = "Steve";
};

} // namespace mc::client::ui::minecraft
