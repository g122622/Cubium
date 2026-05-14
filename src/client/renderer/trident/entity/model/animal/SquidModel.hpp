#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 鱿鱼模型
 *
 * 参考 MC 1.16.5 SquidModel
 */
class SquidModel : public EntityModel {
public:
    SquidModel();
    ~SquidModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::array<std::shared_ptr<ModelRenderer>, 8> m_tentacles;
};

} // namespace mc::client::renderer::entity::model::animal
