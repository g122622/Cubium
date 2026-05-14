#pragma once

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 末影人模型
 *
 * 参考 MC 1.16.5 EndermanModel
 * 末影人身材高大，手臂和腿很长。
 */
class EndermanModel : public model::BipedModel {
public:
    EndermanModel();
    ~EndermanModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置携带状态
     */
    void setCarrying(bool carrying) { m_carrying = carrying; }

    /**
     * @brief 设置尖叫/攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    /**
     * @brief 获取携带状态
     */
    [[nodiscard]] bool isCarrying() const { return m_carrying; }

    /**
     * @brief 获取攻击状态
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

private:
    void setupParts() override;

    bool m_carrying = false;  // 携带方块状态
    bool m_attacking = false; // 尖叫/攻击状态
};

} // namespace mc::client::renderer::entity::model::monster
