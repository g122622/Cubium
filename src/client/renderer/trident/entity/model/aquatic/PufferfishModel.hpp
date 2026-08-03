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
#include <memory>

namespace mc::client::renderer::entity::model::aquatic {

/**
 * @brief 河豚小型模型（未膨胀状态）
 *
 * 纹理尺寸: 32x32
 * 结构: 身体 + 右眼 + 左眼 + 尾巴 + 右鳍 + 左鳍
 */
class PufferfishSmallModel : public EntityModel {
public:
    PufferfishSmallModel();
    ~PufferfishSmallModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    void setInWater(bool inWater) noexcept { m_isInWater = inWater; }

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightEye;
    std::shared_ptr<ModelRenderer> m_leftEye;
    std::shared_ptr<ModelRenderer> m_tail;
    std::shared_ptr<ModelRenderer> m_rightFin;
    std::shared_ptr<ModelRenderer> m_leftFin;

    bool m_isInWater = true;
};

/**
 * @brief 河豚中型模型（半膨胀状态）
 *
 * 纹理尺寸: 32x32
 * 结构: 身体 + 右鳍 + 左鳍 + 前上刺 + 后上刺 + 前右刺 + 后右刺 + 后左刺 + 前左刺 + 后下刺 + 前下刺
 */
class PufferfishMediumModel : public EntityModel {
public:
    PufferfishMediumModel();
    ~PufferfishMediumModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    void setInWater(bool inWater) noexcept { m_isInWater = inWater; }

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightFin;
    std::shared_ptr<ModelRenderer> m_leftFin;
    std::shared_ptr<ModelRenderer> m_frontTopSpines;
    std::shared_ptr<ModelRenderer> m_backTopSpines;
    std::shared_ptr<ModelRenderer> m_frontRightSpines;
    std::shared_ptr<ModelRenderer> m_backRightSpines;
    std::shared_ptr<ModelRenderer> m_backLeftSpines;
    std::shared_ptr<ModelRenderer> m_frontLeftSpines;
    std::shared_ptr<ModelRenderer> m_backBottomSpine;
    std::shared_ptr<ModelRenderer> m_frontBottomSpines;

    bool m_isInWater = true;
};

/**
 * @brief 河豚大型模型（完全膨胀状态）
 *
 * 纹理尺寸: 32x32
 * 结构: 身体 + 右鳍 + 左鳍 + 前上刺 + 中上刺 + 后上刺
 *       + 前右刺 + 前左刺 + 前下刺 + 中下刺 + 后下刺 + 后右刺 + 后左刺
 */
class PufferfishBigModel : public EntityModel {
public:
    PufferfishBigModel();
    ~PufferfishBigModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    void setInWater(bool inWater) noexcept { m_isInWater = inWater; }

private:
    void _setupParts();
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightFin;
    std::shared_ptr<ModelRenderer> m_leftFin;
    std::shared_ptr<ModelRenderer> m_frontTopSpines;
    std::shared_ptr<ModelRenderer> m_topMidSpines;
    std::shared_ptr<ModelRenderer> m_backTopSpines;
    std::shared_ptr<ModelRenderer> m_frontRightSpines;
    std::shared_ptr<ModelRenderer> m_frontLeftSpines;
    std::shared_ptr<ModelRenderer> m_frontBottomSpines;
    std::shared_ptr<ModelRenderer> m_bottomMidSpines;
    std::shared_ptr<ModelRenderer> m_bottomBackSpines;
    std::shared_ptr<ModelRenderer> m_backRightSpines;
    std::shared_ptr<ModelRenderer> m_backLeftSpines;

    bool m_isInWater = true;
};

} // namespace mc::client::renderer::entity::model::aquatic
