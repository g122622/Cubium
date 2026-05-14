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

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 村民模型
 *
 * 参考 MC 1.16.5 VillagerModel
 * 村民具有特殊的头部模型（大鼻子）、帽子和衣服。
 */
class VillagerModel : public EntityModel {
public:
    explicit VillagerModel(f32 scale = 0.0f);
    VillagerModel(f32 scale, i32 textureWidth, i32 textureHeight);
    ~VillagerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置摇头状态（交易不满意时）
     */
    void setShakingHead(bool shaking) { m_shakingHead = shaking; }

    /**
     * @brief 获取头部模型
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }

    /**
     * @brief 设置帽子可见性
     *
     * 用于多层纹理渲染时控制基础帽子是否显示。
     * 当职业帽子为 FULL 或 PARTIAL 时，可能需要隐藏基础帽子。
     *
     * 参考 MC 1.16.5 VillagerModel.setHatVisible
     */
    void setHatVisible(bool visible)
    {
        if (m_hat) {
            m_hat->setVisible(visible);
        }
        if (m_hatBrim) {
            m_hatBrim->setVisible(visible);
        }
    }

    /**
     * @brief 获取帽子模型
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHat() const { return m_hat; }

    /**
     * @brief 获取身体模型
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getBody() const { return m_body; }

    /**
     * @brief 获取衣服模型
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getClothing() const { return m_clothing; }

    /**
     * @brief 获取鼻子模型
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getNose() const { return m_nose; }

protected:
    // 头部部件
    std::shared_ptr<ModelRenderer> m_head;    // 头部
    std::shared_ptr<ModelRenderer> m_hat;     // 帽子
    std::shared_ptr<ModelRenderer> m_hatBrim; // 帽檐
    std::shared_ptr<ModelRenderer> m_nose;    // 大鼻子

    // 身体部件
    std::shared_ptr<ModelRenderer> m_body;     // 身体
    std::shared_ptr<ModelRenderer> m_clothing; // 衣服
    std::shared_ptr<ModelRenderer> m_arms;     // 手臂（交叉）

    // 腿部部件
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;

    bool m_shakingHead = false;
};

} // namespace mc::client::renderer::entity::model::animal
