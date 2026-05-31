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

#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 羊驼模型
 *
 * 支持普通羊驼和流浪商人羊驼。
 *
 * 纹理尺寸：128x64 像素
 *
 * 颜色变体：
 * - Creamy (0): 奶油色
 * - White (1): 白色
 * - Brown (2): 棕色
 * - Gray (3): 灰色
 */
class LlamaModel : public AgeableModel {
public:
    explicit LlamaModel(f32 scale = 0.0f);
    ~LlamaModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否装备箱子
     *
     * 羊驼可以装备箱子，装备后会在身体两侧显示箱子模型。
     */
    void setHasChest(bool hasChest) { m_hasChest = hasChest; }

    /**
     * @brief 获取是否装备箱子
     */
    [[nodiscard]] bool hasChest() const { return m_hasChest; }

protected:
    /**
     * @brief 获取头部部件
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    // 身体部件
    std::shared_ptr<ModelRenderer> m_body;

    // 头部部件
    std::shared_ptr<ModelRenderer> m_head;

    // 成年体腿部
    std::shared_ptr<ModelRenderer> m_backRightLeg;
    std::shared_ptr<ModelRenderer> m_backLeftLeg;
    std::shared_ptr<ModelRenderer> m_frontRightLeg;
    std::shared_ptr<ModelRenderer> m_frontLeftLeg;

    // 箱子部件（成年且有箱子时显示）
    std::shared_ptr<ModelRenderer> m_chest1; // 左侧箱子
    std::shared_ptr<ModelRenderer> m_chest2; // 右侧箱子

    // 状态
    bool m_hasChest = false;
    f32 m_scale = 0.0f;
};

} // namespace mc::client::renderer::entity::model::animal
