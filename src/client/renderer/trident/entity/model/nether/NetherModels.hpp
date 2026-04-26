#pragma once

#include "../core/EntityModel.hpp"
#include "../core/AgeableModel.hpp"

namespace mc::client::renderer::entity::model::nether {

/**
 * @brief 恶魂模型
 *
 * 参考 MC 1.16.5 GhastModel
 */
class GhastModel : public EntityModel {
public:
    GhastModel();
    ~GhastModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::array<std::shared_ptr<ModelRenderer>, 9> m_tentacles;
};

/**
 * @brief 岩浆怪模型
 *
 * 参考 MC 1.16.5 MagmaCubeModel
 * 由 8 个薄片状的 segments 和一个 core 组成
 */
class MagmaCubeModel : public EntityModel {
public:
    MagmaCubeModel();
    explicit MagmaCubeModel(i32 size);
    ~MagmaCubeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置挤压动画状态
     * @param squishFactor 挤压因子
     * @param prevSquishFactor 上一帧挤压因子
     */
    void setSquishFactor(f32 squishFactor, f32 prevSquishFactor);

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_core;
    std::array<std::shared_ptr<ModelRenderer>, 8> m_segments;
    i32 m_size = 1;
    f32 m_squishFactor = 0.0f;
    f32 m_prevSquishFactor = 0.0f;
};

/**
 * @brief 猪灵模型
 *
 * 参考 MC 1.16.5 PiglinModel
 * 继承自 PlayerModel/BipedModel，添加耳朵、鼻子等部件
 * 支持跳舞、弩持有、欣赏物品等动画
 */
class PiglinModel : public EntityModel {
public:
    PiglinModel();
    explicit PiglinModel(f32 scale, i32 textureWidth = 64, i32 textureHeight = 64);
    ~PiglinModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置动作状态
     */
    void setAction(i32 action) { m_action = action; }

    /**
     * @brief 设置是否左撇子
     */
    void setLeftHanded(bool leftHanded) { m_leftHanded = leftHanded; }

    // 动作枚举
    enum class Action {
        DEFAULT = 0,
        DANCING = 1,
        ATTACKING_WITH_MELEE_WEAPON = 2,
        CROSSBOW_HOLD = 3,
        CROSSBOW_CHARGE = 4,
        ADMIRING_ITEM = 5
    };

private:
    void setupParts(f32 scale);

    // 基础部件（继承自 BipedModel 风格）
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_headwear;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;

    // 猪灵特有部件
    std::shared_ptr<ModelRenderer> m_leftEar;   // 左耳
    std::shared_ptr<ModelRenderer> m_rightEar;  // 右耳
    std::shared_ptr<ModelRenderer> m_nose;      // 鼻子
    std::shared_ptr<ModelRenderer> m_leftEye;   // 左眼
    std::shared_ptr<ModelRenderer> m_rightEye;  // 右眼

    i32 m_action = 0;
    bool m_leftHanded = false;
};

/**
 * @brief 疣猪模型
 *
 * 参考 MC 1.16.5 BoarModel (用于 Hoglin 和 Zoglin)
 * 继承自 AgeableModel，支持幼体/成年体
 */
class BoarModel : public ::mc::client::renderer::entity::model::AgeableModel {
public:
    BoarModel();
    ~BoarModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

protected:
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    void setupParts();

    // 头部部件
    std::shared_ptr<ModelRenderer> m_head;           // 头部
    std::shared_ptr<ModelRenderer> m_leftTusk;       // 左獠牙
    std::shared_ptr<ModelRenderer> m_rightTusk;      // 右獠牙
    std::shared_ptr<ModelRenderer> m_leftEar;        // 左耳
    std::shared_ptr<ModelRenderer> m_rightEar;       // 右耳

    // 身体部件
    std::shared_ptr<ModelRenderer> m_body;           // 身体
    std::shared_ptr<ModelRenderer> m_mane;           // 鬃毛
    std::shared_ptr<ModelRenderer> m_rightFrontLeg;
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;
    std::shared_ptr<ModelRenderer> m_rightBackLeg;
    std::shared_ptr<ModelRenderer> m_leftBackLeg;
};

/**
 * @brief 炽足兽模型
 *
 * 参考 MC 1.16.5 StriderModel
 * 包含身体、腿和多个毛发/皮瓣部件
 */
class StriderModel : public EntityModel {
public:
    StriderModel();
    ~StriderModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;           // 身体
    std::shared_ptr<ModelRenderer> m_rightLeg;       // 右腿
    std::shared_ptr<ModelRenderer> m_leftLeg;        // 左腿

    // 6 个毛发/皮瓣部件
    std::shared_ptr<ModelRenderer> m_flapLeftBottom;   // 左下皮瓣
    std::shared_ptr<ModelRenderer> m_flapLeftMiddle;   // 左中皮瓣
    std::shared_ptr<ModelRenderer> m_flapLeftTop;      // 左上皮瓣
    std::shared_ptr<ModelRenderer> m_flapRightBottom;  // 右下皮瓣
    std::shared_ptr<ModelRenderer> m_flapRightMiddle;  // 右中皮瓣
    std::shared_ptr<ModelRenderer> m_flapRightTop;     // 右上皮瓣
};

} // namespace mc::client::renderer::entity::model::nether
