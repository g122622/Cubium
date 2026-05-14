#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 村民模型
 *
 * 参考 MC 1.16.5 VillagerModel
 * 村民具有特殊的头部模型（大鼻子）、帽子和衣服。
 */
class VillagerModel : public EntityModel {
public:
    explicit VillagerModel(f32 scale = 0.0f);
    VillagerModel(f32 scale, i32 textureWidth, i32 textureHeight);
    ~VillagerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置摇头状态（交易不满意时）
     */
    void setShakingHead(bool shaking) { m_shakingHead = shaking; }

    /**
     * @brief 获取头部模型
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }

protected:
    // 头部部件
    std::shared_ptr<ModelRenderer> m_head;    // 头部
    std::shared_ptr<ModelRenderer> m_hat;     // 帽子
    std::shared_ptr<ModelRenderer> m_hatBrim; // 帽檐
    std::shared_ptr<ModelRenderer> m_nose;    // 大鼻子

    // 身体部件
    std::shared_ptr<ModelRenderer> m_body;     // 身体
    std::shared_ptr<ModelRenderer> m_clothing; // 衣服
    std::shared_ptr<ModelRenderer> m_arms;     // 手臂（交叉）

    // 腿部部件
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;

    bool m_shakingHead = false;
};

} // namespace mc::client::renderer::entity::model::animal
