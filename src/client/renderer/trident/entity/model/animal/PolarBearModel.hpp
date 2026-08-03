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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 北极熊模型
 *
 * 特性：
 * - 继承 QuadrupedModel 四足动物基类
 * - 支持后腿站立动画
 * - 支持成年/幼体渲染
 *
 * 纹理尺寸：128x64
 *
 * 模型部件（所有 box 直接添加到 ModelRenderer 上）：
 * - head: 头部主盒 + 鼻子 + 左耳 + 右耳（镜像）
 * - body: 身体上部分 + 下身体
 * - legBackRight/Left: 后腿
 * - legFrontRight/Left: 前腿
 */
class PolarBearModel : public QuadrupedModel {
public:
    /**
     * @brief 构造函数
     *
     * 构造参数：
     * - legHeight = 12
     * - scale = 0.0F
     * - isChildHeadScaled = true
     * - childHeadOffsetY = 16.0F
     * - childHeadOffsetZ = 4.0F
     * - childHeadScale = 2.25F
     * - childBodyScale = 2.0F
     * - childBodyOffsetY = 24
     */
    PolarBearModel();
    ~PolarBearModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置站立动画进度
     * @param standingProgress 站立进度 (0.0 = 四足站立, 1.0 = 后腿站立)
     */
    void setStandingProgress(f32 standingProgress);

    /**
     * @brief 设置生物动画状态（每帧调用）
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

protected:
    /**
     * @brief 设置模型部件
     */
    void setupParts() override;

    /**
     * @brief 获取头部部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    // 动画状态
    f32 m_standingProgress = 0.0f; // 站立进度 (0.0 ~ 1.0)
};

} // namespace mc::client::renderer::entity::model::animal
