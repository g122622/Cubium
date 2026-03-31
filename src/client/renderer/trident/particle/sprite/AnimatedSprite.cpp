#include "AnimatedSprite.hpp"
#include <algorithm>

namespace mc::client::renderer::trident::particle {

AnimatedSprite::AnimatedSprite(
    const glm::vec2& uvMin,
    const glm::vec2& uvMax,
    u32 frameCount,
    f32 frameTime)
    : m_uvMin(uvMin)
    , m_uvMax(uvMax)
    , m_frameCount(std::max(1u, frameCount))
    , m_frameTime(frameTime)
{
}

f32 AnimatedSprite::frameHeight() const {
    if (m_frameCount <= 1) {
        return m_uvMax.y - m_uvMin.y;
    }
    return (m_uvMax.y - m_uvMin.y) / static_cast<f32>(m_frameCount);
}

glm::vec4 AnimatedSprite::getFrameUV(f32 age, f32 maxAge) const {
    if (m_frameCount <= 1 || maxAge <= 0.0f) {
        return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
    }

    // 基于年龄计算当前帧
    const f32 progress = age / maxAge;
    const u32 frameIndex = static_cast<u32>(progress * static_cast<f32>(m_frameCount));
    const u32 clampedFrame = std::min(frameIndex, m_frameCount - 1);

    return getFrameUVByIndex(clampedFrame);
}

glm::vec4 AnimatedSprite::getRandomFrameUV(u32 seed) const {
    if (m_frameCount <= 1) {
        return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
    }

    const u32 frameIndex = seed % m_frameCount;
    return getFrameUVByIndex(frameIndex);
}

glm::vec4 AnimatedSprite::getFrameUVByIndex(u32 frameIndex) const {
    if (m_frameCount <= 1 || frameIndex >= m_frameCount) {
        return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
    }

    const f32 height = frameHeight();
    const f32 frameV = m_uvMin.y + static_cast<f32>(frameIndex) * height;

    return glm::vec4(
        m_uvMin.x,      // minU
        frameV,         // minV
        m_uvMax.x,      // maxU
        frameV + height // maxV
    );
}

} // namespace mc::client::renderer::trident::particle
