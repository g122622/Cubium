#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::projectile {

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
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

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
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

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
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_cube;
    std::shared_ptr<ModelRenderer> m_glass;
    std::shared_ptr<ModelRenderer> m_base;
};

/**
 * @brief 光灵箭模型
 *
 * 参考 MC 1.16.5 SpectralArrowModel (same as ArrowModel)
 */
class SpectralArrowModel : public EntityModel {
public:
    SpectralArrowModel();
    ~SpectralArrowModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

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
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
};

/**
 * @brief 龙火球模型
 */
class DragonFireballModel : public EntityModel {
public:
    DragonFireballModel();
    ~DragonFireballModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_core;
    std::shared_ptr<ModelRenderer> m_outer;
};

/**
 * @brief 唤魔者尖牙模型
 */
class EvokerFangsModel : public EntityModel {
public:
    EvokerFangsModel();
    ~EvokerFangsModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_fangs[4];
};

} // namespace mc::client::renderer::entity::model::projectile
