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

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猫豹/豹猫模型
 *
 * 参考 MC 1.16.5 OcelotModel
 * 猫模型继承自此类。
 */
class OcelotModel : public AgeableModel {
public:
    explicit OcelotModel(f32 scale = 0.0f);
    ~OcelotModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 OcelotModel.setLivingAnimations
     * 用于处理蹲伏和奔跑状态
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置蹲伏状态
     */
    void setCrouching(bool crouching) { m_isCrouching = crouching; }

    /**
     * @brief 设置奔跑状态
     */
    void setSprinting(bool sprinting) { m_isSprinting = sprinting; }

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] int getState() const { return m_state; }

protected:
    // 模型部件
    std::shared_ptr<ModelRenderer> m_head;          // 头部
    std::shared_ptr<ModelRenderer> m_body;          // 身体
    std::shared_ptr<ModelRenderer> m_tail;          // 尾巴1
    std::shared_ptr<ModelRenderer> m_tail2;         // 尾巴2
    std::shared_ptr<ModelRenderer> m_backLeftLeg;   // 后左腿
    std::shared_ptr<ModelRenderer> m_backRightLeg;  // 后右腿
    std::shared_ptr<ModelRenderer> m_frontLeftLeg;  // 前左腿
    std::shared_ptr<ModelRenderer> m_frontRightLeg; // 前右腿

    int m_state = 1; // 0=蹲伏, 1=站立, 2=奔跑, 3=坐下
    bool m_isCrouching = false;
    bool m_isSprinting = false;
};

} // namespace mc::client::renderer::entity::model::animal
