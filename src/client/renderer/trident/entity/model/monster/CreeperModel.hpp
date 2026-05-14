#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 苦力怕模型
 *
 * 参考 MC 1.16.5 CreeperModel
 * 苦力怕有独特的四足身体和头部结构。
 * 包含 creeperArmor 部件用于闪电苦力怕的充能效果。
 */
class CreeperModel : public model::EntityModel {
public:
    CreeperModel();
    explicit CreeperModel(f32 scale);
    ~CreeperModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void renderArmor(f64 scale = 1.0f / 16.0f);

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 获取盔甲层模型
     * 用于闪电苦力怕的充能光效渲染
     */
    [[nodiscard]] std::shared_ptr<model::ModelRenderer> getArmorHead() const { return m_armorHead; }

private:
    void setupParts(f32 scale);

    std::shared_ptr<model::ModelRenderer> m_head;
    std::shared_ptr<model::ModelRenderer> m_body;
    std::shared_ptr<model::ModelRenderer> m_legFrontRight;
    std::shared_ptr<model::ModelRenderer> m_legFrontLeft;
    std::shared_ptr<model::ModelRenderer> m_legBackRight;
    std::shared_ptr<model::ModelRenderer> m_legBackLeft;

    // 盔甲层（用于闪电苦力怕充能效果）
    std::shared_ptr<model::ModelRenderer> m_armorHead;
};

} // namespace mc::client::renderer::entity::model::monster
