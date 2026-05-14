#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::projectile {

/**
 * @brief 三叉戟模型
 *
 * 参考 MC 1.16.5 TridentModel
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
    void setupParts();

    std::shared_ptr<ModelRenderer> m_shaft;       // 主杆
    std::shared_ptr<ModelRenderer> m_crossbar;    // 横杆
    std::shared_ptr<ModelRenderer> m_leftProng;   // 左叉尖
    std::shared_ptr<ModelRenderer> m_middleProng; // 中叉尖
    std::shared_ptr<ModelRenderer> m_rightProng;  // 右叉尖 (镜像)
};

/**
 * @brief 潜影贝子弹模型
 *
 * 参考 MC 1.16.5 ShulkerBulletModel
 */
class ShulkerBulletModel : public EntityModel {
public:
    ShulkerBulletModel();
    ~ShulkerBulletModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_bullet;
};

/**
 * @brief 羊驼唾沫模型
 *
 * 参考 MC 1.16.5 LlamaSpitModel
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
    void setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_main;
};

/**
 * @brief 末影水晶模型
 *
 * 参考 MC 1.16.5 EnderCrystalModel (简化版)
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
    void setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_cube;
    std::shared_ptr<ModelRenderer> m_glass;
    std::shared_ptr<ModelRenderer> m_base;
};

/**
 * @brief 光灵箭模型
 *
 * 参考 MC 1.16.5 ArrowRenderer
 *
 * 注意：Java原版使用直接顶点绘制而非ModelRenderer。
 * 当前实现是一个简化的box模型近似。
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
    void setupParts();

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
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
};

/**
 * @brief 龙火球模型
 *
 * 参考 MC 1.16.5 DragonFireballRenderer
 *
 * 注意：Java原版使用直接顶点绘制而非ModelRenderer。
 * 当前实现是一个简化的box模型近似。
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
    void setupParts();

    std::shared_ptr<ModelRenderer> m_core;
    std::shared_ptr<ModelRenderer> m_outer;
};

/**
 * @brief 唤魔者尖牙模型
 *
 * 参考 MC 1.16.5 EvokerFangsModel
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
    void setupParts();

    std::shared_ptr<ModelRenderer> m_base;     // 底座
    std::shared_ptr<ModelRenderer> m_upperJaw; // 上颚
    std::shared_ptr<ModelRenderer> m_lowerJaw; // 下颚
};

} // namespace mc::client::renderer::entity::model::projectile
