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

#include "OcelotModel.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猫模型
 *
 * 参考 MC 1.16.5 CatModel
 * 继承自 OcelotModel，添加猫特有的动画（睡觉、伸懒腰等）。
 */
class CatModel : public OcelotModel {
public:
    explicit CatModel(f32 scale = 0.0f);
    ~CatModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 CatModel.setLivingAnimations
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置猫特有动画状态
     * @param lieDownAmount 躺下动画进度 (0-1)
     * @param relaxStateAmount 放松状态动画进度 (0-1)
     * @param sleepPoseAmount 睡眠姿势动画进度 (0-1)
     */
    void setCatAnimState(f32 lieDownAmount, f32 relaxStateAmount, f32 sleepPoseAmount);

    /**
     * @brief 设置是否坐下
     */
    void setSitting(bool sitting) { m_isSitting = sitting; }

private:
    f32 m_lieDownAmount = 0.0f;
    f32 m_relaxStateAmount = 0.0f;
    f32 m_sleepPoseAmount = 0.0f;
    bool m_isSitting = false;
};

} // namespace mc::client::renderer::entity::model::animal
