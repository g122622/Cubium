#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include <array>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 蜘蛛模型
 *
 * 参考 MC 1.16.5 SpiderModel
 * 蜘蛛有 8 条腿，分为 4 对。
 */
class SpiderModel : public model::EntityModel {
public:
    SpiderModel();
    ~SpiderModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 获取头部模型
     */
    std::shared_ptr<model::ModelRenderer> getModelHead() { return m_head; }

private:
    void setupParts();

    std::shared_ptr<model::ModelRenderer> m_head;
    std::shared_ptr<model::ModelRenderer> m_neck;
    std::shared_ptr<model::ModelRenderer> m_body;
    std::array<std::shared_ptr<model::ModelRenderer>, 8> m_legs; // 8条腿
};

} // namespace mc::client::renderer::entity::model::monster
