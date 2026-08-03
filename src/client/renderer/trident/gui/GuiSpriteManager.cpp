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

#include "GuiSpriteManager.hpp"
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <vector>

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
    return m_sprites.contains(id);
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
