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

#include "ParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>
#include <string>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 红石粒子数据
 *
 * 用于红石粉尘粒子，携带颜色信息。
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
