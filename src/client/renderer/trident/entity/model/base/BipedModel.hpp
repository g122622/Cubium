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

#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model {

// 前向声明
namespace entity {
class LivingEntity;
}

/**
 * @brief 手的边（左/右）
 */
enum class HandSide { Left, Right };

/**
 * @brief 手臂姿态枚举
 *
 * 与 MC 1.21.11 HumanoidModel.ArmPose 对应（同姿态语义）：
 * - Empty/Item/Block/BowAndArrow/ThrowSpear/CrossbowCharge/CrossbowHold：原有姿态
 * - Spyglass：使用望远镜时第三人称将手臂贴近眼部前方
 * - Brush：使用刷子时第三人称刷扫姿态
 *
 * 注意：MC 1.21.11 中还有 TOOT_HORN 与 SPEAR 姿态，前者依赖山羊角物品与
 * UseAction::TootHorn（项目尚未实现），后者依赖 SpearAnimations 第三人称
 * 投掷长矛动画（项目 ThrowSpear 已覆盖三叉戟/长矛静态持握），暂不引入。
 */
enum class ArmPose { Empty, Item, Block, BowAndArrow, ThrowSpear, CrossbowCharge, CrossbowHold, Spyglass, Brush };

/**
 * @brief 双足动物模型基类
 *
 * 用于玩家、僵尸、骷髅等双足生物的模型基类，继承自 AgeableModel。
 */
class BipedModel : public AgeableModel {
public:
    BipedModel();
    /**
     * @brief 带参数的构造函数
     * @param scale 模型膨胀值
     * @param yOffset Y轴偏移
     * @param textureWidth 纹理宽度
     * @param textureHeight 纹理高度
     */
    BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
    ~BipedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     *
     * 用于设置游泳动画等状态
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置是否蹲伏
     */
    void setSneaking(bool sneaking) { m_isSneaking = sneaking; }

    /**
     * @brief 设置是否坐着
     */
    void setSitting(bool sitting) { m_isSitting = sitting; }

    /**
     * @brief 设置游泳动画进度
     */
    void setSwimAnimation(f32 animation) { m_swimAnimation = animation; }

    /**
     * @brief 设置挥动进度
     */
    void setSwingProgress(f32 progress) { m_swingProgress = progress; }

    /**
     * @brief 设置左手姿态
     */
    void setLeftArmPose(ArmPose pose) { m_leftArmPose = pose; }

    /**
     * @brief 设置右手姿态
     */
    void setRightArmPose(ArmPose pose) { m_rightArmPose = pose; }

    /**
     * @brief 设置主手
     */
    void setMainHand(HandSide hand) { m_mainHand = hand; }

    /**
     * @brief 设置挥动的手
     */
    void setSwingingHand(HandSide hand) { m_swingingHand = hand; }

    /**
     * @brief 设置鞘翅飞行过渡 tick 计数
     *
     * 历史遗留字段，对应 MC 1.21 之前 LivingEntity.fallFlyTicks。MC 1.21.11
     * HumanoidRenderState 已移除 fallFlyTicks，HumanoidModel.setupAnim 仅检查
     * isFallFlying 布尔值，不再使用此字段驱动头部角度。
     *
     * Cubium 中 LivingEntity::tick() 已实现 fallFlyTicks 递增/归零逻辑
     * （对应 MC 1.21.11 LivingEntity.tick 末尾），用于 updateFallFlying()
     * 周期性触发 ELYTRA_GLIDE 游戏事件与装备损坏。渲染器可选择将此值
     * 推送给 BipedModel 以支持未来扩展（如自定义过渡动画），但 MC 1.21.11
     * 原版渲染器不读取此字段。
     */
    void setElytraFlyingTicks(i32 ticks) { m_elytraFlyingTicks = ticks; }

    /**
     * @brief 设置是否处于鞘翅飞行状态
     *
     * 对应 MC 1.21.11 HumanoidRenderState.isFallFlying（boolean），
     * 由渲染器在 setAngles 前从 ClientEntity::isFallFlying() /
     * Entity::isElytraFlying() 推送。控制头部角度（飞行时强制 -π/4）。
     */
    void setFallFlying(bool flying) { m_isFallFlying = flying; }

