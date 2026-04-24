#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::nether {

/**
 * @brief 恶魂模型
 *
 * 参考 MC 1.16.5 GhastModel
 */
class GhastModel : public EntityModel {
public:
    GhastModel();
    ~GhastModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::array<std::shared_ptr<ModelRenderer>, 9> m_tentacles;
};

/**
 * @brief 岩浆怪模型
 *
 * 参考 MC 1.16.5 MagmaCubeModel
 */
class MagmaCubeModel : public EntityModel {
public:
    MagmaCubeModel();
    explicit MagmaCubeModel(i32 size);
    ~MagmaCubeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_core;
    std::array<std::shared_ptr<ModelRenderer>, 8> m_segments;
    i32 m_size = 1;
};

/**
 * @brief 猪灵模型
 *
 * 参考 MC 1.16.5 PiglinModel
 */
class PiglinModel : public EntityModel {
public:
    PiglinModel();
    ~PiglinModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
};

/**
 * @brief 疣猪模型
 *
 * 参考 MC 1.16.5 BoarModel (用于 Hoglin 和 Zoglin)
 */
class BoarModel : public EntityModel {
public:
    BoarModel();
    ~BoarModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightFrontLeg;
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;
    std::shared_ptr<ModelRenderer> m_rightBackLeg;
    std::shared_ptr<ModelRenderer> m_leftBackLeg;
};

/**
 * @brief 炽足兽模型
 *
 * 参考 MC 1.16.5 StriderModel
 */
class StriderModel : public EntityModel {
public:
    StriderModel();
    ~StriderModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
};

} // namespace mc::client::renderer::entity::model::nether
