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

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 苦力怕模型
 *
 * 苦力怕有独特的四足身体和头部结构。
 * 包含 creeperArmor 部件用于闪电苦力怕的充能效果。
 */
class CreeperModel : public model::EntityModel {
public:
    CreeperModel();
    explicit CreeperModel(f32 scale);
    ~CreeperModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void renderArmor(f64 scale = 1.0f / 16.0f);

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 获取盔甲层模型
     * 用于闪电苦力怕的充能光效渲染
     */
    [[nodiscard]] std::shared_ptr<model::ModelRenderer> getArmorHead() const { return m_armorHead; }

private:
    void _setupParts(f32 scale);

    std::shared_ptr<model::ModelRenderer> m_head;
    std::shared_ptr<model::ModelRenderer> m_body;
    std::shared_ptr<model::ModelRenderer> m_legFrontRight;
    std::shared_ptr<model::ModelRenderer> m_legFrontLeft;
    std::shared_ptr<model::ModelRenderer> m_legBackRight;
    std::shared_ptr<model::ModelRenderer> m_legBackLeft;

    // 盔甲层（用于闪电苦力怕充能效果）
    std::shared_ptr<model::ModelRenderer> m_armorHead;
};

} // namespace mc::client::renderer::entity::model::monster
