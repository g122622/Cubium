#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include <array>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 烈焰人模型
 *
 * 参考 MC 1.16.5 BlazeModel
 * 烈焰人由漂浮的头部和环绕的烟雾棒组成。
 */
class BlazeModel : public model::EntityModel {
public:
    BlazeModel();
    ~BlazeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置烟雾棒数量（12根）
     */
    static constexpr i32 SMOKE_ROD_COUNT = 12;

private:
    void setupParts();

    // 头部
    std::shared_ptr<model::ModelRenderer> m_head;

    // 烟雾棒（12根）
    std::array<std::shared_ptr<model::ModelRenderer>, SMOKE_ROD_COUNT> m_smokeRods;

    // 动画参数
    f64 m_ageInTicks = 0.0;
};

} // namespace mc::client::renderer::entity::model::monster
