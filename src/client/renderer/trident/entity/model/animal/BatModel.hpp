#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 蝙蝠模型
 *
 * 参考 MC 1.16.5 BatModel
 * 纹理尺寸: 64x64
 */
class BatModel : public EntityModel {
public:
    BatModel();
    ~BatModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置蝙蝠是否处于悬挂状态
     * @param hanging true 为悬挂，false 为飞行
     */
    void setHanging(bool hanging);

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightEar;
    std::shared_ptr<ModelRenderer> m_leftEar;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_outerRightWing;
    std::shared_ptr<ModelRenderer> m_outerLeftWing;

    bool m_isHanging = false;
};

} // namespace mc::client::renderer::entity::model::animal
