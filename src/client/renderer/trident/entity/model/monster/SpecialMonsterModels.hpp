#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 凋灵模型
 *
 * 参考 MC 1.16.5 WitherModel
 */
class WitherModel : public EntityModel {
public:
    WitherModel();
    explicit WitherModel(f32 scale);
    ~WitherModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 3> m_upperBodyParts;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_heads;
};

/**
 * @brief 史莱姆模型
 *
 * 参考 MC 1.16.5 SlimeModel
 */
class SlimeModel : public EntityModel {
public:
    SlimeModel();
    explicit SlimeModel(i32 size);
    ~SlimeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightEye;
    std::shared_ptr<ModelRenderer> m_leftEye;
    std::shared_ptr<ModelRenderer> m_mouth;
    i32 m_size = 0;
};

/**
 * @brief 守卫者模型
 *
 * 参考 MC 1.16.5 GuardianModel
 */
class GuardianModel : public EntityModel {
public:
    GuardianModel();
    ~GuardianModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    void updateSpines(f64 ageInTicks, f64 spikeAnimation);

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_eye;
    std::array<std::shared_ptr<ModelRenderer>, 12> m_spines;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_tail;
};

/**
 * @brief 远古守卫者模型
 *
 * 参考 MC 1.16.5 GuardianModel（相同结构，不同纹理）
 */
class ElderGuardianModel : public GuardianModel {
public:
    ElderGuardianModel();
    ~ElderGuardianModel() override = default;
};

/**
 * @brief 潜影贝模型
 *
 * 参考 MC 1.16.5 ShulkerModel
 */
class ShulkerModel : public EntityModel {
public:
    ShulkerModel();
    ~ShulkerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getBase() const { return m_base; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getLid() const { return m_lid; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_base;
    std::shared_ptr<ModelRenderer> m_lid;
    std::shared_ptr<ModelRenderer> m_head;
};

/**
 * @brief 蠹虫模型
 *
 * 参考 MC 1.16.5 SilverfishModel
 */
class SilverfishModel : public EntityModel {
public:
    SilverfishModel();
    ~SilverfishModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 7> m_bodyParts;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_wings;
    std::array<f32, 7> m_zPlacement;
};

/**
 * @brief 末影螨模型
 *
 * 参考 MC 1.16.5 EndermiteModel
 */
class EndermiteModel : public EntityModel {
public:
    EndermiteModel();
    ~EndermiteModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 4> m_bodyParts;
};

} // namespace mc::client::renderer::entity::model::monster
