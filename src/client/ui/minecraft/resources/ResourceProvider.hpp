#pragma once

#include "client/renderer/trident/gui/GuiTextureAtlas.hpp"
#include <memory>
#include <optional>

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
