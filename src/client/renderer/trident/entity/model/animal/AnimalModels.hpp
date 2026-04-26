#pragma once

#include "../core/EntityModel.hpp"
#include "../core/AgeableModel.hpp"
#include "../base/QuadrupedModel.hpp"

namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猪模型
 *
 * 参考 MC 1.16.5 PigModel
 */
class PigModel : public QuadrupedModel {
public:
    PigModel();
    ~PigModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;
};

/**
 * @brief 牛模型
 *
 * 参考 MC 1.16.5 CowModel
 */
class CowModel : public QuadrupedModel {
public:
    CowModel();
    ~CowModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
};

/**
 * @brief 羊模型
 *
 * 参考 MC 1.16.5 SheepModel
 */
class SheepModel : public QuadrupedModel {
public:
    SheepModel();
    ~SheepModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置羊毛状态
     * @param hasWool 是否有羊毛
     */
    void setWool(bool hasWool) { m_hasWool = hasWool; }

private:
    bool m_hasWool = true;
};

/**
 * @brief 鸡模型
 *
 * 参考 MC 1.16.5 ChickenModel
 * 继承 AgeableModel 以支持幼体渲染
 */
class ChickenModel : public AgeableModel {
public:
    ChickenModel();
    ~ChickenModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

protected:
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_beak;      // 喙
    std::shared_ptr<ModelRenderer> m_wattle;    // 肉垂（下巴下面的红肉）
    std::shared_ptr<ModelRenderer> m_comb;      // 鸡冠
};

} // namespace mc::client::renderer::entity::model::animal
