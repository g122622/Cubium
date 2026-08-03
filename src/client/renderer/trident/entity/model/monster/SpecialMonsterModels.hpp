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
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <memory>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 凋灵模型
 */
class WitherModel : public EntityModel {
public:
    WitherModel();
    explicit WitherModel(f32 scale);
    ~WitherModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置侧头朝向（偏航、俯仰）
     *
     * 对应 MC 1.21.11 WitherBossModel.setupHeadRotation(renderState, head, index)：
     *   head.yRot = (yHeadRots[index] - bodyRot) * PI / 180
     *   head.xRot = xHeadRots[index] * PI / 180
     *
     * 调用时机：在 setAngles 之后由 EntityRendererManager::_createModelForEntity
     * 或 WitherRenderer::render 调用，使用已插值的角度值。
     *
     * @param yaw0   左头偏航角（度，已减去身体偏航角，对应 MC yHeadRots[0]-bodyRot）
     * @param pitch0 左头俯仰角（度）
     * @param yaw1   右头偏航角（度，已减去身体偏航角）
     * @param pitch1 右头俯仰角（度）
     */
    void setSideHeadRotations(f32 yaw0, f32 pitch0, f32 yaw1, f32 pitch1)
    {
        m_sideHeadYaw[0] = yaw0;
        m_sideHeadPitch[0] = pitch0;
        m_sideHeadYaw[1] = yaw1;
        m_sideHeadPitch[1] = pitch1;
        m_hasSideHeadRotations = true;
    }

private:
    void _setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 3> m_upperBodyParts;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_heads;

    // 侧头朝向（度），由 setSideHeadRotations 设置。
    // m_heads[1] 使用 index 0（左头），m_heads[2] 使用 index 1（右头）。
    // 对应 MC 1.21.11 WitherRenderState.yHeadRots[2] / xHeadRots[2]。
    std::array<f32, 2> m_sideHeadYaw = {0.0f, 0.0f};
    std::array<f32, 2> m_sideHeadPitch = {0.0f, 0.0f};
    bool m_hasSideHeadRotations = false;
};

/**
 * @brief 史莱姆模型
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
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightEye;
    std::shared_ptr<ModelRenderer> m_leftEye;
    std::shared_ptr<ModelRenderer> m_mouth;
    i32 m_size = 0;
};

/**
 * @brief 守卫者模型
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
    void _setupParts();
    void _updateSpines(f64 ageInTicks, f64 spikeAnimation);

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
 * 与守卫者结构相同，使用不同纹理
 */
class ElderGuardianModel : public GuardianModel {
public:
    ElderGuardianModel();
    ~ElderGuardianModel() override = default;
};

/**
 * @brief 潜影贝模型
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
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_base;
    std::shared_ptr<ModelRenderer> m_lid;
    std::shared_ptr<ModelRenderer> m_head;

    // 实体状态
    f32 m_peekAmount = 0.0f;
};

/**
 * @brief 蠹虫模型
 */
class SilverfishModel : public EntityModel {
public:
    SilverfishModel();
    ~SilverfishModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 7> m_bodyParts;
    std::array<std::shared_ptr<ModelRenderer>, 3> m_wings;
    std::array<f32, 7> m_zPlacement;
};

/**
 * @brief 末影螨模型
 */
class EndermiteModel : public EntityModel {
public:
    EndermiteModel();
    ~EndermiteModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::array<std::shared_ptr<ModelRenderer>, 4> m_bodyParts;
};

} // namespace mc::client::renderer::entity::model::monster
