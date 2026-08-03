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

#include "SheepWoolLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::entity {

namespace {
// 羊毛颜色 RGB 值
const Vector3f WOOL_COLORS[16] = {
    Vector3f(1.0f, 1.0f, 1.0f),    // 白色 (0)
    Vector3f(0.85f, 0.85f, 0.85f), // 橙色 (1)
    Vector3f(0.8f, 0.6f, 1.0f),    // 品红色 (2)
    Vector3f(0.6f, 0.8f, 1.0f),    // 淡蓝色 (3)
    Vector3f(1.0f, 1.0f, 0.5f),    // 黄色 (4)
    Vector3f(0.5f, 1.0f, 0.5f),    // 黄绿色 (5)
    Vector3f(1.0f, 0.6f, 0.6f),    // 粉红色 (6)
    Vector3f(0.5f, 0.5f, 0.5f),    // 灰色 (7)
    Vector3f(0.3f, 0.3f, 0.3f),    // 淡灰色 (8)
    Vector3f(0.4f, 0.3f, 0.2f),    // 青色 (9)
    Vector3f(0.3f, 0.3f, 0.6f),    // 紫色 (10)
    Vector3f(0.2f, 0.3f, 0.5f),    // 蓝色 (11)
    Vector3f(0.4f, 0.3f, 0.2f),    // 棕色 (12)
    Vector3f(0.2f, 0.4f, 0.2f),    // 绿色 (13)
    Vector3f(0.6f, 0.2f, 0.2f),    // 红色 (14)
    Vector3f(0.1f, 0.1f, 0.1f),    // 黑色 (15)
};
} // namespace

// 静态成员定义
template <typename TEntity, typename TModel>
std::unique_ptr<pipeline::EntityMesh> SheepWoolLayer<TEntity, TModel>::s_woolMesh = nullptr;

template <typename TEntity, typename TModel>
void SheepWoolLayer<TEntity, TModel>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取父模型并复制动画状态到羊毛模型
    TModel* parentModel = getParentModel();
    TModel* woolModel = getWoolModel();

    // 如果有羊毛模型和父模型，复制动画状态
    if (woolModel && parentModel) {
        woolModel->copyAnglesFrom(*parentModel);
    }

    // 获取羊毛颜色
    u32 ticksExisted = 0;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        ticksExisted = entity.ticksExisted();
    }
    Vector3f color = getWoolColor(entity, ticksExisted);

    // 获取或创建羊毛网格
    pipeline::EntityMesh* mesh = _getOrCreateWoolMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算羊毛变换矩阵
    // 羊毛层覆盖羊的身体，略微放大以避免 z-fighting
    std::array<f64, 16> woolTransform;
    woolTransform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 羊毛略微膨胀
    const f32 woolScale = 1.05f;
    woolTransform[0] = woolScale;
    woolTransform[5] = woolScale;
    woolTransform[10] = woolScale;

    // 羊毛位置在身体中心
    woolTransform[3] = 0.0;  // X
    woolTransform[7] = 0.7;  // Y - 身体高度
    woolTransform[11] = 0.0; // Z

    // 应用步态动画（羊毛跟随身体摆动）
    f64 limbSwing = context.limbSwing;
    f64 limbSwingAmount = context.limbSwingAmount;
    if (limbSwingAmount > 0.01) {
        f64 swingAngle = std::sin(limbSwing * 0.5) * limbSwingAmount * 0.1;
        woolTransform[2] = swingAngle * 0.15;
    }

    // 获取实体位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 使用羊毛颜色作为叠加颜色
    Vector4f overlayColor(color.x, color.y, color.z, 1.0f);

    // 使用实体的受伤时间
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(cmd, *mesh, woolTransform, entityPos, 1.0, overlayColor, hurtTime, deathTime);

    (void)context;
}

template <typename TEntity, typename TModel>
bool SheepWoolLayer<TEntity, TModel>::shouldRender(const TEntity& entity) const
{
    // 检查是否被剪切
    if constexpr (std::is_base_of_v<::mc::SheepEntity, TEntity>) {
        if (entity.isSheared()) {
            return false;
        }
    }

    // 检查是否隐身
    if constexpr (std::is_base_of_v<::mc::Entity, TEntity>) {
        if (entity.hasFlag(::mc::EntityFlags::Invisible)) {
            return false;
        }
    }

    return true;
}

