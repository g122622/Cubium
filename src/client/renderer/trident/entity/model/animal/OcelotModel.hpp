#pragma once

#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猫豹/豹猫模型
 *
 * 参考 MC 1.16.5 OcelotModel
 * 猫模型继承自此类。
 */
class OcelotModel : public AgeableModel {
public:
    explicit OcelotModel(f32 scale = 0.0f);
    ~OcelotModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置动画状态
     * @param state 状态 (1=站立, 2=奔跑, 3=坐下/睡觉)
     */
    void setState(int state) { m_state = state; }

protected:
    // 模型部件
    std::shared_ptr<ModelRenderer> m_head;          // 头部
    std::shared_ptr<ModelRenderer> m_body;          // 身体
    std::shared_ptr<ModelRenderer> m_tail;          // 尾巴1
    std::shared_ptr<ModelRenderer> m_tail2;         // 尾巴2
    std::shared_ptr<ModelRenderer> m_backLeftLeg;   // 后左腿
    std::shared_ptr<ModelRenderer> m_backRightLeg;  // 后右腿
    std::shared_ptr<ModelRenderer> m_frontLeftLeg;  // 前左腿
    std::shared_ptr<ModelRenderer> m_frontRightLeg; // 前右腿

    int m_state = 1; // 1=站立, 2=奔跑, 3=坐下
};

} // namespace mc::client::renderer::entity::model::animal
