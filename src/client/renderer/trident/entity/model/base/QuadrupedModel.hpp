#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"

namespace mc::client::renderer::entity::model {

/**
 * @brief 四足动物模型基类
 *
 * 用于猪、牛、羊等四足动物的模型基类。
 * 参考 MC 1.16.5 QuadrupedModel
 */
class QuadrupedModel : public EntityModel {
public:
    QuadrupedModel();
    ~QuadrupedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

protected:
    /**
     * @brief 设置模型部件
     */
    virtual void setupParts();

    // 模型部件
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
};

} // namespace mc::client::renderer::entity::model
