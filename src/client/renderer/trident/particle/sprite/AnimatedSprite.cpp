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

#include "AnimatedSprite.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle {

AnimatedSprite::AnimatedSprite(const glm::vec2& uvMin, const glm::vec2& uvMax, u32 frameCount, f64 frameTime)
    : m_uvMin(uvMin)
    , m_uvMax(uvMax)
    , m_frameCount(std::max(1u, frameCount))
    , m_frameTime(frameTime)
{}

f64 AnimatedSprite::frameHeight() const
{
    if (m_frameCount <= 1) {
        return m_uvMax.y - m_uvMin.y;
    }
    return (m_uvMax.y - m_uvMin.y) / static_cast<f64>(m_frameCount);
}

glm::vec4 AnimatedSprite::getFrameUV(f64 age, f64 maxAge) const
{
    if (m_frameCount <= 1 || maxAge <= 0.0) {
        return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
    }

    // 基于年龄计算当前帧
    const f64 progress = age / maxAge;
    const u32 frameIndex = static_cast<u32>(progress * static_cast<f64>(m_frameCount));
    const u32 clampedFrame = std::min(frameIndex, m_frameCount - 1);

    return getFrameUVByIndex(clampedFrame);
}

glm::vec4 AnimatedSprite::getRandomFrameUV(u32 seed) const
{
    if (m_frameCount <= 1) {
        return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
    }

    const u32 frameIndex = seed % m_frameCount;
    return getFrameUVByIndex(frameIndex);
}

glm::vec4 AnimatedSprite::getFrameUVByIndex(u32 frameIndex) const
{
    if (m_frameCount <= 1 || frameIndex >= m_frameCount) {
        return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
    }

    const f64 height = frameHeight();
    const f64 frameV = m_uvMin.y + static_cast<f64>(frameIndex) * height;

    return glm::vec4(m_uvMin.x, // minU
        frameV,                 // minV
        m_uvMax.x,              // maxU
        frameV + height         // maxV
    );
}

} // namespace mc::client::renderer::trident::particle
