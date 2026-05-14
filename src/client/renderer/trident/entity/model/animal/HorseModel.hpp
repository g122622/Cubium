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
 * @brief 马模型
 *
 * 参考 MC 1.16.5 HorseModel
 * 支持马、驴、骡、骷髅马、僵尸马。
 */
class HorseModel : public AgeableModel {
public:
    explicit HorseModel(f32 scale = 0.0f);
    ~HorseModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 HorseModel.setLivingAnimations
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置马鞍状态
     */
    void setSaddled(bool saddled) { m_saddled = saddled; }

    /**
     * @brief 设置是否被骑乘
     */
    void setRidden(bool ridden) { m_ridden = ridden; }

    /**
     * @brief 设置吃草动画进度
     */
    void setGrassEatingAmount(f32 amount) { m_grassEatingAmount = amount; }

    /**
     * @brief 设置后腿站立动画进度
     */
    void setRearingAmount(f32 amount) { m_rearingAmount = amount; }

    /**
     * @brief 设置嘴巴张开角度
     */
    void setMouthOpennessAngle(f32 angle) { m_mouthOpennessAngle = angle; }

    /**
     * @brief 设置尾巴计数器（用于尾巴摆动）
     */
    void setTailCounter(i32 counter) { m_tailCounter = counter; }

    /**
     * @brief 设置渲染偏航角
     */
    void setRenderYawOffset(f32 yaw) { m_renderYawOffset = yaw; }

    /**
     * @brief 设置前一个渲染偏航角
     */
    void setPrevRenderYawOffset(f32 yaw) { m_prevRenderYawOffset = yaw; }

    /**
     * @brief 设置头部偏航角
     */
    void setRotationYawHead(f32 yaw) { m_rotationYawHead = yaw; }

    /**
     * @brief 设置前一个头部偏航角
     */
    void setPrevRotationYawHead(f32 yaw) { m_prevRotationYawHead = yaw; }

    /**
     * @brief 设置旋转俯仰角
     */
    void setRotationPitch(f32 pitch) { m_rotationPitch = pitch; }

    /**
     * @brief 设置前一个旋转俯仰角
     */
    void setPrevRotationPitch(f32 pitch) { m_prevRotationPitch = pitch; }

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_inWater = inWater; }

    /**
     * @brief 设置生存时间
     */
    void setTicksExisted(i32 ticks) { m_ticksExisted = ticks; }

protected:
    /**
     * @brief 获取头部部件
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

    /**
     * @brief 添加耳朵部件
     */
    virtual void addEars(std::shared_ptr<ModelRenderer> head);

private:
    // 身体部件
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_head;

    // 成年体腿部
    std::shared_ptr<ModelRenderer> m_backRightLeg;  // field_228262_f_
    std::shared_ptr<ModelRenderer> m_backLeftLeg;   // field_228263_g_
    std::shared_ptr<ModelRenderer> m_frontRightLeg; // field_228264_h_
    std::shared_ptr<ModelRenderer> m_frontLeftLeg;  // field_228265_i_

    // 幼体腿部（放大版）
    std::shared_ptr<ModelRenderer> m_backRightLegBaby;  // field_228266_j_
    std::shared_ptr<ModelRenderer> m_backLeftLegBaby;   // field_228267_k_
    std::shared_ptr<ModelRenderer> m_frontRightLegBaby; // field_228268_l_
    std::shared_ptr<ModelRenderer> m_frontLeftLegBaby;  // field_228269_m_

    // 尾巴
    std::shared_ptr<ModelRenderer> m_tail; // field_217133_j

    // 鞍部件
    std::vector<std::shared_ptr<ModelRenderer>> m_saddleParts; // field_217134_k

    // 骑乘部件（缰绳）
    std::vector<std::shared_ptr<ModelRenderer>> m_ridingParts; // field_217135_l

    // 状态
    bool m_saddled = false;
    bool m_ridden = false;
    f32 m_grassEatingAmount = 0.0f;
    f32 m_rearingAmount = 0.0f;
    f32 m_mouthOpennessAngle = 0.0f;
    i32 m_tailCounter = 0;
    f32 m_renderYawOffset = 0.0f;
    f32 m_prevRenderYawOffset = 0.0f;
    f32 m_rotationYawHead = 0.0f;
    f32 m_prevRotationYawHead = 0.0f;
    f32 m_rotationPitch = 0.0f;
    f32 m_prevRotationPitch = 0.0f;
    bool m_inWater = false;
    i32 m_ticksExisted = 0;
    f32 m_scale = 0.0f;
};

} // namespace mc::client::renderer::entity::model::animal
