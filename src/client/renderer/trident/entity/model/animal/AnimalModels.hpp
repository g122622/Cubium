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

#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include <memory>

// TODO 把这个文件拆成多个文件，每个模型一个文件，放在 model/animal 目录下
namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猪模型
 */
class PigModel : public QuadrupedModel {
public:
    PigModel();
    ~PigModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;
};

/**
 * @brief 牛模型
 */
class CowModel : public QuadrupedModel {
public:
    CowModel();
    ~CowModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;
};

/**
 * @brief 羊模型
 */
class SheepModel : public QuadrupedModel {
public:
    SheepModel();
    ~SheepModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置羊毛状态
     * @param hasWool 是否有羊毛
     */
    void setWool(bool hasWool) { m_hasWool = hasWool; }

    /**
     * @brief 设置吃草动画状态
     * @param isEating 是否正在吃草
     * @param eatingTimer 吃草计时器 (0-40)
     */
    void setEatingGrass(bool isEating, i32 eatingTimer)
    {
        m_isEating = isEating;
        m_eatingTimer = eatingTimer;
    }

    /**
     * @brief 设置头部旋转角度（用于吃草动画）
     * @param headRotationPointY 头部 Y 旋转点偏移 (来自实体)
     * @param headRotationAngleX 头部 X 旋转角度 (来自实体)
     */
    void setHeadRotation(f32 headRotationPointY, f32 headRotationAngleX)
    {
        m_headRotationPointY = headRotationPointY;
        m_headRotationAngleX = headRotationAngleX;
    }

private:
    bool m_hasWool = true;
    bool m_isEating = false;
    i32 m_eatingTimer = 0;
    f32 m_headRotationPointY = 0.0f; // 来自实体的头部 Y 偏移
    f32 m_headRotationAngleX = 0.0f; // 来自实体的头部 X 旋转角度
};

/**
 * @brief 鸡模型
 *
 * 继承 AgeableModel 以支持幼体渲染
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
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_beak;   // 喙
    std::shared_ptr<ModelRenderer> m_wattle; // 肉垂（下巴下面的红肉）
    std::shared_ptr<ModelRenderer> m_comb;   // 鸡冠
};

} // namespace mc::client::renderer::entity::model::animal
