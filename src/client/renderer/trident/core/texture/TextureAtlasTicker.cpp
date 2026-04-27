#include "TextureAtlasTicker.hpp"
#include <algorithm>

namespace mc::client::renderer::trident {

void TextureAtlasTicker::registerAnimatedSprite(std::shared_ptr<AnimatedSprite> sprite) {
    if (!sprite || !sprite->isAnimated()) {
        return;
    }

    // 检查是否已注册
    const auto it = std::find_if(m_sprites.begin(), m_sprites.end(),
        [&sprite](const std::shared_ptr<AnimatedSprite>& existing) {
            return existing.get() == sprite.get();
        });

    if (it == m_sprites.end()) {
        m_sprites.push_back(std::move(sprite));
    }
}

void TextureAtlasTicker::unregisterAnimatedSprite(const AnimatedSprite* sprite) {
    const auto it = std::find_if(m_sprites.begin(), m_sprites.end(),
        [sprite](const std::shared_ptr<AnimatedSprite>& existing) {
            return existing.get() == sprite;
        });

    if (it != m_sprites.end()) {
        m_sprites.erase(it);
    }
}

void TextureAtlasTicker::tick() {
    for (auto& sprite : m_sprites) {
        if (sprite) {
            sprite->tick();
        }
    }
}

Result<void> TextureAtlasTicker::uploadPendingFrames(
    TridentContext* context,
    TridentTextureAtlas& atlas)
{
    for (auto& sprite : m_sprites) {
        if (sprite) {
            auto result = sprite->uploadCurrentFrame(context, atlas);
            if (!result.success()) {
                // 记录错误但继续处理其他精灵
                // spdlog::warn("Failed to upload animated sprite frame: {}", result.error().message());
            }
        }
    }

    return {};
}

void TextureAtlasTicker::clear() {
    m_sprites.clear();
}

} // namespace mc::client::renderer::trident
