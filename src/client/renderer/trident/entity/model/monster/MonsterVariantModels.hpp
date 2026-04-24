#pragma once

#include "../base/BipedModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 僵尸村民模型
 *
 * 参考 MC 1.16.5 ZombieVillagerModel
 */
class ZombieVillagerModel : public BipedModel {
public:
    ZombieVillagerModel();
    explicit ZombieVillagerModel(f32 scale, bool slim);
    ~ZombieVillagerModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    void setHeadVisible(bool visible);

private:
    void setupParts(f32 scale, bool slim);

    std::shared_ptr<ModelRenderer> m_villagerNose;
};

/**
 * @brief 溺尸模型
 *
 * 参考 MC 1.16.5 DrownedModel (extends ZombieModel)
 */
class DrownedModel : public BipedModel {
public:
    DrownedModel();
    explicit DrownedModel(f32 scale, bool slim);
    ~DrownedModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
};

/**
 * @brief 流浪者模型
 *
 * 参考 MC 1.16.5 SkeletonModel (same structure, different texture)
 */
class StrayModel : public BipedModel {
public:
    StrayModel();
    ~StrayModel() override = default;
};

/**
 * @brief 尸壳模型
 *
 * 参考 MC 1.16.5 ZombieModel (same structure, different texture)
 */
class HuskModel : public BipedModel {
public:
    HuskModel();
    ~HuskModel() override = default;
};

/**
 * @brief 洞穴蜘蛛模型
 *
 * 参考 MC 1.16.5 SpiderModel (same structure, smaller scale)
 */
class CaveSpiderModel : public EntityModel {
public:
    CaveSpiderModel();
    ~CaveSpiderModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_neck;
    std::shared_ptr<ModelRenderer> m_body;
    std::array<std::shared_ptr<ModelRenderer>, 8> m_legs;
};

/**
 * @brief 巨人模型
 *
 * 参考 MC 1.16.5 GiantModel (same as ZombieModel but larger)
 */
class GiantModel : public BipedModel {
public:
    GiantModel();
    ~GiantModel() override = default;
};

} // namespace mc::client::renderer::entity::model::monster
