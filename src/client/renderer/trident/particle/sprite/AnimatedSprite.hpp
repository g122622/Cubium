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

#include "ISprite.hpp"
#include "common/core/Types.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle {

/**
 * @brief 动画精灵（多帧）
 *
 * 支持多帧动画的粒子纹理精灵。
 * 帧按垂直方向排列在纹理中。
 */
class AnimatedSprite : public ISprite {
public:
    /**
     * @brief 构造动画精灵
     *
     * @param uvMin UV 左上角坐标（整个动画条的左上角）
     * @param uvMax UV 右下角坐标（整个动画条的右下角）
     * @param frameCount 帧数
     * @param frameTime 每帧持续时间（秒）
     */
    AnimatedSprite(const glm::vec2& uvMin, const glm::vec2& uvMax, u32 frameCount, f64 frameTime);

    ~AnimatedSprite() override = default;

    // ========================================================================
    // ISprite 接口实现
    // ========================================================================

    [[nodiscard]] glm::vec4 getFrameUV(f64 age, f64 maxAge) const override;
    [[nodiscard]] glm::vec4 getRandomFrameUV(u32 seed) const override;
    [[nodiscard]] bool isAnimated() const override { return m_frameCount > 1; }
    [[nodiscard]] u32 frameCount() const override { return m_frameCount; }
    [[nodiscard]] f64 frameTime() const override { return m_frameTime; }

    // ========================================================================
    // 属性访问器
    // ========================================================================

    [[nodiscard]] const glm::vec2& uvMin() const { return m_uvMin; }
    [[nodiscard]] const glm::vec2& uvMax() const { return m_uvMax; }

    /**
     * @brief 获取单帧高度
     *
     * @return 单帧在 UV 空间中的高度
     */
    [[nodiscard]] f64 frameHeight() const;

    /**
     * @brief 获取指定帧的 UV 坐标
     *
     * @param frameIndex 帧索引（0 到 frameCount-1）
     * @return UV 坐标 (minU, minV, maxU, maxV)
     */
    [[nodiscard]] glm::vec4 getFrameUVByIndex(u32 frameIndex) const;

private:
    glm::vec2 m_uvMin;
    glm::vec2 m_uvMax;
    u32 m_frameCount;
    f64 m_frameTime;
};

} // namespace mc::client::renderer::trident::particle
