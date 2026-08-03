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
 * @brief 兔子模型
 *
 * 纹理尺寸: 64x32
 *
 * 部件：
 * - rabbitLeftFoot/rabbitRightFoot: 后脚
 * - rabbitLeftThigh/rabbitRightThigh: 大腿
 * - rabbitBody: 身体
 * - rabbitLeftArm/rabbitRightArm: 前腿
 * - rabbitHead: 头部
 * - rabbitRightEar/rabbitLeftEar: 耳朵
 * - rabbitTail: 尾巴
 * - rabbitNose: 鼻子
 *
 * 幼体参数:
 * - 头部缩放: 0.56666666 (17/30)
 * - 身体缩放: 0.4 (2/5)
 * - 头部偏移: Y + 5, Z + 2
 * - 身体偏移: Y + 24
 */
class RabbitModel : public AgeableModel {
public:
    RabbitModel();
    ~RabbitModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置跳跃旋转值
     * @param jumpRotation 跳跃旋转值 (0.0 - 1.0，通过 sin(jumpCompletion * PI) 计算)
     */
    void setJumpRotation(f32 jumpRotation);

protected:
    /**
     * @brief 获取头部部件（用于幼体渲染）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件（用于幼体渲染）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    void _setupParts();

    // 后脚
    std::shared_ptr<ModelRenderer> m_leftFoot;
    std::shared_ptr<ModelRenderer> m_rightFoot;
    // 大腿
    std::shared_ptr<ModelRenderer> m_leftThigh;
    std::shared_ptr<ModelRenderer> m_rightThigh;
    // 身体
    std::shared_ptr<ModelRenderer> m_body;
    // 前腿
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightArm;
    // 头部
    std::shared_ptr<ModelRenderer> m_head;
    // 耳朵
    std::shared_ptr<ModelRenderer> m_rightEar;
    std::shared_ptr<ModelRenderer> m_leftEar;
    // 尾巴
    std::shared_ptr<ModelRenderer> m_tail;
    // 鼻子
    std::shared_ptr<ModelRenderer> m_nose;

    f32 m_jumpRotation = 0.0f;
};

} // namespace mc::client::renderer::entity::model::animal
