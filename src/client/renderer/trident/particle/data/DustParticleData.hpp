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
 * @brief 灰尘粒子数据
 *
 * 携带灰尘粒子（Dust / Redstone）的颜色和缩放数据。
 * 对应 MC Java 的 DustParticleOption（color, scale）。
 *
 * MC 协议格式：i32 color(ARGB), f32 scale
 *
 * 用法示例：
 * @code
 * // 红色灰尘粒子
 * auto dustData = std::make_unique<DustParticleData>(0xFFFF0000, 1.0f);
 *
 * // 自定义颜色和缩放
 * auto customData = std::make_unique<DustParticleData>(0xFF00FF00, 2.0f);
 * @endcode
 */
class DustParticleData : public ParticleData {
public:
    /**
     * @brief 构造灰尘粒子数据
     *
     * @param color ARGB 颜色值（如 0xFFFF0000 为红色）
     * @param scale 缩放因子（MC 限制 [0.01, 4.0]）
     */
    DustParticleData(u32 color = 0xFFFF0000, f32 scale = 1.0f);

    ~DustParticleData() override = default;

    // 允许拷贝
    DustParticleData(const DustParticleData&) = default;
    DustParticleData& operator=(const DustParticleData&) = default;

    // 允许移动
    DustParticleData(DustParticleData&&) noexcept = default;
    DustParticleData& operator=(DustParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return ParticleTypeId::Dust; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 灰尘粒子特有接口
    // ========================================================================

    /**
     * @brief 获取 ARGB 颜色值
     *
     * @return ARGB 颜色（如 0xFFFF0000 为红色）
     */
    [[nodiscard]] u32 color() const noexcept { return m_color; }

    /**
     * @brief 获取缩放因子
     *
     * @return 缩放因子
     */
    [[nodiscard]] f32 scale() const noexcept { return m_scale; }

    /**
     * @brief 将 ARGB 颜色转换为 RGBA 浮点向量
     *
     * @return RGBA 颜色向量（每个分量 0.0-1.0）
     */
    [[nodiscard]] glm::vec4 toRGBAVector() const;

    /**
     * @brief 从 RGBA 浮点向量创建灰尘粒子数据
     *
     * @param rgba RGBA 颜色向量（每个分量 0.0-1.0）
     * @param scale 缩放因子
     * @return 灰尘粒子数据
     */
    static DustParticleData fromRGBAVector(const glm::vec4& rgba, f32 scale = 1.0f);

private:
    u32 m_color; ///< ARGB 颜色值
    f32 m_scale; ///< 缩放因子
};

/**
 * @brief 颜色过渡灰尘粒子数据
 *
 * 携带颜色过渡灰尘粒子（DustColorTransition）的起始颜色、目标颜色和缩放数据。
 * 对应 MC Java 的 DustColorTransitionOption（fromColor, toColor, scale）。
 *
 * MC 协议格式：i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
 */
class DustColorTransitionParticleData : public ParticleData {
public:
    /**
     * @brief 构造颜色过渡灰尘粒子数据
     *
     * @param fromColor 起始 ARGB 颜色值（如 0xFF39E5C0 为幽匿青色）
     * @param toColor 目标 ARGB 颜色值（如 0xFFFF0000 为红色）
     * @param scale 缩放因子（MC 限制 [0.01, 4.0]）
     */
    DustColorTransitionParticleData(u32 fromColor = 0xFF39E5C0, u32 toColor = 0xFFFF0000, f32 scale = 1.0f);

    ~DustColorTransitionParticleData() override = default;

    // 允许拷贝
    DustColorTransitionParticleData(const DustColorTransitionParticleData&) = default;
    DustColorTransitionParticleData& operator=(const DustColorTransitionParticleData&) = default;

    // 允许移动
    DustColorTransitionParticleData(DustColorTransitionParticleData&&) noexcept = default;
    DustColorTransitionParticleData& operator=(DustColorTransitionParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return ParticleTypeId::DustColorTransition; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 颜色过渡灰尘粒子特有接口
    // ========================================================================

    /**
     * @brief 获取起始 ARGB 颜色值
     *
     * @return 起始 ARGB 颜色
     */
    [[nodiscard]] u32 fromColor() const noexcept { return m_fromColor; }

    /**
     * @brief 获取目标 ARGB 颜色值
     *
     * @return 目标 ARGB 颜色
     */
    [[nodiscard]] u32 toColor() const noexcept { return m_toColor; }

    /**
     * @brief 获取缩放因子
     *
     * @return 缩放因子
     */
    [[nodiscard]] f32 scale() const noexcept { return m_scale; }

    /**
     * @brief 将起始 ARGB 颜色转换为 RGBA 浮点向量
     *
     * @return RGBA 颜色向量（每个分量 0.0-1.0）
     */
    [[nodiscard]] glm::vec4 fromColorToRGBAVector() const;

    /**
     * @brief 将目标 ARGB 颜色转换为 RGBA 浮点向量
     *
     * @return RGBA 颜色向量（每个分量 0.0-1.0）
     */
    [[nodiscard]] glm::vec4 toColorToRGBAVector() const;

private:
    u32 m_fromColor; ///< 起始 ARGB 颜色值
    u32 m_toColor;   ///< 目标 ARGB 颜色值
    f32 m_scale;     ///< 缩放因子
};

} // namespace mc::client::renderer::trident::particle::data