    /**
     * @brief 设置鞘翅飞行速度因子
     *
     * 对应 MC 1.21.11 HumanoidRenderState.speedValue，由渲染器在 setAngles 前
     * 调用 `elytra::computeSpeedValue(isFallFlying, velocity.lengthSquared())` 计算：
     *   - 默认 1.0
     *   - 鞘翅飞行时 speedValue = (velocity.lengthSquared() / 0.2)^3
     *   - 最终钳制到 [1.0, +∞)
     * 该值作为手臂/腿部摆动振幅的除数：值越大，摆动越慢（视觉上像风阻）。
     */
    void setSpeedValue(f32 speedValue) { m_speedValue = speedValue; }

    /**
     * @brief 设置是否真正游泳
     */
    void setActuallySwimming(bool swimming) { m_isActuallySwimming = swimming; }

    /**
     * @brief 设置弩装填进度（已使用 ticks，含 partialTick 插值）
     *
     * 对应 MC 1.21 HumanoidRenderState.ticksUsingItem，由渲染器在 setAngles 前
     * 调用 PlayerEntity::getTicksUsingItem(partialTick) 计算。
     * 仅当 m_leftArmPose 或 m_rightArmPose 为 CrossbowCharge 时使用。
     */
    void setCrossbowChargeTicks(f32 ticks) { m_crossbowChargeTicks = ticks; }

    /**
     * @brief 设置弩的最大装填时长（ticks）
     *
     * 对应 MC 1.21 HumanoidRenderState.maxCrossbowChargeDuration，
     * 由渲染器调用 CrossbowItem::getChargeTime(stack) 计算。
     * 用于将 m_crossbowChargeTicks 归一化为 [0,1] 进度比。
     */
    void setMaxCrossbowChargeDuration(f32 duration) { m_maxCrossbowChargeDuration = duration; }

    /**
     * @brief 设置所有部件可见性
     */
    void setVisible(bool visible);

    /**
     * @brief 复制模型属性到另一个模型
     */
    void copyModelAttributesTo(BipedModel& target) const;

    /**
     * @brief 获取指定边的手臂
     */
    std::shared_ptr<ModelRenderer> getArmForSide(HandSide side);

    /**
     * @brief 获取头部模型
     */
    std::shared_ptr<ModelRenderer> getModelHead() { return m_bipedHead; }

    /**
     * @brief 获取帽子层模型
     */
    std::shared_ptr<ModelRenderer> getModelHeadwear() { return m_bipedHeadwear; }

    /**
     * @brief 获取身体模型
     */
    std::shared_ptr<ModelRenderer> getModelBody() { return m_bipedBody; }

    /**
     * @brief 获取右臂模型
     */
    std::shared_ptr<ModelRenderer> getRightArm() { return m_bipedRightArm; }

    /**
     * @brief 获取左臂模型
     */
    std::shared_ptr<ModelRenderer> getLeftArm() { return m_bipedLeftArm; }

    /**
     * @brief 获取右腿模型
     */
    std::shared_ptr<ModelRenderer> getRightLeg() { return m_bipedRightLeg; }

    /**
     * @brief 获取左腿模型
     */
    std::shared_ptr<ModelRenderer> getLeftLeg() { return m_bipedLeftLeg; }

    /**
     * @brief 平移手部用于手持物品渲染
     *
     * 将矩阵变换到手臂的局部坐标系，使手持物品跟随手臂动画。
     * 变换顺序：平移到旋转点 → Z轴旋转 → Y轴旋转 → X轴旋转
     *
     * 子类（如 PlayerModel）可 override 此方法以扩展纤细手臂等特殊偏移逻辑。
     *
     * @param handSide 手侧（左手或右手）
     * @param outMatrix 输出变换矩阵（4x4，行主序）
     */
    virtual void translateHand(HandSide handSide, std::array<f64, 16>& outMatrix) const;

protected:
    /**
     * @brief 设置模型部件
     */
    virtual void setupParts();

    /**
     * @brief 获取头部部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

    /**
     * @brief 处理右手姿态
     */
    virtual void handleRightArmPose();

    /**
     * @brief 处理左手姿态
     */
    virtual void handleLeftArmPose();

    /**
     * @brief 处理挥动动画
     * @param ageInTicks 年龄 ticks
     */
    virtual void handleSwingAnimation(f64 ageInTicks);

    /**
     * @brief 处理游泳动画
     */
    virtual void handleSwimAnimation(f64 limbSwing);

