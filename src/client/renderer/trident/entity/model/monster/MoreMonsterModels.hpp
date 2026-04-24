#pragma once

#include "../core/EntityModel.hpp"
#include "../base/BipedModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 灾厄村民模型
 *
 * 参考 MC 1.16.5 IllagerModel
 */
class IllagerModel : public EntityModel {
public:
    IllagerModel();
    explicit IllagerModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
    ~IllagerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHat() const { return m_hat; }

protected:
    void setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_hat;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_arms;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
};

/**
 * @brief 恼鬼模型
 *
 * 参考 MC 1.16.5 VexModel (extends BipedModel)
 */
class VexModel : public BipedModel {
public:
    VexModel();
    ~VexModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightWing;
};

/**
 * @brief 铁傀儡模型
 *
 * 参考 MC 1.16.5 IronGolemModel
 */
class IronGolemModel : public EntityModel {
public:
    IronGolemModel();
    ~IronGolemModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getRightArm() const { return m_rightArm; }

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_rightLeg;
};

/**
 * @brief 雪傀儡模型
 *
 * 参考 MC 1.16.5 SnowManModel
 */
class SnowGolemModel : public EntityModel {
public:
    SnowGolemModel();
    ~SnowGolemModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_bottomBody;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightHand;
    std::shared_ptr<ModelRenderer> m_leftHand;
};

/**
 * @brief 蜜蜂模型
 *
 * 参考 MC 1.16.5 BeeModel
 */
class BeeModel : public EntityModel {
public:
    BeeModel();
    ~BeeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_torso;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_frontLegs;
    std::shared_ptr<ModelRenderer> m_middleLegs;
    std::shared_ptr<ModelRenderer> m_backLegs;
    std::shared_ptr<ModelRenderer> m_stinger;
    std::shared_ptr<ModelRenderer> m_leftAntenna;
    std::shared_ptr<ModelRenderer> m_rightAntenna;
};

/**
 * @brief 狐狸模型
 *
 * 参考 MC 1.16.5 FoxModel
 */
class FoxModel : public EntityModel {
public:
    FoxModel();
    ~FoxModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightEar;
    std::shared_ptr<ModelRenderer> m_leftEar;
    std::shared_ptr<ModelRenderer> m_snout;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
    std::shared_ptr<ModelRenderer> m_tail;
};

/**
 * @brief 熊猫模型
 *
 * 参考 MC 1.16.5 PandaModel (extends QuadrupedModel)
 */
class PandaModel : public EntityModel {
public:
    PandaModel();
    explicit PandaModel(i32 textureOffset, f32 scale);
    ~PandaModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts(i32 textureOffset, f32 scale);

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
};

/**
 * @brief 鹦鹉模型
 *
 * 参考 MC 1.16.5 ParrotModel
 */
class ParrotModel : public EntityModel {
public:
    ParrotModel();
    ~ParrotModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_tail;
    std::shared_ptr<ModelRenderer> m_wingLeft;
    std::shared_ptr<ModelRenderer> m_wingRight;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_head2;
    std::shared_ptr<ModelRenderer> m_beak1;
    std::shared_ptr<ModelRenderer> m_beak2;
    std::shared_ptr<ModelRenderer> m_feather;
    std::shared_ptr<ModelRenderer> m_legLeft;
    std::shared_ptr<ModelRenderer> m_legRight;
};

/**
 * @brief 幻翼模型
 *
 * 参考 MC 1.16.5 PhantomModel
 */
class PhantomModel : public EntityModel {
public:
    PhantomModel();
    ~PhantomModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_leftWingBody;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightWingBody;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_tail1;
    std::shared_ptr<ModelRenderer> m_tail2;
};

/**
 * @brief 劫掠兽模型
 *
 * 参考 MC 1.16.5 RavagerModel
 */
class RavagerModel : public EntityModel {
public:
    RavagerModel();
    ~RavagerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_jaw;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_neck;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
};

} // namespace mc::client::renderer::entity::model::monster
