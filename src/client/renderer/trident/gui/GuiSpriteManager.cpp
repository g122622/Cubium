#include "GuiSpriteManager.hpp"

namespace mc::client::renderer::trident::gui {

void GuiSpriteManager::registerSprite(const GuiSprite& sprite)
{
    if (sprite.id.empty()) {
        return;
    }
    m_sprites[sprite.id] = sprite;
}

void GuiSpriteManager::registerSprite(
    const std::string& id, i32 x, i32 y, i32 width, i32 height, i32 atlasWidth, i32 atlasHeight)
{
    GuiSprite sprite(id, x, y, width, height, atlasWidth, atlasHeight);
    m_sprites[id] = sprite;
}

void GuiSpriteManager::registerSprites(const std::vector<GuiSprite>& sprites)
{
    for (const auto& sprite : sprites) {
        registerSprite(sprite);
    }
}

const GuiSprite* GuiSpriteManager::getSprite(const std::string& id) const
{
    auto it = m_sprites.find(id);
    if (it != m_sprites.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GuiSpriteManager::hasSprite(const std::string& id) const
{
    return m_sprites.find(id) != m_sprites.end();
}

void GuiSpriteManager::clearSprites()
{
    m_sprites.clear();
}

void GuiSpriteManager::setAtlasSize(i32 width, i32 height)
{
    m_atlasWidth = width;
    m_atlasHeight = height;
}

} // namespace mc::client::renderer::trident::gui
