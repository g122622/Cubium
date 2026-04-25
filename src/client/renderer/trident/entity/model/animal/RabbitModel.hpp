#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 兔子模型
 *
 * 参考 MC 1.16.5 RabbitModel
 * 纹理尺寸: 64x32
 *
 * 部件：
 * - rabbitLeftFoot/rabbitRightFoot: 后脚
 * - rabbitLeftThigh/rabbitRightThigh: 大腿
 * - rabbitBody: 身体
 * - rabbitLeftArm/rabbitRightArm: 前腿
 * - rabbitHead: 头部
 * - rabbitRightEar/rabbitLeftEar: 耳朵
 * - rabbitTail: 尾巴
 * - rabbitNose: 鼻子
 */
class RabbitModel : public EntityModel {
public:
    RabbitModel();
    ~RabbitModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置跳跃旋转值
     * @param jumpRotation 跳跃旋转值 (0.0 - 1.0，通过 sin(jumpCompletion * PI) 计算)
     */
    void setJumpRotation(f32 jumpRotation);

    /**
     * @brief 设置是否为幼体
     */
    void setChild(bool isChild) { m_isChild = isChild; }

private:
    void setupParts();

    // 后脚
    std::shared_ptr<ModelRenderer> m_leftFoot;
    std::shared_ptr<ModelRenderer> m_rightFoot;
    // 大腿
    std::shared_ptr<ModelRenderer> m_leftThigh;
    std::shared_ptr<ModelRenderer> m_rightThigh;
    // 身体
    std::shared_ptr<ModelRenderer> m_body;
    // 前腿
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightArm;
    // 头部
    std::shared_ptr<ModelRenderer> m_head;
    // 耳朵
    std::shared_ptr<ModelRenderer> m_rightEar;
    std::shared_ptr<ModelRenderer> m_leftEar;
    // 尾巴
    std::shared_ptr<ModelRenderer> m_tail;
    // 鼻子
    std::shared_ptr<ModelRenderer> m_nose;

    f32 m_jumpRotation = 0.0f;
    bool m_isChild = false;
};

} // namespace mc::client::renderer::entity::model::animal
