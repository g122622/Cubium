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

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 僵尸模型
 *
 * 僵尸是双足生物，手臂向前伸，攻击时有特殊动画。
 */
class ZombieModel : public model::BipedModel {
public:
    /**
     * @brief 构造函数
     * @param slim 是否使用细长纹理（尸壳/溺尸使用64x32，普通僵尸使用64x64）
     */
    explicit ZombieModel(bool slim = false);
    ~ZombieModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置纹理尺寸
     * @param useSlimTexture true使用64x32纹理，false使用64x64纹理
     */
    void setTextureDimensions(bool useSlimTexture);

    /**
     * @brief 设置攻击状态
     * @param aggressive 是否处于攻击状态
     */
    void setAggressive(bool aggressive) { m_isAggressive = aggressive; }

    /**
     * @brief 获取攻击状态
     */
    [[nodiscard]] bool isAggressive() const { return m_isAggressive; }

private:
    void setupParts() override;

    bool m_slim = false;         // 是否使用细长纹理
    bool m_isAggressive = false; // 是否处于激怒/攻击中状态（由 EntityRendererManager::_applyZombieState 推送）
};

} // namespace mc::client::renderer::entity::model::monster
