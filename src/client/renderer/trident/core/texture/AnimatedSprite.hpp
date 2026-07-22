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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include <vector>

namespace mc::client::renderer::trident {

/**
 * @brief 动画精灵
 *
 * 管理动画纹理的帧数据和播放状态。每游戏 tick 调用 tick() 推进帧状态，
 * 由 AtlasManager::uploadPendingAnimationFrames 负责把待上传帧经统一暂存池
 * 批量上传到图集（见 AtlasHandle::uploadRegionsBatch）。
 *
 * @note 此类不是线程安全的。tick() 应在主线程调用。
 */
class AnimatedSprite {
public:
    /**
     * @brief 单帧图像数据
     */
    struct FrameData {
        std::vector<u8> pixels; ///< RGBA像素数据
        u32 width = 0;          ///< 帧宽度
        u32 height = 0;         ///< 帧高度
    };

    /**
     * @brief 默认构造函数
     */
    AnimatedSprite() = default;

    /**
     * @brief 从动画元数据和帧数据构造
     * @param metadata 动画元数据
     * @param frames 帧数据数组
     * @param atlasX 在图集中的X位置（像素）
     * @param atlasY 在图集中的Y位置（像素）
     */
    AnimatedSprite(const mc::resource::metadata::AnimationMetadata& metadata,
        std::vector<FrameData>&& frames,
        u32 atlasX,
        u32 atlasY);

    // 禁止拷贝
    AnimatedSprite(const AnimatedSprite&) = delete;
    AnimatedSprite& operator=(const AnimatedSprite&) = delete;

    // 允许移动
    AnimatedSprite(AnimatedSprite&& other) noexcept = default;
    AnimatedSprite& operator=(AnimatedSprite&& other) noexcept = default;

    /**
     * @brief 每游戏tick更新动画状态
     *
     * 更新帧计数器，检查是否需要切换帧。
     * 此方法应在客户端主循环中每tick调用一次。
     */
    void tick();

    /**
     * @brief 获取插值帧进度
     * @param partialTick 部分tick（0.0-1.0）
     * @return 插值后的帧索引（用于UV偏移计算）
     *
     * 如果禁用插值，返回当前帧索引。
     * 如果启用插值，返回当前帧和下一帧之间的插值位置。
     */
    [[nodiscard]] f32 getInterpolatedFrame(f32 partialTick) const;

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否为有效动画
     * @return 如果有多个帧返回true
     */
    [[nodiscard]] bool isAnimated() const noexcept { return m_frames.size() > 1; }

    /**
     * @brief 获取当前帧索引
     */
    [[nodiscard]] i32 currentFrameIndex() const noexcept
    {
        if (m_metadata.frames.empty()) {
            return static_cast<i32>(m_frameCounter);
        }
        return m_metadata.frames[m_frameCounter].index;
    }

    /**
     * @brief 获取下一帧索引
     */
    [[nodiscard]] i32 nextFrameIndex() const noexcept;

    /**
     * @brief 获取当前帧进度（0.0-1.0）
     *
     * 表示当前帧已播放的时间比例。
     */
    [[nodiscard]] f32 frameProgress() const noexcept
    {
        if (m_currentFrameTime <= 0) {
            return 0.0f;
        }
        return static_cast<f32>(m_tickCounter) / static_cast<f32>(m_currentFrameTime);
    }

    /**
     * @brief 获取帧宽度
     */
    [[nodiscard]] u32 frameWidth() const noexcept { return m_frameWidth; }

    /**
     * @brief 获取帧高度
     */
    [[nodiscard]] u32 frameHeight() const noexcept { return m_frameHeight; }

    /**
     * @brief 获取图集X位置
     */
    [[nodiscard]] u32 atlasX() const noexcept { return m_atlasX; }

    /**
     * @brief 获取图集Y位置
     */
    [[nodiscard]] u32 atlasY() const noexcept { return m_atlasY; }

    /**
     * @brief 获取总帧数
     */
    [[nodiscard]] mc::Size frameCount() const noexcept { return m_frames.size(); }

    /**
     * @brief 获取动画元数据
     */
    [[nodiscard]] const mc::resource::metadata::AnimationMetadata& metadata() const noexcept { return m_metadata; }

    /**
     * @brief 检查是否需要上传帧数据
     */
    [[nodiscard]] bool needsUpload() const noexcept { return m_needsUpload; }

    /**
     * @brief 标记当前帧已上传
     */
    void markUploaded() noexcept { m_needsUpload = false; }

    /**
     * @brief 获取当前帧的像素数据
     *
     * 返回当前帧索引对应的帧数据。
     * 如果帧数据为空，返回空vector。
     */
    [[nodiscard]] const std::vector<u8>& currentFramePixels() const noexcept
    {
        auto idx = currentFrameIndex();
        if (idx >= 0 && static_cast<mc::Size>(idx) < m_frames.size()) {
            return m_frames[static_cast<mc::Size>(idx)].pixels;
        }
        static const std::vector<u8> empty;
        return empty;
    }

    /**
     * @brief 获取资源位置
     *
     * 返回动画纹理的资源位置标识。
     * 需要在注册时设置。
     */
    [[nodiscard]] const mc::ResourceLocation& location() const noexcept { return m_location; }

    /**
     * @brief 设置资源位置
     * @param loc 资源位置
     */
    void setLocation(const mc::ResourceLocation& loc) { m_location = loc; }

private:
    // ========== 成员变量 ==========

    mc::resource::metadata::AnimationMetadata m_metadata; ///< 动画元数据
    std::vector<FrameData> m_frames;                      ///< 帧数据数组
    mc::ResourceLocation m_location;                      ///< 资源位置

    u32 m_atlasX = 0;      ///< 图集X位置
    u32 m_atlasY = 0;      ///< 图集Y位置
    u32 m_frameWidth = 0;  ///< 帧宽度
    u32 m_frameHeight = 0; ///< 帧高度

    mc::Size m_frameCounter = 0; ///< 当前帧计数器（在frames数组中的位置）
    i32 m_tickCounter = 0;       ///< 当前帧内tick计数
    i32 m_currentFrameTime = 1;  ///< 当前帧持续时间

    bool m_needsUpload = true; ///< 是否需要上传帧数据
};

} // namespace mc::client::renderer::trident
