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
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 实体效果粒子数据
 *
 * 携带实体效果粒子（EntityEffect）的 ARGB 颜色数据。
 * 对应 MC Java 的 ColorParticleOption（color）。
 *
 * MC 协议格式：i32 color(ARGB)（4 字节）
 *
 * 与 DustParticleData 的区别：
 * - EntityEffect 仅携带颜色，无 scale
 * - 颜色直接作为粒子的固定颜色，alpha 通常为 0（透明），由粒子本身决定
 *
 * 用法示例：
 * @code
 * // 红色实体效果粒子
 * auto effectData = std::make_unique<EntityEffectParticleData>(0xFFFF0000);
 *
 * // 自定义颜色（白色）
 * auto whiteData = std::make_unique<EntityEffectParticleData>(0xFFFFFFFF);
 * @endcode
 */
class EntityEffectParticleData : public ParticleData {
public:
    /**
     * @brief 构造实体效果粒子数据
     *
     * @param color ARGB 颜色值（如 0xFFFF0000 为红色）
     */
    explicit EntityEffectParticleData(u32 color = 0xFFFFFFFF);

    ~EntityEffectParticleData() override = default;

    // 允许拷贝
    EntityEffectParticleData(const EntityEffectParticleData&) = default;
    EntityEffectParticleData& operator=(const EntityEffectParticleData&) = default;

    // 允许移动
    EntityEffectParticleData(EntityEffectParticleData&&) noexcept = default;
    EntityEffectParticleData& operator=(EntityEffectParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return ParticleTypeId::EntityEffect; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 实体效果粒子特有接口
    // ========================================================================

    /**
     * @brief 获取 ARGB 颜色值
     *
     * @return ARGB 颜色（如 0xFFFF0000 为红色）
     */
    [[nodiscard]] u32 color() const noexcept { return m_color; }

    /**
     * @brief 将 ARGB 颜色转换为 RGBA 浮点向量
     *
     * @return RGBA 颜色向量（每个分量 0.0-1.0）
     */
    [[nodiscard]] glm::vec4 toRGBAVector() const;

    /**
     * @brief 从 RGBA 浮点向量创建实体效果粒子数据
     *
     * @param rgba RGBA 颜色向量（每个分量 0.0-1.0）
     * @return 实体效果粒子数据
     */
    static EntityEffectParticleData fromRGBAVector(const glm::vec4& rgba);

private:
    u32 m_color; ///< ARGB 颜色值
};

} // namespace mc::client::renderer::trident::particle::data
