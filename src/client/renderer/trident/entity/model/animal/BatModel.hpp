#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 蝙蝠模型
 *
 * 参考 MC 1.16.5 BatModel
 */
class BatModel : public EntityModel {
public:
    BatModel();
    ~BatModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_outerRightWing;
    std::shared_ptr<ModelRenderer> m_outerLeftWing;
};

} // namespace mc::client::renderer::entity::model::animal
