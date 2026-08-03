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

#include "AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace {

std::array<mc::f64, 16> identityMatrix()
{
    return {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
}

std::array<mc::f64, 16> multiplyMatrices(const std::array<mc::f64, 16>& a, const std::array<mc::f64, 16>& b)
{
    std::array<mc::f64, 16> result{};
    for (mc::i32 row = 0; row < 4; ++row) {
        for (mc::i32 col = 0; col < 4; ++col) {
            for (mc::i32 k = 0; k < 4; ++k) {
                result[static_cast<std::size_t>(row * 4 + col)] +=
                    a[static_cast<std::size_t>(row * 4 + k)] * b[static_cast<std::size_t>(k * 4 + col)];
            }
        }
    }
    return result;
}

std::array<mc::f64, 16> translationMatrix(mc::f64 x, mc::f64 y, mc::f64 z)
{
    return {1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0};
}

std::array<mc::f64, 16> scaleMatrix(mc::f64 scale)
{
    return {scale, 0.0, 0.0, 0.0, 0.0, scale, 0.0, 0.0, 0.0, 0.0, scale, 0.0, 0.0, 0.0, 0.0, 1.0};
}

} // namespace

namespace mc::client::renderer::entity::model {

AgeableModel::AgeableModel()
    : EntityModel()
    , m_isChildHeadScaled(false)
    , m_childHeadOffsetY(5.0f)
    , m_childHeadOffsetZ(2.0f)
    , m_childHeadScale(2.0f)
    , m_childBodyScale(2.0f)
    , m_childBodyOffsetY(24.0f)
{}

AgeableModel::AgeableModel(bool isChildHeadScaled, f32 childHeadOffsetY, f32 childHeadOffsetZ)
    : EntityModel()
    , m_isChildHeadScaled(isChildHeadScaled)
    , m_childHeadOffsetY(childHeadOffsetY)
    , m_childHeadOffsetZ(childHeadOffsetZ)
    , m_childHeadScale(2.0f)
    , m_childBodyScale(2.0f)
    , m_childBodyOffsetY(24.0f)
{}

AgeableModel::AgeableModel(bool isChildHeadScaled,
    f32 childHeadOffsetY,
    f32 childHeadOffsetZ,
    f32 childHeadScale,
    f32 childBodyScale,
    f32 childBodyOffsetY)
    : EntityModel()
    , m_isChildHeadScaled(isChildHeadScaled)
    , m_childHeadOffsetY(childHeadOffsetY)
    , m_childHeadOffsetZ(childHeadOffsetZ)
    , m_childHeadScale(childHeadScale)
    , m_childBodyScale(childBodyScale)
    , m_childBodyOffsetY(childBodyOffsetY)
{}

void AgeableModel::render(f64 scale)
{
    // 幼体渲染需要分别处理头部和身体
    // 头部缩放 1.5F / childHeadScale，身体缩放 1.0F / childBodyScale

    if (m_isChild) {
        // 幼体渲染
        // 渲染头部部件
        auto headParts = getHeadParts();
        if (!headParts.empty()) {
            // 头部缩放：只有当 isChildHeadScaled 为 true 时才缩放
            f32 headScale = m_isChildHeadScaled ? (1.5f / m_childHeadScale) : 1.0f;
            f64 headOffsetY = static_cast<f64>(m_childHeadOffsetY) / 16.0;
            f64 headOffsetZ = static_cast<f64>(m_childHeadOffsetZ) / 16.0;

            // 对每个头部部件应用缩放和偏移
            for (auto& part : headParts) {
                if (part) {
                    // 保存原始状态
                    f64 origRotX = part->rotationPointX();
                    f64 origRotY = part->rotationPointY();
                    f64 origRotZ = part->rotationPointZ();

                    // 应用偏移
                    part->setRotationPoint(origRotX, origRotY + headOffsetY, origRotZ + headOffsetZ);

                    // 渲染
                    part->render(scale * headScale);

                    // 恢复原始状态
                    part->setRotationPoint(origRotX, origRotY, origRotZ);
                }
            }
        }

        // 渲染身体部件
        auto bodyParts = getBodyParts();
        if (!bodyParts.empty()) {
            // 身体缩放
            f32 bodyScale = 1.0f / m_childBodyScale;
            f64 bodyOffsetY = static_cast<f64>(m_childBodyOffsetY) / 16.0;

            // 对每个身体部件应用缩放和偏移
            for (auto& part : bodyParts) {
                if (part) {
                    // 保存原始状态
                    f64 origRotY = part->rotationPointY();

                    // 应用偏移
                    part->setRotationPoint(part->rotationPointX(), origRotY + bodyOffsetY, part->rotationPointZ());

                    // 渲染
                    part->render(scale * bodyScale);

                    // 恢复原始状态
                    part->setRotationPoint(part->rotationPointX(), origRotY, part->rotationPointZ());
                }
            }
        }
    } else {
        // 成年体渲染：分别渲染头部和身体部件
        auto headParts = getHeadParts();
        for (auto& part : headParts) {
            if (part) {
                part->render(scale);
            }
        }

        auto bodyParts = getBodyParts();
        for (auto& part : bodyParts) {
            if (part) {
                part->render(scale);
            }
        }
    }
}

void AgeableModel::generateMesh(std::vector<ModelVertex>& vertices, std::vector<u32>& indices, f64 scale) const
{
    if (!m_isChild) {
        for (const auto& part : getHeadParts()) {
            if (part) {
                part->generateMesh(vertices, indices, scale);
            }
        }
        for (const auto& part : getBodyParts()) {
            if (part) {
                part->generateMesh(vertices, indices, scale);
            }
        }
        return;
    }

    auto headMatrix = identityMatrix();
    if (m_isChildHeadScaled) {
        headMatrix = multiplyMatrices(headMatrix, scaleMatrix(1.5 / static_cast<f64>(m_childHeadScale)));
    }
    headMatrix = multiplyMatrices(headMatrix,
        translationMatrix(
            0.0, static_cast<f64>(m_childHeadOffsetY) * scale, static_cast<f64>(m_childHeadOffsetZ) * scale));
    for (const auto& part : getHeadParts()) {
        if (part) {
            part->generateMesh(vertices, indices, headMatrix, scale);
        }
    }

    auto bodyMatrix = multiplyMatrices(scaleMatrix(1.0 / static_cast<f64>(m_childBodyScale)),
        translationMatrix(0.0, static_cast<f64>(m_childBodyOffsetY) * scale, 0.0));
    for (const auto& part : getBodyParts()) {
        if (part) {
            part->generateMesh(vertices, indices, bodyMatrix, scale);
        }
    }
}

void AgeableModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    EntityModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

void AgeableModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 默认实现为空，子类可以重写
}

} // namespace mc::client::renderer::entity::model
