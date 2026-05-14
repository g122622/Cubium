#include "ResourceProvider.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"

namespace mc::client::ui::minecraft {

ResourceProvider::ResourceProvider(Font& font, renderer::trident::gui::GuiRenderer& renderer)
    : m_font(font)
    , m_renderer(renderer)
{}

void ResourceProvider::loadGuiTextureAtlas(const std::string& path)
{
    // TODO: 从资源路径加载纹理图集
    // 需要通过GuiTextureLoader加载
    (void)path;
}

} // namespace mc::client::ui::minecraft
