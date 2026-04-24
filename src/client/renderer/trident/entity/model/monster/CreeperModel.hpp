#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 苦力怕模型
 *
 * 参考 MC 1.16.5 CreeperModel
 * 苦力怕有独特的四足身体和头部结构。
 */
class CreeperModel : public model::EntityModel {
public:
    CreeperModel();
    ~CreeperModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<model::ModelRenderer> m_head;
    std::shared_ptr<model::ModelRenderer> m_body;
    std::shared_ptr<model::ModelRenderer> m_legFrontRight;
    std::shared_ptr<model::ModelRenderer> m_legFrontLeft;
    std::shared_ptr<model::ModelRenderer> m_legBackRight;
    std::shared_ptr<model::ModelRenderer> m_legBackLeft;
};

} // namespace mc::client::renderer::entity::model::monster
