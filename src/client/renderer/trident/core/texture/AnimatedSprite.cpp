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
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <utility>
#include <vector>

namespace mc::client::renderer::trident {

AnimatedSprite::AnimatedSprite(
    const mc::resource::metadata::AnimationMetadata& metadata, std::vector<FrameData>&& frames, u32 atlasX, u32 atlasY)
    : m_metadata(metadata)
    , m_frames(std::move(frames))
    , m_atlasX(atlasX)
    , m_atlasY(atlasY)
{
    if (!m_frames.empty()) {
        m_frameWidth = m_frames[0].width;
        m_frameHeight = m_frames[0].height;
    } else if (m_metadata.width > 0 && m_metadata.height > 0) {
        m_frameWidth = static_cast<u32>(m_metadata.width);
        m_frameHeight = static_cast<u32>(m_metadata.height);
    }

    // 初始化当前帧时间
    if (!m_metadata.frames.empty()) {
        m_currentFrameTime = m_metadata.getFrameTime(0);
    }
}

void AnimatedSprite::tick()
{
    if (!isAnimated()) {
        return;
    }

    ++m_tickCounter;

    // 检查是否需要切换帧
    if (m_tickCounter >= m_currentFrameTime) {
        const i32 oldFrameIndex = currentFrameIndex();

        m_tickCounter = 0;

        // 切换到下一帧
        const mc::Size frameCount = m_metadata.frames.empty() ? m_frames.size() : m_metadata.frames.size();

        m_frameCounter = (m_frameCounter + 1) % frameCount;

        // 更新当前帧时间
        if (!m_metadata.frames.empty()) {
            m_currentFrameTime = m_metadata.getFrameTime(m_frameCounter);
        }

        // 只有帧索引变化时才标记需要上传
        const i32 newFrameIndex = currentFrameIndex();
        if (oldFrameIndex != newFrameIndex) {
            m_needsUpload = true;
        }
    }
}

f32 AnimatedSprite::getInterpolatedFrame(f32 partialTick) const
{
    if (!m_metadata.interpolate || !isAnimated()) {
        return static_cast<f32>(currentFrameIndex());
    }

    const f32 progress = frameProgress() + partialTick / static_cast<f32>(m_currentFrameTime);
    const i32 currentIdx = currentFrameIndex();
    const i32 nextIdx = nextFrameIndex();

    return static_cast<f32>(currentIdx) + progress * static_cast<f32>(nextIdx - currentIdx);
}

i32 AnimatedSprite::nextFrameIndex() const noexcept
{
    if (m_metadata.frames.empty()) {
        const i32 next = static_cast<i32>(m_frameCounter) + 1;
        return next >= static_cast<i32>(m_frames.size()) ? 0 : next;
    }

    const mc::Size nextPos = (m_frameCounter + 1) % m_metadata.frames.size();
    return m_metadata.frames[nextPos].index;
}

} // namespace mc::client::renderer::trident
