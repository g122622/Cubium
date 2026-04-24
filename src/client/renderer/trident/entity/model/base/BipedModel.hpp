#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"

namespace mc::client::renderer::entity::model {

/**
 * @brief 双足动物模型基类
 *
 * 用于玩家、僵尸、骷髅等双足生物的模型基类。
 * 参考 MC 1.16.5 BipedModel
 */
class BipedModel : public EntityModel {
public:
    BipedModel();
    /**
     * @brief 带参数的构造函数
     * @param scale 模型缩放
     * @param yOffset Y轴偏移
     * @param textureWidth 纹理宽度
     * @param textureHeight 纹理高度
     */
    BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
    ~BipedModel() override = default;

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
    std::shared_ptr<ModelRenderer> m_headwear;    // 帽子层
    std::shared_ptr<ModelRenderer> m_headOverlay; // 别名，指向 m_headwear
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
};

} // namespace mc::client::renderer::entity::model
