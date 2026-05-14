#include "AnimatedSprite.hpp"
#include "../TridentContext.hpp"
#include "TridentTexture.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>

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
        // MC 1.16.5: 保存旧帧索引，用于检查是否需要上传
        const i32 oldFrameIndex = currentFrameIndex();

        m_tickCounter = 0;

        // 切换到下一帧
        const mc::Size frameCount = m_metadata.frames.empty() ? m_frames.size() : m_metadata.frames.size();

        m_frameCounter = (m_frameCounter + 1) % frameCount;

        // 更新当前帧时间
        if (!m_metadata.frames.empty()) {
            m_currentFrameTime = m_metadata.getFrameTime(m_frameCounter);
        }

        // MC 1.16.5: 只有帧索引变化时才标记需要上传
        const i32 newFrameIndex = currentFrameIndex();
        if (oldFrameIndex != newFrameIndex) {
            m_needsUpload = true;
        }
    } else if (m_metadata.interpolate) {
        // MC 1.16.5: 插值期间需要持续上传
        m_needsUpload = true;
    }
}

mc::Result<void> AnimatedSprite::uploadCurrentFrame(TridentContext* context, TridentTextureAtlas& atlas)
{
    if (m_frames.empty()) {
        return mc::Error(mc::ErrorCode::InvalidState, "AnimatedSprite has no frames");
    }

    // 如果不需要上传且未启用插值，跳过
    if (!m_needsUpload && !m_metadata.interpolate) {
        return {};
    }

    FrameData frameToUpload;

    if (m_metadata.interpolate && isAnimated()) {
        // 生成插值帧
        const f32 progress = frameProgress();
        frameToUpload = generateInterpolatedFrame(progress);
    } else {
        // 使用当前帧
        const i32 frameIdx = currentFrameIndex();
        if (frameIdx < 0 || static_cast<mc::Size>(frameIdx) >= m_frames.size()) {
            return mc::Error(mc::ErrorCode::OutOfRange, "Invalid frame index");
        }
        frameToUpload = m_frames[frameIdx];
    }

    auto result = uploadFrame(context, atlas, frameToUpload);
    if (result.success()) {
        m_needsUpload = false;
    }

    return result;
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

AnimatedSprite::FrameData AnimatedSprite::generateInterpolatedFrame(f32 progress) const
{
    if (m_frames.empty() || m_frameWidth == 0 || m_frameHeight == 0) {
        return {};
    }

    const i32 currentIdx = currentFrameIndex();
    const i32 nextIdx = nextFrameIndex();

    if (currentIdx < 0 || static_cast<mc::Size>(currentIdx) >= m_frames.size() || nextIdx < 0 ||
        static_cast<mc::Size>(nextIdx) >= m_frames.size()) {
        return {};
    }

    const auto& currentFrame = m_frames[currentIdx];
    const auto& nextFrame = m_frames[nextIdx];

    // 确保两帧尺寸相同
    if (currentFrame.pixels.size() != nextFrame.pixels.size()) {
        return currentFrame;
    }

    FrameData result;
    result.width = m_frameWidth;
    result.height = m_frameHeight;
    result.pixels.resize(currentFrame.pixels.size());

    // 逐像素插值
    const mc::Size pixelCount = currentFrame.pixels.size() / 4;
    for (mc::Size i = 0; i < pixelCount; ++i) {
        const mc::Size offset = i * 4;

        // R通道
        result.pixels[offset] = static_cast<u8>(mc::math::lerp(
            static_cast<f32>(currentFrame.pixels[offset]), static_cast<f32>(nextFrame.pixels[offset]), progress));

        // G通道
        result.pixels[offset + 1] = static_cast<u8>(mc::math::lerp(static_cast<f32>(currentFrame.pixels[offset + 1]),
            static_cast<f32>(nextFrame.pixels[offset + 1]),
            progress));

        // B通道
        result.pixels[offset + 2] = static_cast<u8>(mc::math::lerp(static_cast<f32>(currentFrame.pixels[offset + 2]),
            static_cast<f32>(nextFrame.pixels[offset + 2]),
            progress));

        // A通道（不插值，保持当前帧的alpha）
        result.pixels[offset + 3] = currentFrame.pixels[offset + 3];
    }

    return result;
}

mc::Result<void> AnimatedSprite::uploadFrame(
    TridentContext* context, TridentTextureAtlas& atlas, const FrameData& frame)
{
    if (frame.pixels.empty()) {
        return mc::Error(mc::ErrorCode::InvalidData, "Frame has no pixel data");
    }

    // 计算上传区域
    const u32 atlasWidth = atlas.width();
    const u32 atlasHeight = atlas.height();

    if (m_atlasX + m_frameWidth > atlasWidth || m_atlasY + m_frameHeight > atlasHeight) {
        return mc::Error(mc::ErrorCode::OutOfRange, "Frame position out of atlas bounds");
    }

    // 通过TridentTexture上传纹理区域
    // 注意：这里需要扩展TridentTextureAtlas以支持区域上传
    // 暂时使用完整纹理上传的方式

    // TODO: 实现纹理子区域上传
    // 这需要使用vkCmdCopyBufferToImage上传到纹理的特定区域
    // 参考 MC 的 TextureAtlasSprite.uploadFrames()

    return {};
}

} // namespace mc::client::renderer::trident
