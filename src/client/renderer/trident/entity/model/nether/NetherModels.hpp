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

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::nether {

/**
 * @brief 恶魂模型
 */
class GhastModel : public EntityModel {
public:
    GhastModel();
    ~GhastModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::array<std::shared_ptr<ModelRenderer>, 9> m_tentacles;
};

/**
 * @brief 岩浆怪模型
 *
 * 由 8 个薄片状的 segments 和一个 core 组成
 */
class MagmaCubeModel : public EntityModel {
public:
    MagmaCubeModel();
    explicit MagmaCubeModel(i32 size);
    ~MagmaCubeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置挤压动画状态
     * @param squishFactor 挤压因子
     * @param prevSquishFactor 上一帧挤压因子
     */
    void setSquishFactor(f32 squishFactor, f32 prevSquishFactor);

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_core;
    std::array<std::shared_ptr<ModelRenderer>, 8> m_segments;
    i32 m_size = 1;
    f32 m_squishFactor = 0.0f;
    f32 m_prevSquishFactor = 0.0f;
};

/**
 * @brief 猪灵模型
 *
 * 继承自 BipedModel，添加耳朵等部件
 * 猪灵使用标准手臂（宽度4），不是纤细手臂
 * 支持跳舞、弩持有、欣赏物品等动画
 */
class PiglinModel : public ::mc::client::renderer::entity::model::BipedModel {
public:
    PiglinModel();
    explicit PiglinModel(f32 scale, i32 textureWidth = 64, i32 textureHeight = 64);
    ~PiglinModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置动作状态
     */
    void setAction(i32 action) { m_action = action; }

    /**
     * @brief 设置是否左撇子
     */
    void setLeftHanded(bool leftHanded) { m_leftHanded = leftHanded; }

    /**
     * @brief 复制角度到外层部件
     */
    void copyAnglesToWear();

    // 动作枚举
    enum class Action {
        DEFAULT = 0,
        DANCING = 1,
        ATTACKING_WITH_MELEE_WEAPON = 2,
        CROSSBOW_HOLD = 3,
        CROSSBOW_CHARGE = 4,
        ADMIRING_ITEM = 5
    };

protected:
    void handleRightArmPose() override;
    void handleLeftArmPose() override;

private:
    // TODO: setupPiglinParts 尚未使用，猪灵部件设置目前内联在构造函数中，需要重构提取到此方法
    void _setupPiglinParts(f32 scale);

    // 猪灵特有部件（耳朵）
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_leftEar;  // 左耳
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_rightEar; // 右耳

    // 外观层部件引用（从 PlayerModel 风格）
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bipedLeftArmwear;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bipedRightArmwear;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bipedLeftLegwear;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bipedRightLegwear;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bipedBodyWear;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bipedHeadwearPiglin;

    i32 m_action = 0;
    bool m_leftHanded = false;
};

/**
 * @brief 疣猪模型
 *
 * 继承自 AgeableModel，支持幼体/成年体。
 * 攻击动画期间头部 X 旋转从 DEFAULT_HEAD_X_ROT(0.87266463) 插值到
 * ATTACK_HEAD_X_ROT_END(-PI/9 ≈ -0.34906584)，形成甩头效果。
 */
class BoarModel : public ::mc::client::renderer::entity::model::AgeableModel {
public:
    BoarModel();
    ~BoarModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置撞飞攻击动画剩余 tick
     *
     * 对应 MC 原版 HoglinBase.getAttackAnimationRemainingTicks()。
     * 收到 HoglinAttack 状态包时设为 10，每tick递减，0表示动画结束。
     * 攻击动画期间头部 X 旋转从 DEFAULT_HEAD_X_ROT 插值到 ATTACK_HEAD_X_ROT_END。
     *
     * @param ticks 剩余 tick 数（0 = 无攻击动画）
     */
    void setAttackAnimationTicks(i32 ticks) { m_attackAnimationTicks = ticks; }

protected:
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

private:
    void _setupParts();

    // 攻击动画常量（对应 MC 原版 HoglinModel）
    static constexpr f32 DEFAULT_HEAD_X_ROT = 0.87266463f;     // 头部默认 X 旋转（约 50 度）
    static constexpr f32 ATTACK_HEAD_X_ROT_END = -0.34906584f; // 攻击时头部 X 旋转终点（-PI/9 ≈ -20 度）
    static constexpr i32 ATTACK_ANIMATION_DURATION = 10;       // 攻击动画持续 tick 数

    // 攻击动画状态
    i32 m_attackAnimationTicks = 0;

    // 头部部件
    std::shared_ptr<ModelRenderer> m_head;      // 头部
    std::shared_ptr<ModelRenderer> m_leftTusk;  // 左獠牙
    std::shared_ptr<ModelRenderer> m_rightTusk; // 右獠牙
    std::shared_ptr<ModelRenderer> m_leftEar;   // 左耳
    std::shared_ptr<ModelRenderer> m_rightEar;  // 右耳

    // 身体部件
    std::shared_ptr<ModelRenderer> m_body; // 身体
    std::shared_ptr<ModelRenderer> m_mane; // 鬃毛
    std::shared_ptr<ModelRenderer> m_rightFrontLeg;
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;
    std::shared_ptr<ModelRenderer> m_rightBackLeg;
    std::shared_ptr<ModelRenderer> m_leftBackLeg;
};

/**
 * @brief 炽足兽模型
 *
 * 包含身体、腿和多个毛发/皮瓣部件
 */
class StriderModel : public EntityModel {
public:
    StriderModel();
    ~StriderModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否有乘客
     */
    void setHasPassengers(bool hasPassengers) { m_hasPassengers = hasPassengers; }

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;     // 身体
    std::shared_ptr<ModelRenderer> m_rightLeg; // 右腿
    std::shared_ptr<ModelRenderer> m_leftLeg;  // 左腿

    // 6 个毛发/皮瓣部件
    std::shared_ptr<ModelRenderer> m_flapLeftBottom;  // 左下皮瓣
    std::shared_ptr<ModelRenderer> m_flapLeftMiddle;  // 左中皮瓣
    std::shared_ptr<ModelRenderer> m_flapLeftTop;     // 左上皮瓣
    std::shared_ptr<ModelRenderer> m_flapRightBottom; // 右下皮瓣
    std::shared_ptr<ModelRenderer> m_flapRightMiddle; // 右中皮瓣
    std::shared_ptr<ModelRenderer> m_flapRightTop;    // 右上皮瓣

    bool m_hasPassengers = false; // 是否有乘客
};

} // namespace mc::client::renderer::entity::model::nether
