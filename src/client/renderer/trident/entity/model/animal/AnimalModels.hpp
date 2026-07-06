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

#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include <memory>

// TODO 把这个文件拆成多个文件，每个模型一个文件，放在 model/animal 目录下
namespace mc::client::renderer::entity::model::animal {

/**
 * @brief 猪模型
 */
class PigModel : public QuadrupedModel {
public:
    PigModel();
    ~PigModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;
};

/**
 * @brief 牛模型
 */
class CowModel : public QuadrupedModel {
public:
    CowModel();
    ~CowModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;
};

/**
 * @brief 羊模型
 *
 * 支持吃草动画（头部低头/摆动），参考 MC 1.16.5 SheepModel。
 * 吃草动画由 eatAnimationTimer (0-40) 驱动：
 * - 0: 无动画，头部恢复正常姿态
 * - 1-3: 头部逐渐低下的过渡阶段
 * - 4-36: 头部保持低位并左右摆动
 * - 37-40: 头部逐渐抬起的过渡阶段
 */
class SheepModel : public QuadrupedModel {
public:
    SheepModel();
    ~SheepModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置生物动画状态（每帧调用）
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

    /**
     * @brief 设置羊毛状态
     * @param hasWool 是否有羊毛
     */
    void setWool(bool hasWool) { m_hasWool = hasWool; }

    /**
     * @brief 设置吃草动画计时器
     * @param eatAnimationTimer 吃草动画计时器 (0-40 ticks)
     *
     * 参考 MC Sheep.eatAnimationTick：收到状态码 10 时设为 40，每 tick 递减 1。
     */
    void setEatAnimationTimer(i32 eatAnimationTimer) { m_eatAnimationTimer = eatAnimationTimer; }

    /**
     * @brief 计算头部吃草 Y 位置缩放因子
     *
     * 参考 MC Sheep.getHeadEatPositionScale：
     * - eatAnimationTick <= 0: 返回 0.0（正常姿态）
     * - eatAnimationTick >= 4 && <= 36: 返回 1.0（完全低头）
     * - eatAnimationTick < 4: 过渡阶段 (eatAnimationTick - partialTick) / 4.0
     * - eatAnimationTick > 36: 恢复阶段 -(eatAnimationTick - 40 - partialTick) / 4.0
     */
    [[nodiscard]] f32 getHeadEatPositionScale(f32 partialTick) const;

    /**
     * @brief 计算头部吃草 X 旋转角度
     *
     * 参考 MC Sheep.getHeadEatAngleScale：
     * - eatAnimationTick > 4 && <= 36: PI/5 + 摆动 (sin 驱动)
     * - eatAnimationTick > 0: PI/5（低头但不摆动）
     * - eatAnimationTick <= 0: headPitch（恢复正常）
     */
    [[nodiscard]] f32 getHeadEatAngleScale(f32 partialTick) const;

private:
    bool m_hasWool = true;
    i32 m_eatAnimationTimer = 0; // 吃草动画计时器 (0-40)
    f64 m_headPitch = 0.0;       // 保存的头部俯仰角（用于恢复姿态）
    f32 m_partialTick = 0.0f;    // 保存的部分 tick（用于插值）

    static constexpr f32 EAT_ANIMATION_DURATION = 40.0f;
};

/**
 * @brief 鸡模型
 *
 * 继承 AgeableModel 以支持幼体渲染
 */
class ChickenModel : public AgeableModel {
public:
    ChickenModel();
    ~ChickenModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

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
    std::shared_ptr<ModelRenderer> m_beak;   // 喙
    std::shared_ptr<ModelRenderer> m_wattle; // 肉垂（下巴下面的红肉，对应 MC red_thing）
};

} // namespace mc::client::renderer::entity::model::animal
