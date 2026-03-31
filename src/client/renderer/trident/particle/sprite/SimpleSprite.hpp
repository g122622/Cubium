#pragma once

#include "ISprite.hpp"

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

    [[nodiscard]] glm::vec4 getFrameUV(f32 age, f32 maxAge) const override;
    [[nodiscard]] glm::vec4 getRandomFrameUV(u32 seed) const override;
    [[nodiscard]] bool isAnimated() const override { return false; }
    [[nodiscard]] u32 frameCount() const override { return 1; }
    [[nodiscard]] f32 frameTime() const override { return 0.0f; }

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