    /**
     * @brief 处理弩装填动画（双手协调）
     *
     * 对应 MC 1.21 AnimationUtils.animateCrossbowCharge。同时设置主手（持弩手）
     * 与副手（拉弦手）的角度，副手角度随装填进度从初始位置 lerp 到拉弦完成位置。
     *
     * @param isRightHanded true 表示主手为右手
     */
    void handleCrossbowCharge(bool isRightHanded);

    /**
     * @brief 处理弩持握动画（双手协调）
     *
     * 对应 MC 1.21 AnimationUtils.animateCrossbowHold。同时设置主副手角度，
     * 让模型呈双手持弩瞄准姿态。
     *
     * @param isRightHanded true 表示主手为右手
     */
    void handleCrossbowHold(bool isRightHanded);

    /**
     * @brief 角度插值（弧度）
     */
    static f32 rotLerpRad(f32 angle, f64 maxAngle, f64 target);

    /**
     * @brief 获取手臂角度平方
     */
    static f32 getArmAngleSq(f32 limbSwing);

    /**
     * @brief 获取主手
     */
    HandSide getMainHand() const;

    // 模型部件
    std::shared_ptr<ModelRenderer> m_bipedHead;
    std::shared_ptr<ModelRenderer> m_bipedHeadwear;
    std::shared_ptr<ModelRenderer> m_bipedBody;
    std::shared_ptr<ModelRenderer> m_bipedRightArm;
    std::shared_ptr<ModelRenderer> m_bipedLeftArm;
    std::shared_ptr<ModelRenderer> m_bipedRightLeg;
    std::shared_ptr<ModelRenderer> m_bipedLeftLeg;

    // 子类使用的别名引用
    std::shared_ptr<ModelRenderer>& m_head = m_bipedHead;
    std::shared_ptr<ModelRenderer>& m_headwear = m_bipedHeadwear;
    std::shared_ptr<ModelRenderer>& m_body = m_bipedBody;
    std::shared_ptr<ModelRenderer>& m_rightArm = m_bipedRightArm;
    std::shared_ptr<ModelRenderer>& m_leftArm = m_bipedLeftArm;
    std::shared_ptr<ModelRenderer>& m_rightLeg = m_bipedRightLeg;
    std::shared_ptr<ModelRenderer>& m_leftLeg = m_bipedLeftLeg;

    // 模型参数
    f32 m_modelScale = 0.0f;
    f32 m_yOffset = 0.0f;

    // 状态
    bool m_isSneaking = false;
    bool m_isSitting = false;
    f32 m_swimAnimation = 0.0f;
    f32 m_swingProgress = 0.0f;
    ArmPose m_leftArmPose = ArmPose::Empty;
    ArmPose m_rightArmPose = ArmPose::Empty;
    HandSide m_mainHand = HandSide::Right;
    HandSide m_swingingHand = HandSide::Right;
    // 历史遗留字段：MC 1.21.11 HumanoidRenderState 已无 fallFlyTicks，
    // HumanoidModel.setupAnim 仅检查 isFallFlying 布尔。Cubium 中 LivingEntity::tick
    // 已实现 fallFlyTicks 递增逻辑（用于服务端 updateFallFlying 周期触发），
    // 但渲染器不读取此字段（与 MC 1.21.11 行为一致）。
    i32 m_elytraFlyingTicks = 0;
    bool m_isActuallySwimming = false;
    // 鞘翅飞行状态（对应 MC 1.21.11 HumanoidRenderState.isFallFlying）
    bool m_isFallFlying = false;
    // 鞘翅飞行速度因子（对应 MC 1.21.11 HumanoidRenderState.speedValue）
    // 默认 1.0；鞘翅飞行时为 (velocity.lengthSquared() / 0.2)^3，钳制到 [1.0, +∞)
    f32 m_speedValue = 1.0f;

    // 弩装填动画状态
    // m_crossbowChargeTicks: 已使用 ticks（含 partialTick 插值），对应 MC HumanoidRenderState.ticksUsingItem
    // m_maxCrossbowChargeDuration: 弩最大装填时长（ticks），对应 MC HumanoidRenderState.maxCrossbowChargeDuration
    f32 m_crossbowChargeTicks = 0.0f;
    f32 m_maxCrossbowChargeDuration = 0.0f;
};

} // namespace mc::client::renderer::entity::model
