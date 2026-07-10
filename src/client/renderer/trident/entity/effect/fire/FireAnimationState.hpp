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
 * TODO: interpolate 插值尚未实现。当 metadata.interpolate=true 时，
 * 应在帧切换过程中根据 frameProgress() 计算当前帧与下一帧之间的
 * 混合 V 偏移（或在着色器中做双采样混合）。当前实现直接跳帧，
 * 不做插值。frameProgress() 已预留供未来插值实现使用。
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
     * @brief 获取当前帧进度（0.0-1.0）
     *
     * 表示当前帧已播放的 tick 比例。
     * 供未来 interpolate 插值实现使用：当 metadata.interpolate=true 时，
     * 渲染器可据此值在当前帧和下一帧之间做 UV 混合或着色器双采样。
     * 当前未启用插值，此方法暂未被渲染路径调用。
     */
    [[nodiscard]] f32 frameProgress() const noexcept;
};

} // namespace mc::client::renderer::entity::effect::fire
