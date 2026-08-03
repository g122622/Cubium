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

#include "ArrowLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::entity {

// 静态成员定义
template <typename TEntity>
std::unique_ptr<pipeline::EntityMesh> ArrowLayer<TEntity>::s_arrowMesh = nullptr;

template <typename TEntity>
void ArrowLayer<TEntity>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 获取箭矢数量
    i32 arrowCount = 0;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        arrowCount = entity.getArrowCount();
    }

    if (arrowCount <= 0) {
        return;
    }

    // 使用 ArrowEntity 实际渲染，而不是手动构建网格
    // 这里简化为获取或创建箭矢网格

    pipeline::EntityMesh* mesh = _getOrCreateArrowMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 渲染多支箭矢，随机分布在实体身上
    for (i32 i = 0; i < arrowCount && i < 10; ++i) {
        // 随机位置和角度（基于实体 ID 和箭矢索引）
        u32 seed = static_cast<u32>(entity.id() * 31 + i);
        f32 randX = static_cast<f32>((seed * 16807) % 1000) / 1000.0f - 0.5f;
        f32 randY = static_cast<f32>(((seed * 16807) + 12345) % 1000) / 1000.0f;
        f32 randZ = static_cast<f32>(((seed * 16807) + 67890) % 1000) / 1000.0f - 0.5f;
        f32 randYaw = static_cast<f32>(((seed * 16807) + 11111) % 1000) / 1000.0f * 360.0f;
        f32 randPitch = static_cast<f32>(((seed * 16807) + 22222) % 1000) / 1000.0f * 180.0f - 90.0f;

        _renderArrowPipeline(entity, randX, randY, randZ, randYaw, randPitch, cmd, context, pipeline);
    }
}

template <typename TEntity>
bool ArrowLayer<TEntity>::shouldRender(const TEntity& entity) const
{
    // 检查实体是否被箭射中
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        return entity.getArrowCount() > 0;
    }
    return false;
}

template <typename TEntity>
void ArrowLayer<TEntity>::_buildArrowMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 简化的箭矢网格

    constexpr f32 ARROW_LENGTH = 0.5f;
    constexpr f32 ARROW_RADIUS = 0.025f;
    constexpr f32 HEAD_LENGTH = 0.1f;
    constexpr f32 HEAD_RADIUS = 0.05f;

    vertices.clear();
    indices.clear();

    f32 halfLength = ARROW_LENGTH / 2.0f;

    // 箭杆正面 - ModelVertex(x, y, z, u, v, nx, ny, nz)
    vertices.push_back(model::ModelVertex(-ARROW_RADIUS, -halfLength, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(ARROW_RADIUS, -halfLength, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(ARROW_RADIUS, halfLength - HEAD_LENGTH, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(-ARROW_RADIUS, halfLength - HEAD_LENGTH, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f));

    // 箭杆背面
    vertices.push_back(model::ModelVertex(-ARROW_RADIUS, -halfLength, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(ARROW_RADIUS, -halfLength, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(ARROW_RADIUS, halfLength - HEAD_LENGTH, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, -1.0f));
    vertices.push_back(
        model::ModelVertex(-ARROW_RADIUS, halfLength - HEAD_LENGTH, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, -1.0f));

    // 箭头
    vertices.push_back(model::ModelVertex(-HEAD_RADIUS, halfLength - HEAD_LENGTH, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(HEAD_RADIUS, halfLength - HEAD_LENGTH, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(0.0f, halfLength, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f));

    // 索引
    indices.insert(indices.end(), {0, 1, 2, 0, 2, 3}); // 箭杆正面
    indices.insert(indices.end(), {4, 6, 5, 4, 7, 6}); // 箭杆背面
    indices.insert(indices.end(), {8, 9, 10});         // 箭头
}

template <typename TEntity>
void ArrowLayer<TEntity>::_renderArrowPipeline(TEntity& entity,
    f32 x,
    f32 y,
    f32 z,
    f32 yaw,
    f32 pitch,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    pipeline::EntityMesh* mesh = _getOrCreateArrowMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算箭矢变换矩阵
    f32 yawRad = mc::math::toRadians(yaw);
    f32 pitchRad = mc::math::toRadians(pitch);

    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);
    f32 cosPitch = std::cos(pitchRad);
    f32 sinPitch = std::sin(pitchRad);

    std::array<f64, 16> arrowTransform;
    arrowTransform[0] = cosYaw;
    arrowTransform[1] = sinYaw * sinPitch;
    arrowTransform[2] = -sinYaw * cosPitch;
    arrowTransform[3] = static_cast<f64>(x);

    arrowTransform[4] = 0.0;
    arrowTransform[5] = cosPitch;
    arrowTransform[6] = sinPitch;
    arrowTransform[7] = static_cast<f64>(y + 0.8);

    arrowTransform[8] = sinYaw;
    arrowTransform[9] = -cosYaw * sinPitch;
    arrowTransform[10] = cosYaw * cosPitch;
    arrowTransform[11] = static_cast<f64>(z);

    arrowTransform[12] = 0.0;
    arrowTransform[13] = 0.0;
    arrowTransform[14] = 0.0;
    arrowTransform[15] = 1.0;

    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(
        cmd, *mesh, arrowTransform, entityPos, 1.0, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

    (void)context;
}

template <typename TEntity>
pipeline::EntityMesh* ArrowLayer<TEntity>::_getOrCreateArrowMesh(pipeline::EntityPipeline& pipeline)
{
    if (s_arrowMesh && s_arrowMesh->indexCount > 0) {
        return s_arrowMesh.get();
    }

    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    _buildArrowMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("ArrowLayer: Failed to create arrow mesh");
        return nullptr;
    }

    s_arrowMesh = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    return s_arrowMesh.get();
}

// 显式实例化
template class ArrowLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::entity
