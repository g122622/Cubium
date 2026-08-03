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

namespace mc::client::renderer::entity::model::projectile {

/**
 * @brief 三叉戟模型
 *
 * 纹理尺寸: 32x32
 * 结构: 主杆 + 横杆 + 左叉尖 + 中叉尖 + 右叉尖
 */
class TridentModel : public EntityModel {
public:
    TridentModel();
    ~TridentModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_shaft;       // 主杆
    std::shared_ptr<ModelRenderer> m_crossbar;    // 横杆
    std::shared_ptr<ModelRenderer> m_leftProng;   // 左叉尖
    std::shared_ptr<ModelRenderer> m_middleProng; // 中叉尖
    std::shared_ptr<ModelRenderer> m_rightProng;  // 右叉尖 (镜像)
};

/**
 * @brief 潜影贝子弹模型
 */
class ShulkerBulletModel : public EntityModel {
public:
    ShulkerBulletModel();
    ~ShulkerBulletModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_bullet;
};

/**
 * @brief 羊驼唾沫模型
 */
class LlamaSpitModel : public EntityModel {
public:
    LlamaSpitModel();
    explicit LlamaSpitModel(f32 scale);
    ~LlamaSpitModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_main;
};

/**
 * @brief 末影水晶模型
 */
class EnderCrystalModel : public EntityModel {
public:
    EnderCrystalModel();
    explicit EnderCrystalModel(f32 scale);
    ~EnderCrystalModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_cube;
    std::shared_ptr<ModelRenderer> m_glass;
    std::shared_ptr<ModelRenderer> m_base;
};

/**
 * @brief 光灵箭模型
 *
 * 注意：当前实现是一个简化的box模型近似。
 * 正确实现应该在渲染器中使用顶点缓冲区绘制：
 * - 缩放因子: 0.05625F
 * - 箭头尖端: 4个顶点绘制箭杆
 * - 箭杆: 4次90度旋转绘制4面
 */
class SpectralArrowModel : public EntityModel {
public:
    SpectralArrowModel();
    ~SpectralArrowModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_arrow;
};

/**
 * @brief 凋灵之首模型
 */
class WitherSkullModel : public EntityModel {
public:
    WitherSkullModel();
    ~WitherSkullModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_head;
};

/**
 * @brief 龙火球模型
 *
 * 注意：当前实现是一个简化的box模型近似。
 * 正确实现应该在渲染器中使用顶点缓冲区绘制：
 * - 缩放因子: 2.0F
 * - 4个顶点绘制一个面向相机的四边形
 */
class DragonFireballModel : public EntityModel {
public:
    DragonFireballModel();
    ~DragonFireballModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_core;
    std::shared_ptr<ModelRenderer> m_outer;
};

/**
 * @brief 唤魔者尖牙模型
 *
 * 由 base、upperJaw、lowerJaw 三部分组成
 */
class EvokerFangsModel : public EntityModel {
public:
    EvokerFangsModel();
    ~EvokerFangsModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_base;     // 底座
    std::shared_ptr<ModelRenderer> m_upperJaw; // 上颚
    std::shared_ptr<ModelRenderer> m_lowerJaw; // 下颚
};

} // namespace mc::client::renderer::entity::model::projectile
