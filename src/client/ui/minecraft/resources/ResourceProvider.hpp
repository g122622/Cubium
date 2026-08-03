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

#include "client/renderer/trident/gui/GuiTextureAtlas.hpp"
#include <string>

namespace mc::client {
class Font;
}

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client::ui::minecraft {

/**
 * @brief UI资源提供者
 *
 * 管理Minecraft UI所需的资源，如纹理图集。
 * 不再依赖 IRenderBackend，改为直接使用 Font 和 GuiRenderer。
 */
class ResourceProvider {
public:
    /**
     * @brief 构造函数
     * @param font 字体对象
     * @param renderer GUI渲染器
     */
    ResourceProvider(Font& font, renderer::trident::gui::GuiRenderer& renderer);

    /**
     * @brief 加载GUI纹理图集
     * @param path 资源路径
     */
    void loadGuiTextureAtlas(const std::string& path);

    /**
     * @brief 获取纹理图集（可变）
     */
    [[nodiscard]] renderer::trident::gui::GuiTextureAtlas& atlas() { return m_atlas; }
    [[nodiscard]] const renderer::trident::gui::GuiTextureAtlas& atlas() const { return m_atlas; }

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
    Font& m_font;
    renderer::trident::gui::GuiRenderer& m_renderer;
    renderer::trident::gui::GuiTextureAtlas m_atlas;
};

} // namespace mc::client::ui::minecraft
