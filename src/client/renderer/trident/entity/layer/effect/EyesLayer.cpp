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

#include "EyesLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/model/monster/EndermanModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpiderModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::effect {

template <typename TEntity, typename TModel>
void EyesLayer<TEntity, TModel>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取眼睛纹理和颜色
    ResourceLocation texture = getEyesTexture(entity);
    Vector3f color = getEyesColor(entity);

    // 获取头部部件变换
    std::shared_ptr<model::ModelRenderer> headPart = getParentModel()->getModelHead();
    if (!headPart) {
        return;
    }

    // 从头部部件获取变换矩阵
    std::array<f64, 16> headTransform;
    headPart->getTransformMatrix(headTransform);

    // 构建眼睛网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildEyesMesh(headTransform, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // 创建临时网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("EyesLayer: Failed to create eyes mesh");
        return;
    }

    // 获取实体位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 使用发光颜色作为叠加颜色
    Vector4f overlayColor(color.x, color.y, color.z, 1.0f);

    // 切换到叠加混合模式（用于眼睛发光效果）
    pipeline.bind(cmd, pipeline::BlendMode::Additive);

    pipeline.drawMesh(cmd, result.value(), headTransform, entityPos, 1.0, overlayColor, 0.0f, 0.0f);

    // 恢复 Alpha 混合模式
    pipeline.bind(cmd, pipeline::BlendMode::Alpha);

    // TODO: 将眼睛纹理绑定到管线（当前使用叠加混合，纹理尚未应用）
    (void)texture;
}

template <typename TEntity, typename TModel>
bool EyesLayer<TEntity, TModel>::shouldRender(const TEntity& entity) const
{
    // 默认情况下眼睛层总是可见
    // 子类可以根据实体状态重写此方法
    // 例如：末影人在愤怒时眼睛发光
    (void)entity;
    return true;
}

template <typename TEntity, typename TModel>
void EyesLayer<TEntity, TModel>::buildEyesMesh(
    const std::array<f64, 16>& headTransform, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 眼睛是一个简单的四边形，位于头部前方
    // 眼睛位置相对于头部中心

    constexpr f32 EYE_WIDTH = 0.25f;
    constexpr f32 EYE_HEIGHT = 0.15f;
    constexpr f32 EYE_DEPTH = 0.05f;
    constexpr f32 EYE_SPACING = 0.15f; // 左右眼相对于头部中心的水平偏移

    f32 hw = EYE_WIDTH / 2.0f;
    f32 hh = EYE_HEIGHT / 2.0f;

    vertices.clear();
    indices.clear();

    // 提取头部位置（变换矩阵的平移部分）
    f64 headX = headTransform[3];  // X 平移
    f64 headY = headTransform[7];  // Y 平移
    f64 headZ = headTransform[11]; // Z 平移

    // 左眼位置（相对于头部中心）
    f32 leftEyeX = static_cast<f32>(headX) - EYE_SPACING;
    f32 rightEyeX = static_cast<f32>(headX) + EYE_SPACING;
    f32 eyeY = static_cast<f32>(headY);
    f32 eyeZ = static_cast<f32>(headZ) + static_cast<f32>(EYE_DEPTH);

    // 左眼顶点
    // 顶点格式: ModelVertex(x, y, z, u, v, nx, ny, nz)
    vertices.push_back(model::ModelVertex(leftEyeX - hw, eyeY - hh, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX + hw, eyeY - hh, eyeZ, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX + hw, eyeY + hh, eyeZ, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX - hw, eyeY + hh, eyeZ, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));

    // 右眼顶点
    vertices.push_back(model::ModelVertex(rightEyeX - hw, eyeY - hh, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX + hw, eyeY - hh, eyeZ, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX + hw, eyeY + hh, eyeZ, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX - hw, eyeY + hh, eyeZ, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));

    // 左眼索引
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    // 右眼索引
    indices.push_back(4);
    indices.push_back(5);
    indices.push_back(6);
    indices.push_back(4);
    indices.push_back(6);
    indices.push_back(7);
}

// 显式实例化常用类型
template class EyesLayer<::mc::LivingEntity, ::mc::client::renderer::entity::model::BipedModel>;
template class EyesLayer<::mc::LivingEntity, ::mc::client::renderer::entity::model::monster::SpiderModel>;
template class EyesLayer<::mc::LivingEntity, ::mc::client::renderer::entity::model::monster::EndermanModel>;

} // namespace mc::client::renderer::entity::layer::effect
