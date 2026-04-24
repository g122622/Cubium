#pragma once

#include "../base/QuadrupedModel.hpp"

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 兔子模型
 *
 * 参考 MC 1.16.5 RabbitModel
 */
class RabbitModel : public EntityModel {
public:
    RabbitModel();
    ~RabbitModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置跳跃进度
     */
    void setJumpProgress(f32 progress) { m_jumpProgress = progress; }

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightEar;
    std::shared_ptr<ModelRenderer> m_leftEar;
    std::shared_ptr<ModelRenderer> m_nose;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightFrontLeg;
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;
    std::shared_ptr<ModelRenderer> m_rightBackLeg;
    std::shared_ptr<ModelRenderer> m_leftBackLeg;
    std::shared_ptr<ModelRenderer> m_tail;

    f32 m_jumpProgress = 0.0f;
};

} // namespace mc::client::renderer::entity::model::animal
