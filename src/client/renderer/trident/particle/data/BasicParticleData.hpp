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

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 基础粒子数据（无参数粒子）
 *
 * 用于不需要额外参数的粒子类型，如火焰、烟雾、爱心等。
 *
 * 用法示例：
 * @code
 * auto flameData = std::make_unique<BasicParticleData>(ParticleTypeId::Flame);
 * auto smokeData = std::make_unique<BasicParticleData>(ParticleTypeId::Smoke);
 * @endcode
 */
class BasicParticleData : public ParticleData {
public:
    /**
     * @brief 构造基础粒子数据
     *
     * @param type 粒子类型 ID
     */
    explicit BasicParticleData(ParticleTypeId type);

    /**
     * @brief 从资源位置构造基础粒子数据
     *
     * @param typeName 粒子类型名称（如 "minecraft:flame"）
     */
    explicit BasicParticleData(const std::string& typeName);

    ~BasicParticleData() override = default;

    // 允许拷贝
    BasicParticleData(const BasicParticleData&) = default;
    BasicParticleData& operator=(const BasicParticleData&) = default;

    // 允许移动
    BasicParticleData(BasicParticleData&&) noexcept = default;
    BasicParticleData& operator=(BasicParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return m_type; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override { return ""; }
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

private:
    ParticleTypeId m_type;
};

} // namespace mc::client::renderer::trident::particle::data
