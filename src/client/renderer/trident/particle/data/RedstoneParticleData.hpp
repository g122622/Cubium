#pragma once

#include "ParticleData.hpp"
#include <glm/glm.hpp>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 红石粒子数据
 *
 * 用于红石粉尘粒子，携带颜色信息。
 * 参考 MC 1.16.5 RedstoneParticleData
 *
 * 用法示例：
 * @code
 * // 红色红石粒子（默认）
 * auto redstoneData = std::make_unique<RedstoneParticleData>(glm::vec3(1.0f, 0.0f, 0.0f));
 *
 * // 自定义颜色
 * auto coloredData = std::make_unique<RedstoneParticleData>(glm::vec3(0.5f, 0.8f, 1.0f));
 * @endcode
 */
class RedstoneParticleData : public ParticleData {
public:
    /**
     * @brief 构造红石粒子数据
     *
     * @param color RGB 颜色值（每个分量 0.0-1.0）
     */
    explicit RedstoneParticleData(const glm::vec3& color = glm::vec3(1.0f, 0.0f, 0.0f));

    ~RedstoneParticleData() override = default;

    // 允许拷贝
    RedstoneParticleData(const RedstoneParticleData&) = default;
    RedstoneParticleData& operator=(const RedstoneParticleData&) = default;

    // 允许移动
    RedstoneParticleData(RedstoneParticleData&&) noexcept = default;
    RedstoneParticleData& operator=(RedstoneParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return ParticleTypeId::Redstone; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 红石特有方法
    // ========================================================================

    /**
     * @brief 获取颜色
     *
     * @return RGB 颜色值
     */
    [[nodiscard]] const glm::vec3& getColor() const { return m_color; }

    /**
     * @brief 设置颜色
     *
     * @param color RGB 颜色值
     */
    void setColor(const glm::vec3& color) { m_color = color; }

private:
    glm::vec3 m_color; ///< RGB 颜色值
};

} // namespace mc::client::renderer::trident::particle::data
