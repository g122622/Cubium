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
 * @brief 末影人模型
 *
 * 末影人身材高大，手臂和腿很长。
 */
class EndermanModel : public model::BipedModel {
public:
    EndermanModel();
    ~EndermanModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置携带状态
     */
    void setCarrying(bool carrying) { m_carrying = carrying; }

    /**
     * @brief 设置尖叫/攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    /**
     * @brief 获取携带状态
     */
    [[nodiscard]] bool isCarrying() const { return m_carrying; }

    /**
     * @brief 获取攻击状态
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

private:
    void setupParts() override;

    bool m_carrying = false;  // 携带方块状态
    bool m_attacking = false; // 尖叫/攻击状态
};

} // namespace mc::client::renderer::entity::model::monster