template <typename TEntity, typename TModel>
Vector3f SheepWoolLayer<TEntity, TModel>::getWoolColor(const TEntity& entity, u32 ticksExisted)
{
    // 检查是否为 jeb_ 彩虹羊
    if (isRainbowSheep(entity)) {
        return computeRainbowColor(ticksExisted);
    }

    // 尝试将 entity 转换为 SheepEntity 以获取颜色
    // 如果不是 SheepEntity，返回默认白色
    if constexpr (std::is_base_of_v<::mc::SheepEntity, TEntity>) {
        u8 colorIndex = static_cast<u8>(entity.getFleeceColor());
        if (colorIndex < 16) {
            return WOOL_COLORS[colorIndex];
        }
    }
    return Vector3f(1.0f, 1.0f, 1.0f); // 默认白色
}

template <typename TEntity, typename TModel>
Vector3f SheepWoolLayer<TEntity, TModel>::computeRainbowColor(u32 ticksExisted)
{
    // 颜色每 2 tick 变化一次，循环 16 种颜色
    u32 colorIndex = (ticksExisted / 2) % 16;
    return WOOL_COLORS[colorIndex];
}

template <typename TEntity, typename TModel>
bool SheepWoolLayer<TEntity, TModel>::isRainbowSheep(const TEntity& entity)
{
    // 检查实体是否有自定义名称 "jeb_"
    if constexpr (std::is_base_of_v<::mc::Entity, TEntity>) {
        if (entity.hasCustomName()) {
            std::string name = entity.customNameText();
            return name == "jeb_" || name == "jeb";
        }
    }
    return false;
}

template <typename TEntity, typename TModel>
void SheepWoolLayer<TEntity, TModel>::_buildWoolMesh(
    std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 羊毛覆盖羊的身体，是一个类似椭球的形状

    vertices.clear();
    indices.clear();

    // TODO: 羊毛网格当前使用简化椭球体，需替换为精确的羊身体模型形状
    constexpr i32 SEGMENTS = 16;
    constexpr i32 RINGS = 12;

    // 羊毛尺寸（相对于羊身体）
    constexpr f32 WOOL_RADIUS_X = 0.5f;
    constexpr f32 WOOL_RADIUS_Y = 0.6f;
    constexpr f32 WOOL_RADIUS_Z = 0.7f;

    // 生成椭球顶点
    for (i32 ring = 0; ring <= RINGS; ++ring) {
        f32 theta = static_cast<f32>(ring) / static_cast<f32>(RINGS) * mc::math::PI;
        f32 sinTheta = std::sin(theta);
        f32 cosTheta = std::cos(theta);

        for (i32 seg = 0; seg <= SEGMENTS; ++seg) {
            f32 phi = static_cast<f32>(seg) / static_cast<f32>(SEGMENTS) * mc::math::TWO_PI;
            f32 sinPhi = std::sin(phi);
            f32 cosPhi = std::cos(phi);

            f32 x = WOOL_RADIUS_X * sinTheta * cosPhi;
            f32 y = WOOL_RADIUS_Y * cosTheta;
            f32 z = WOOL_RADIUS_Z * sinTheta * sinPhi;

            f32 u = static_cast<f32>(seg) / static_cast<f32>(SEGMENTS);
            f32 v = static_cast<f32>(ring) / static_cast<f32>(RINGS);

            // 法线（椭球法线）
            f32 nx = x / (WOOL_RADIUS_X * WOOL_RADIUS_X);
            f32 ny = y / (WOOL_RADIUS_Y * WOOL_RADIUS_Y);
            f32 nz = z / (WOOL_RADIUS_Z * WOOL_RADIUS_Z);
            f32 len = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= len;
            ny /= len;
            nz /= len;

            vertices.push_back(model::ModelVertex(x, y, z, u, v, nx, ny, nz));
        }
    }

    // 生成索引
    for (i32 ring = 0; ring < RINGS; ++ring) {
        for (i32 seg = 0; seg < SEGMENTS; ++seg) {
            u32 current = static_cast<u32>(ring * (SEGMENTS + 1) + seg);
            u32 next = static_cast<u32>((ring + 1) * (SEGMENTS + 1) + seg);

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

template <typename TEntity, typename TModel>
pipeline::EntityMesh* SheepWoolLayer<TEntity, TModel>::_getOrCreateWoolMesh(pipeline::EntityPipeline& pipeline)
{
    if (s_woolMesh && s_woolMesh->indexCount > 0) {
        return s_woolMesh.get();
    }

    // 构建羊毛网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    _buildWoolMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("SheepWoolLayer: Failed to create wool mesh");
        return nullptr;
    }

    s_woolMesh = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    return s_woolMesh.get();
}

// 显式实例化
template class SheepWoolLayer<::mc::LivingEntity, ::mc::client::renderer::entity::model::BipedModel>;
template class SheepWoolLayer<::mc::SheepEntity, ::mc::client::renderer::entity::model::BipedModel>;

} // namespace mc::client::renderer::entity::layer::entity
