/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::aquatic {

/**
 * @brief 鳕鱼模型
 */
class CodModel : public EntityModel {
public:
    CodModel();
    ~CodModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_finTop; // 背鳍
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_headFront; // 头部前端
    std::shared_ptr<ModelRenderer> m_finRight;  // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;   // 左鳍
    std::shared_ptr<ModelRenderer> m_tail;

    bool m_isInWater = true;
};

/**
 * @brief 鲑鱼模型
 */
class SalmonModel : public EntityModel {
public:
    SalmonModel();
    ~SalmonModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_bodyFront; // 身体前部
    std::shared_ptr<ModelRenderer> m_bodyRear;  // 身体后部
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_finRight;   // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;    // 左鳍
    std::shared_ptr<ModelRenderer> m_tail;       // 尾巴（子部件）
    std::shared_ptr<ModelRenderer> m_dorsalFin;  // 背鳍（子部件）
    std::shared_ptr<ModelRenderer> m_ventralFin; // 腹鳍（子部件）

    bool m_isInWater = true;
};

/**
 * @brief 海豚模型
 *
 * textureWidth = 64, textureHeight = 64
 *
 * 运动状态推送：
 *   EntityRendererManager::_applyDolphinMotionState 从 ClientEntity::velocity() 计算
 *   水平速度平方（horizontalDistanceSqr = vx*vx + vz*vz，对应 MC 1.21.11
 *   DolphinRenderer 中 isMoving = deltaMovement.horizontalDistanceSqr() > 1.0E-7），
 *   通过 setMotionMagnitude 推送。setAngles 中根据 m_motionMagnitude 是否超过
 *   MOTION_THRESHOLD (1.0E-7) 判断是否播放游泳摆尾动画。
 */
class DolphinModel : public EntityModel {
public:
    DolphinModel();
    ~DolphinModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置运动向量水平模长的平方
     *
     * 对应 MC 1.21.11 DolphinRenderer 中：
     *   p_364903_.isMoving = p_480257_.getDeltaMovement().horizontalDistanceSqr() > 1.0E-7
     * 由 EntityRendererManager::_applyDolphinMotionState 在 setAngles 之前推送，
     * setAngles 依据该值是否超过 MOTION_THRESHOLD 决定是否播放游泳摆尾动画。
     *
     * @param magnitude 水平速度平方（vx*vx + vz*vz）
     */
    void setMotionMagnitude(f64 magnitude) { m_motionMagnitude = magnitude; }

    /**
     * @brief 运动判定阈值，对应 MC 1.21.11 DolphinRenderer 中的 1.0E-7
     *
     * 当 m_motionMagnitude > MOTION_THRESHOLD 时判定海豚正在移动，播放游泳摆尾动画；
     * 否则恢复静态尾巴角度。公开此常量便于单元测试验证阈值边界行为。
     */
    static constexpr f64 MOTION_THRESHOLD = 1.0E-7;

    // ========== 部件访问器（供单元测试验证摆尾角度） ==========
    // 返回裸指针而非 shared_ptr，避免测试用例意外修改部件引用计数。
    // 这些访问器仅用于读取 setAngles 写入的 rotateAngleX/Y 值。
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& body() const { return m_body; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& tail() const { return m_tail; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& tailFin() const { return m_tailFin; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& dorsalFin() const { return m_dorsalFin; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& finRight() const { return m_finRight; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& finLeft() const { return m_finLeft; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& head() const { return m_head; }
    [[nodiscard]] const std::shared_ptr<ModelRenderer>& nose() const { return m_nose; }

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_body;      // 身体
    std::shared_ptr<ModelRenderer> m_tail;      // 尾巴
    std::shared_ptr<ModelRenderer> m_tailFin;   // 尾鳍
    std::shared_ptr<ModelRenderer> m_dorsalFin; // 背鳍
    std::shared_ptr<ModelRenderer> m_finRight;  // 右鳍
    std::shared_ptr<ModelRenderer> m_finLeft;   // 左鳍
    std::shared_ptr<ModelRenderer> m_head;      // 头部（子部件）
    std::shared_ptr<ModelRenderer> m_nose;      // 鼻子（子部件）

    f64 m_motionMagnitude = 0.0;
};

/**
 * @brief 海龟模型
 *
 * 继承自 QuadrupedModel，有特殊的怀孕状态
 */
class TurtleModel : public EntityModel {
public:
    TurtleModel();
    explicit TurtleModel(f32 scale);
    ~TurtleModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

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
    void _setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_head;          // 头部
    std::shared_ptr<ModelRenderer> m_body;          // 身体
    std::shared_ptr<ModelRenderer> m_pregnant;      // 怀孕时的腹部
    std::shared_ptr<ModelRenderer> m_legBackRight;  // 右后腿
    std::shared_ptr<ModelRenderer> m_legBackLeft;   // 左后腿
    std::shared_ptr<ModelRenderer> m_legFrontRight; // 右前腿
    std::shared_ptr<ModelRenderer> m_legFrontLeft;  // 左前腿

    bool m_isInWater = true;
    bool m_isOnGround = false;
    bool m_isDigging = false;
    bool m_hasEgg = false;
    bool m_isChild = false;
};

/**
 * @brief 热带鱼模型抽象基类
 *
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
    void setColorMultipliers(f32 r, f32 g, f32 b)
    {
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
 * 纹理尺寸: 32x32
 * 结构: 身体 + 尾巴 + 右鳍 + 左鳍 + 背鳍
 */
class TropicalFishAModel : public AbstractTropicalFishModel {
public:
    explicit TropicalFishAModel(f32 scale = 0.0f);
    ~TropicalFishAModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void _setupParts(f32 scale);

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
 * 纹理尺寸: 32x32
 * 结构: 身体 + 尾巴 + 右鳍 + 左鳍 + 背鳍 + 腹鳍
 */
class TropicalFishBModel : public AbstractTropicalFishModel {
public:
    explicit TropicalFishBModel(f32 scale = 0.0f);
    ~TropicalFishBModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

private:
    void _setupParts(f32 scale);

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_tail;
    std::shared_ptr<ModelRenderer> m_finRight;
    std::shared_ptr<ModelRenderer> m_finLeft;
    std::shared_ptr<ModelRenderer> m_finTop;
    std::shared_ptr<ModelRenderer> m_finBottom;

    bool m_isInWater = true;
};

/**
 * @brief 美西螈模型
 *
 * 纹理尺寸: 64x64
 * 结构: 身体 + 头部 + 尾巴 + 四条腿 + 鳃（3对）
 * 支持变体纹理选择和水中/陆地动画
 */
class AxolotlModel : public EntityModel {
public:
    AxolotlModel();
    ~AxolotlModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否在水中
     */
    void setInWater(bool inWater) { m_isInWater = inWater; }

    /**
     * @brief 设置是否在地面
     */
    void setOnGround(bool onGround) { m_isOnGround = onGround; }

    /**
     * @brief 设置是否正在装死
     */
    void setPlayingDead(bool playingDead) { m_isPlayingDead = playingDead; }

    /**
     * @brief 设置是否为幼体
     */
    void setChild(bool isChild) { m_isChild = isChild; }

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;          // 身体
    std::shared_ptr<ModelRenderer> m_head;          // 头部
    std::shared_ptr<ModelRenderer> m_tail;          // 尾巴
    std::shared_ptr<ModelRenderer> m_leftHindLeg;   // 左后腿
    std::shared_ptr<ModelRenderer> m_rightHindLeg;  // 右后腿
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;  // 左前腿
    std::shared_ptr<ModelRenderer> m_rightFrontLeg; // 右前腿
    std::shared_ptr<ModelRenderer> m_topGills;      // 顶部鳃
    std::shared_ptr<ModelRenderer> m_leftGills;     // 左侧鳃
    std::shared_ptr<ModelRenderer> m_rightGills;    // 右侧鳃

    bool m_isInWater = true;
    bool m_isOnGround = false;
    bool m_isPlayingDead = false;
    bool m_isChild = false;
};

} // namespace mc::client::renderer::entity::model::aquatic
