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
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void setupParts();
    std::shared_ptr<ModelRenderer> m_body;        // 身体
    std::shared_ptr<ModelRenderer> m_tail;        // 尾巴
    std::shared_ptr<ModelRenderer> m_tailFin;     // 尾鳍
    std::shared_ptr<ModelRenderer> m_finRight;    // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;     // 左鳍
    std::shared_ptr<ModelRenderer> m_head;        // 头部（子部件）
    std::shared_ptr<ModelRenderer> m_nose;        // 鼻子（子部件）

    bool m_isInWater = true;
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
};

} // namespace mc::client::renderer::entity::model::aquatic
