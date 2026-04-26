#pragma once

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::aquatic {

/**
 * @brief 鳕鱼模型
 *
 * 参考 MC 1.16.5 CodModel
 */
class CodModel : public EntityModel {
public:
    CodModel();
    ~CodModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_finTop;      // 背鳍
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_headFront;   // 头部前端
    std::shared_ptr<ModelRenderer> m_finRight;    // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;     // 左鳍
    std::shared_ptr<ModelRenderer> m_tail;

    bool m_isInWater = true;
};

/**
 * @brief 鲑鱼模型
 *
 * 参考 MC 1.16.5 SalmonModel
 */
class SalmonModel : public EntityModel {
public:
    SalmonModel();
    ~SalmonModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_bodyFront;   // 身体前部
    std::shared_ptr<ModelRenderer> m_bodyRear;    // 身体后部
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_finRight;    // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;     // 左鳍
    std::shared_ptr<ModelRenderer> m_tail;        // 尾巴（子部件）
    std::shared_ptr<ModelRenderer> m_dorsalFin;   // 背鳍（子部件）
    std::shared_ptr<ModelRenderer> m_ventralFin;  // 腹鳍（子部件）

    bool m_isInWater = true;
};

/**
 * @brief 海豚模型
 *
 * 参考 MC 1.16.5 DolphinModel
 * textureWidth = 64, textureHeight = 64
 */
class DolphinModel : public EntityModel {
public:
    DolphinModel();
    ~DolphinModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中（已废弃，使用setMotionMagnitude）
     * @deprecated 使用 setMotionMagnitude 替代
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

    /**
     * @brief 设置运动向量模长的平方
     * Java 原版使用 Entity.horizontalMag(getMotion()) > 1.0E-7D 判断是否在移动
     */
    void setMotionMagnitude(f64 magnitude) { m_motionMagnitude = magnitude; }

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_body;        // 身体
    std::shared_ptr<ModelRenderer> m_tail;        // 尾巴
    std::shared_ptr<ModelRenderer> m_tailFin;     // 尾鳍
    std::shared_ptr<ModelRenderer> m_dorsalFin;   // 背鳍（新增）
    std::shared_ptr<ModelRenderer> m_finRight;    // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;     // 左鳍
    std::shared_ptr<ModelRenderer> m_head;        // 头部（子部件）
    std::shared_ptr<ModelRenderer> m_nose;        // 鼻子（子部件）

    bool m_isInWater = true;
    f64 m_motionMagnitude = 0.0;  // horizontalMag(motion)
};

/**
 * @brief 海龟模型
 *
 * 参考 MC 1.16.5 TurtleModel
 * 继承自 QuadrupedModel，有特殊的怀孕状态
 */
class TurtleModel : public EntityModel {
public:
    TurtleModel();
    explicit TurtleModel(f32 scale);
    ~TurtleModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

    /**
     * @brief 设置是否在地面
     */
    void setOnGround(bool onGround) { m_isOnGround = onGround; }

    /**
     * @brief 设置是否在挖掘
     */
    void setDigging(bool digging) { m_isDigging = digging; }

    /**
     * @brief 设置是否有蛋（怀孕状态）
     */
    void setHasEgg(bool hasEgg) { m_hasEgg = hasEgg; }

    /**
     * @brief 设置是否为幼体
     */
    void setChild(bool isChild) { m_isChild = isChild; }

private:
    void setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_head;         // 头部
    std::shared_ptr<ModelRenderer> m_body;         // 身体
    std::shared_ptr<ModelRenderer> m_pregnant;     // 怀孕时的腹部
    std::shared_ptr<ModelRenderer> m_legBackRight; // 右后腿
    std::shared_ptr<ModelRenderer> m_legBackLeft;  // 左后腿
    std::shared_ptr<ModelRenderer> m_legFrontRight;// 右前腿
    std::shared_ptr<ModelRenderer> m_legFrontLeft; // 左前腿

    bool m_isInWater = true;
    bool m_isOnGround = false;
    bool m_isDigging = false;
    bool m_hasEgg = false;
    bool m_isChild = false;
};

/**
 * @brief 热带鱼模型抽象基类
 *
 * 参考 MC 1.16.5 AbstractTropicalFishModel
 * 提供颜色乘数设置功能
 */
class AbstractTropicalFishModel : public EntityModel {
public:
    AbstractTropicalFishModel() = default;
    ~AbstractTropicalFishModel() override = default;

    /**
     * @brief 设置颜色乘数
     * @param r 红色乘数
     * @param g 绿色乘数
     * @param b 蓝色乘数
     */
    void setColorMultipliers(f32 r, f32 g, f32 b) {
        m_colorR = r;
        m_colorG = g;
        m_colorB = b;
    }

    void render(f64 scale = 1.0f / 16.0f) override;

protected:
    f32 m_colorR = 1.0f;
    f32 m_colorG = 1.0f;
    f32 m_colorB = 1.0f;
};

/**
 * @brief 热带鱼A型模型（小体型）
 *
 * 参考 MC 1.16.5 TropicalFishAModel
 * 纹理尺寸: 32x32
 * 结构: 身体 + 尾巴 + 右鳍 + 左鳍 + 背鳍
 */
class TropicalFishAModel : public AbstractTropicalFishModel {
public:
    explicit TropicalFishAModel(f32 scale = 0.0f);
    ~TropicalFishAModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_tail;
    std::shared_ptr<ModelRenderer> m_finRight;
    std::shared_ptr<ModelRenderer> m_finLeft;
    std::shared_ptr<ModelRenderer> m_finTop;

    bool m_isInWater = true;
};

/**
 * @brief 热带鱼B型模型（大体型）
 *
 * 参考 MC 1.16.5 TropicalFishBModel
 * 纹理尺寸: 32x32
 * 结构: 身体 + 尾巴 + 右鳍 + 左鳍 + 背鳍 + 腹鳍
 */
class TropicalFishBModel : public AbstractTropicalFishModel {
public:
    explicit TropicalFishBModel(f32 scale = 0.0f);
    ~TropicalFishBModel() override = default;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_tail;
    std::shared_ptr<ModelRenderer> m_finRight;
    std::shared_ptr<ModelRenderer> m_finLeft;
    std::shared_ptr<ModelRenderer> m_finTop;
    std::shared_ptr<ModelRenderer> m_finBottom;

    bool m_isInWater = true;
};

} // namespace mc::client::renderer::entity::model::aquatic
