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
 * @brief 狼模型
 *
 * 参考 MC 1.16.5 WolfModel（继承自 TintedAgeableModel）
 * 狼有特殊的姿态：站立、坐下、睡觉、摇尾巴等。
 */
class WolfModel : public AgeableModel {
public:
    WolfModel();
    ~WolfModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 WolfModel.setLivingAnimations
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置动画状态
     * @param isSitting 是否坐下
     * @param isAngry 是否愤怒（func_233678_J__）
     * @param isWet 是否湿润（用于动画，非着色）
     * @param tailRotation 尾巴旋转角度（对应 ageInTicks）
     * @param shakeAngle 摇晃角度
     * @param interestedAngle 感兴趣角度
     */
    void setAnimState(bool isSitting, bool isAngry, bool isWet, f32 tailRotation, f32 shakeAngle, f32 interestedAngle);

    /**
     * @brief 设置着色颜色（湿状态）
     */
    void setTint(f32 r, f32 g, f32 b)
    {
        m_tintR = r;
        m_tintG = g;
        m_tintB = b;
    }

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
    // 头部部件
    std::shared_ptr<ModelRenderer> m_head;      // 头部旋转点
    std::shared_ptr<ModelRenderer> m_headChild; // 头部实际盒子

    // 身体部件
    std::shared_ptr<ModelRenderer> m_body; // 身体
    std::shared_ptr<ModelRenderer> m_mane; // 鬃毛

    // 腿部部件
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legBackRight;

    // 尾巴部件
    std::shared_ptr<ModelRenderer> m_tail;      // 尾巴旋转点
    std::shared_ptr<ModelRenderer> m_tailChild; // 尾巴实际盒子

    // 动画状态
    bool m_isSitting = false;
    bool m_isAngry = false;
    bool m_isWet = false;
    f32 m_tailRotation = 0.0f;
    f32 m_shakeAngle = 0.0f;
    f32 m_interestedAngle = 0.0f;

    // 着色
    f32 m_tintR = 1.0f;
    f32 m_tintG = 1.0f;
    f32 m_tintB = 1.0f;
};

} // namespace mc::client::renderer::entity::model::animal
