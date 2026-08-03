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

#include "TextureAtlasTicker.hpp"
#include "client/renderer/trident/core/texture/AnimatedSprite.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace mc::client::renderer::trident {

void TextureAtlasTicker::registerAnimatedSprite(std::shared_ptr<AnimatedSprite> sprite)
{
    if (!sprite || !sprite->isAnimated()) {
        return;
    }

    // 检查是否已注册
    const auto it = std::find_if(m_sprites.begin(),
        m_sprites.end(),
        [&sprite](const std::shared_ptr<AnimatedSprite>& existing) { return existing.get() == sprite.get(); });

    if (it == m_sprites.end()) {
        m_sprites.push_back(std::move(sprite));
    }
}

void TextureAtlasTicker::unregisterAnimatedSprite(const AnimatedSprite* sprite)
{
    const auto it = std::find_if(m_sprites.begin(),
        m_sprites.end(),
        [sprite](const std::shared_ptr<AnimatedSprite>& existing) { return existing.get() == sprite; });

    if (it != m_sprites.end()) {
        m_sprites.erase(it);
    }
}

void TextureAtlasTicker::tick()
{
    for (auto& sprite : m_sprites) {
        sprite->tick();
    }
}

void TextureAtlasTicker::clear()
{
    m_sprites.clear();
}

} // namespace mc::client::renderer::trident
