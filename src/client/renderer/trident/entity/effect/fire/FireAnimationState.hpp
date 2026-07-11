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

#include "common/core/Types.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"

namespace mc::client::renderer::entity::effect::fire {

/**
 * @brief 单张火焰纹理的动画播放状态
 *
 * 采用与 AnimatedSprite 一致的双计数器模式：
 * m_tickCounter 累加 tick，达到 m_currentFrameTime 后切换帧。
 * 不同帧可有独立时长（由 mcmeta frames[].time 指定）。
 *
 * 插值（interpolate=true）已实现：FireEffect 在 tick() 中检测
 * metadata.interpolate，若为 true 则根据 frameProgress() 逐像素
 * lerp 当前帧与下一帧，将混合结果上传到 VkImage 对应区域。
 * 算法与 MC 1.16.5 TextureAtlasSprite.InterpolationData 一致：
 * R/G/B 三通道线性插值，A 通道不插值（保留当前帧 alpha）。
 *
 * 此结构独立于 FireEffect.hpp，不依赖 Vulkan，便于单元测试。
 */
struct FireAnimationState {
    /// 动画元数据（帧序列、frametime、interpolate）
    resource::metadata::AnimationMetadata metadata;
    /// 该纹理的帧数（用于无自定义帧序列时模运算）
    u32 frameCount = 0;
    /// 当前帧在 metadata.frames 数组中的位置（无自定义序列时等同帧索引）
    u32 frameCounter = 0;
    /// 当前帧内已累计的 tick
    i32 tickCounter = 0;
    /// 当前帧持续时间（tick）
    i32 currentFrameTime = 1;

    /**
     * @brief 从元数据和帧数初始化播放状态
     *
     * 若有自定义帧序列，currentFrameTime 取第一帧的 time；
     * 否则取 metadata.frametime。
     */
    void init(const resource::metadata::AnimationMetadata& md, u32 frames);

    /**
     * @brief 每 tick 更新动画状态
     *
     * 累加 tickCounter，达到 currentFrameTime 后切换到下一帧。
     * 帧切换采用模运算实现循环。
     */
    void tick();

    /**
     * @brief 获取当前帧索引
     *
     * 有自定义帧序列时返回 frames[frameCounter].index；
     * 否则返回 frameCounter 本身。
     */
    [[nodiscard]] i32 currentFrameIndex() const noexcept;

    /**
     * @brief 获取下一帧索引
     *
     * 用于插值：当 metadata.interpolate=true 时，需要知道下一帧
     * 索引以便在当前帧和下一帧之间逐像素 lerp。
     *
     * 有自定义帧序列时返回 frames[(frameCounter + 1) % frames.size()].index；
     * 否则返回 (frameCounter + 1) % frameCount。
     *
     * @return 下一帧索引（循环回绕到帧序列起点）
     */
    [[nodiscard]] i32 nextFrameIndex() const noexcept;

    /**
     * @brief 获取当前帧进度（0.0-1.0）
     *
     * 表示当前帧已播放的 tick 比例。
     * FireEffect::tick() 中检测到 metadata.interpolate=true 时，
     * 使用此值作为 lerp 权重，逐像素混合当前帧与下一帧，
     * 将混合结果上传到 VkImage 对应区域，产生平滑过渡。
     */
    [[nodiscard]] f32 frameProgress() const noexcept;
};

} // namespace mc::client::renderer::entity::effect::fire
