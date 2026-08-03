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

namespace mc::client::renderer::entity::model {

/**
 * @brief 四足动物模型基类
 *
 * 用于猪、牛、羊等四足动物的模型基类。
 */
class QuadrupedModel : public AgeableModel {
public:
    /**
     * @brief 默认构造函数（腿高为 6，无膨胀）
     */
    QuadrupedModel();

    /**
     * @brief 完整参数的构造函数
     * @param legHeight 腿高度（像素）
     * @param scale 膨胀值
     * @param isChildHeadScaled 是否缩放幼体头部
     * @param childHeadOffsetY 幼体头部 Y 偏移
     * @param childHeadOffsetZ 幼体头部 Z 偏移
     * @param childHeadScale 幼体头部缩放
     * @param childBodyScale 幼体身体缩放
     * @param childBodyOffsetY 幼体身体 Y 偏移
     */
    QuadrupedModel(i32 legHeight,
        f32 scale,
        bool isChildHeadScaled,
        f32 childHeadOffsetY,
        f32 childHeadOffsetZ,
        f32 childHeadScale,
        f32 childBodyScale,
        f32 childBodyOffsetY);

    ~QuadrupedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

protected:
    /**
     * @brief 设置模型部件
     */
    virtual void setupParts();

    /**
     * @brief 获取头部部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

    // 模型部件
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;

    // 模型参数
    i32 m_legHeight = 6;
    f32 m_scale = 0.0f;
};

} // namespace mc::client::renderer::entity::model
