#pragma once

#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 马模型
 *
 * 参考 MC 1.16.5 HorseModel
 * 支持马、驴、骡、骷髅马、僵尸马。
 */
class HorseModel : public AgeableModel {
public:
    explicit HorseModel(f32 scale = 0.0f);
    ~HorseModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置马鞍状态
     */
    void setSaddled(bool saddled) { m_saddled = saddled; }

    /**
     * @brief 设置是否被骑乘
     */
    void setRidden(bool ridden) { m_ridden = ridden; }

protected:
    // 身体部件
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_head;

    // 腿部部件（两套用于站立和奔跑动画）
    std::shared_ptr<ModelRenderer> m_backLeftLeg;
    std::shared_ptr<ModelRenderer> m_backRightLeg;
    std::shared_ptr<ModelRenderer> m_frontLeftLeg;
    std::shared_ptr<ModelRenderer> m_frontRightLeg;
    std::shared_ptr<ModelRenderer> m_backLeftLegBaby;
    std::shared_ptr<ModelRenderer> m_backRightLegBaby;
    std::shared_ptr<ModelRenderer> m_frontLeftLegBaby;
    std::shared_ptr<ModelRenderer> m_frontRightLegBaby;

    // 尾巴
    std::shared_ptr<ModelRenderer> m_tail;

    // 鞍相关部件
    std::vector<std::shared_ptr<ModelRenderer>> m_saddleParts;
    std::vector<std::shared_ptr<ModelRenderer>> m_ridingParts;

    bool m_saddled = false;
    bool m_ridden = false;
};

} // namespace mc::client::renderer::entity::model::animal
