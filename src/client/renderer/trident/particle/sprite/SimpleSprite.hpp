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
 * @brief 简单精灵（单帧）
 *
 * 静态纹理精灵，不支持动画。
 */
class SimpleSprite : public ISprite {
public:
    /**
     * @brief 构造简单精灵
     *
     * @param uvMin UV 左上角坐标
     * @param uvMax UV 右下角坐标
     */
    SimpleSprite(const glm::vec2& uvMin, const glm::vec2& uvMax);

    ~SimpleSprite() override = default;

    // ========================================================================
    // ISprite 接口实现
    // ========================================================================

    [[nodiscard]] glm::vec4 getFrameUV(f64 age, f64 maxAge) const override;
    [[nodiscard]] glm::vec4 getRandomFrameUV(u32 seed) const override;
    [[nodiscard]] bool isAnimated() const override { return false; }
    [[nodiscard]] u32 frameCount() const override { return 1; }
    [[nodiscard]] f64 frameTime() const override { return 0.0; }

    // ========================================================================
    // 属性访问器
    // ========================================================================

    [[nodiscard]] const glm::vec2& uvMin() const { return m_uvMin; }
    [[nodiscard]] const glm::vec2& uvMax() const { return m_uvMax; }

private:
    glm::vec2 m_uvMin;
    glm::vec2 m_uvMax;
};

} // namespace mc::client::renderer::trident::particle
