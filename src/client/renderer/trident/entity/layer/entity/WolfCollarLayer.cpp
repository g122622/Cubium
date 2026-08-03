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

#include "WolfCollarLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/layer/entity/WolfCollarColors.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::entity {

using wolf_collar_colors::getCollarColorByIndex;

// 静态成员定义
std::unique_ptr<pipeline::EntityMesh> WolfCollarLayer::s_collarMesh = nullptr;

void WolfCollarLayer::renderPipeline(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取项圈颜色
    Vector3f color = _getCollarColor(entity);

    // 获取或创建项圈网格
    pipeline::EntityMesh* mesh = _getOrCreateCollarMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算项圈变换矩阵
    // 项圈位于狼的颈部
    std::array<f64, 16> collarTransform;
    collarTransform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 项圈位置（颈部）
    collarTransform[3] = 0.0;  // X
    collarTransform[7] = 0.5;  // Y - 颈部高度
    collarTransform[11] = 0.1; // Z - 略微向前

    // 应用头部旋转（项圈跟随头部）
    f32 headYaw = static_cast<f32>(context.netHeadYaw);
    f32 yawRad = mc::math::toRadians(headYaw);
    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);

    // 轻微的头部旋转影响
    collarTransform[0] = cosYaw;
    collarTransform[2] = -sinYaw * 0.3; // 轻微影响
    collarTransform[8] = sinYaw * 0.3;
    collarTransform[10] = cosYaw;

    // 获取实体位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 使用项圈颜色作为叠加颜色
    Vector4f overlayColor(color.x, color.y, color.z, 1.0f);

    // 使用实体的受伤时间
    f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
    f32 deathTime = static_cast<f32>(entity.deathTime());

    pipeline.drawMesh(cmd, *mesh, collarTransform, entityPos, 1.0, overlayColor, hurtTime, deathTime);

    (void)context;
}

bool WolfCollarLayer::shouldRender(const ::mc::client::ClientEntity& entity) const
{
    // 只有驯服的狼才显示项圈
    // 对应 MC 1.21.11 WolfCollarLayer.submit() 中的 collarColor != null 检查
    // （MC 中 extractRenderState 在未驯服时将 collarColor 设为 null）
    return entity.wolfTamed();
}

Vector3f WolfCollarLayer::_getCollarColor(const ::mc::client::ClientEntity& entity)
{
    u8 colorIndex = static_cast<u8>(entity.wolfCollarColor());
    return getCollarColorByIndex(colorIndex);
}
void WolfCollarLayer::_buildCollarMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 项圈是一个简单的环形，围绕颈部

    vertices.clear();
    indices.clear();

    // 项圈尺寸
    constexpr f32 COLLAR_RADIUS = 0.15f;
    constexpr f32 COLLAR_HEIGHT = 0.1f;
    constexpr i32 SEGMENTS = 16;

    // 生成环形顶点
    for (i32 seg = 0; seg <= SEGMENTS; ++seg) {
        f32 angle = static_cast<f32>(seg) / static_cast<f32>(SEGMENTS) * mc::math::TWO_PI;
        f32 cosAngle = std::cos(angle);
        f32 sinAngle = std::sin(angle);

        f32 x = COLLAR_RADIUS * cosAngle;
        f32 z = COLLAR_RADIUS * sinAngle;

        // 外环 - ModelVertex(x, y, z, u, v, nx, ny, nz)
        vertices.push_back(model::ModelVertex(
            x, 0.0f, z, static_cast<f32>(seg) / static_cast<f32>(SEGMENTS), 0.0f, cosAngle, 0.0f, sinAngle));
        vertices.push_back(model::ModelVertex(
            x, COLLAR_HEIGHT, z, static_cast<f32>(seg) / static_cast<f32>(SEGMENTS), 1.0f, cosAngle, 0.0f, sinAngle));
    }

    // 生成索引
    for (i32 seg = 0; seg < SEGMENTS; ++seg) {
        u32 base = static_cast<u32>(seg * 2);

        // 外环
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 1);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }
}

pipeline::EntityMesh* WolfCollarLayer::_getOrCreateCollarMesh(pipeline::EntityPipeline& pipeline)
{
    if (s_collarMesh && s_collarMesh->indexCount > 0) {
        return s_collarMesh.get();
    }

    // 构建项圈网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    _buildCollarMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("WolfCollarLayer: Failed to create collar mesh");
        return nullptr;
    }

    s_collarMesh = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    return s_collarMesh.get();
}

} // namespace mc::client::renderer::entity::layer::entity
