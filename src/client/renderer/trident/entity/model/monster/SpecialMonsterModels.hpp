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

#include "../core/EntityModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 凋灵模型
 *
 * 参考 MC 1.16.5 WitherModel
 */
class WitherModel : public EntityModel {
public:
    WitherModel();
    explicit WitherModel(f32 scale);
    ~WitherModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 3> m_upperBodyParts;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_heads;
};

/**
 * @brief 史莱姆模型
 *
 * 参考 MC 1.16.5 SlimeModel
 */
class SlimeModel : public EntityModel {
public:
    SlimeModel();
    explicit SlimeModel(i32 size);
    ~SlimeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightEye;
    std::shared_ptr<ModelRenderer> m_leftEye;
    std::shared_ptr<ModelRenderer> m_mouth;
    i32 m_size = 0;
};

/**
 * @brief 守卫者模型
 *
 * 参考 MC 1.16.5 GuardianModel
 */
class GuardianModel : public EntityModel {
public:
    GuardianModel();
    ~GuardianModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置尖刺动画值
     * @param animation 尖刺动画值 (0.0 - 1.0)，从实体获取
     */
    void setSpikeAnimation(f32 animation) { m_spikeAnimation = animation; }

    /**
     * @brief 设置尾巴动画值
     * @param animation 尾巴动画值，从实体获取
     */
    void setTailAnimation(f32 animation) { m_tailAnimation = animation; }

    /**
     * @brief 设置目标实体眼睛位置（用于眼睛追踪）
     * @param targetEyeY 目标眼睛Y坐标
     * @param targetEyeOffset 目标眼睛X偏移
     */
    void setTargetEyePosition(f32 targetEyeY, f32 targetEyeOffset)
    {
        m_targetEyeY = targetEyeY;
        m_targetEyeOffset = targetEyeOffset;
    }

private:
    void setupParts();
    void updateSpines(f64 ageInTicks, f64 spikeAnimation);

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_eye;
    std::array<std::shared_ptr<ModelRenderer>, 12> m_spines;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_tail;

    // 实体状态
    f32 m_spikeAnimation = 0.0f;
    f32 m_tailAnimation = 0.0f;
    f32 m_targetEyeY = 0.0f;
    f32 m_targetEyeOffset = 0.0f;
};

/**
 * @brief 远古守卫者模型
 *
 * 参考 MC 1.16.5 GuardianModel（相同结构，不同纹理）
 */
class ElderGuardianModel : public GuardianModel {
public:
    ElderGuardianModel();
    ~ElderGuardianModel() override = default;
};

/**
 * @brief 潜影贝模型
 *
 * 参考 MC 1.16.5 ShulkerModel
 */
class ShulkerModel : public EntityModel {
public:
    ShulkerModel();
    ~ShulkerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置开盖动画值
     * @param peekAmount 开盖动画值 (0.0 - 1.0)，从实体获取
     */
    void setPeekAmount(f32 peekAmount) { m_peekAmount = peekAmount; }

    [[nodiscard]] std::shared_ptr<ModelRenderer> getBase() const { return m_base; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getLid() const { return m_lid; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }

private:
    void setupParts();

    std::shared_ptr<ModelRenderer> m_base;
    std::shared_ptr<ModelRenderer> m_lid;
    std::shared_ptr<ModelRenderer> m_head;

    // 实体状态
    f32 m_peekAmount = 0.0f;
};

/**
 * @brief 蠹虫模型
 *
 * 参考 MC 1.16.5 SilverfishModel
 */
class SilverfishModel : public EntityModel {
public:
    SilverfishModel();
    ~SilverfishModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 7> m_bodyParts;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_wings;
    std::array<f32, 7> m_zPlacement;
};

/**
 * @brief 末影螨模型
 *
 * 参考 MC 1.16.5 EndermiteModel
 */
class EndermiteModel : public EntityModel {
public:
    EndermiteModel();
    ~EndermiteModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 4> m_bodyParts;
};

} // namespace mc::client::renderer::entity::model::monster
