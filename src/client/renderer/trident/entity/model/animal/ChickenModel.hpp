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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
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
 * @brief 鸡模型
 *
 * 继承 AgeableModel 以支持幼体渲染。纹理尺寸 64x32。
 * 部件：头、喙、肉垂、身、左右翼、左右腿。
 */
class ChickenModel : public AgeableModel {
public:
    ChickenModel();
    ~ChickenModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

protected:
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_beak;   // 喙
    std::shared_ptr<ModelRenderer> m_wattle; // 肉垂（下巴下面的红肉，对应 MC red_thing）
};

} // namespace mc::client::renderer::entity::model::animal
