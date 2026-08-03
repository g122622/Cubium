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

#include "CapeLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <cmath>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::cosmetic {

namespace {
constexpr f32 CAPE_WIDTH = 10.0f / 16.0f;  // 斗篷宽度（世界单位）
constexpr f32 CAPE_HEIGHT = 16.0f / 16.0f; // 斗篷高度
constexpr i32 SWING_ANGLE_BUCKETS = 36;    // 摆动角度分桶数（每10度一个桶）
} // namespace

void CapeLayer::renderPipeline(::mc::Player& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 检查是否有斗篷纹理
    if (!m_customCapeRegion) {
        return;
    }

    // 计算斗篷摆动角度
    f32 swingAngle = _calculateCapeSwing(entity, context, static_cast<f32>(context.partialTicks));

    // 获取或创建斗篷网格
    pipeline::EntityMesh* mesh = _getOrCreateCapeMesh(swingAngle, pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算斗篷变换矩阵
    std::array<f64, 16> capeTransform;
    capeTransform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 斗篷附着在身体背部
    // 位置在身体中心偏后
    capeTransform[3] = 0.0;                     // X
    capeTransform[7] = 1.0 - CAPE_HEIGHT / 2.0; // Y - 斗篷中心在身体上方
    capeTransform[11] = 0.125;                  // Z - 略微向后偏移

    // 获取实体的世界位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 使用实体的受伤时间
    f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
    f32 deathTime = static_cast<f32>(entity.deathTime());

    pipeline.drawMesh(cmd, *mesh, capeTransform, entityPos, 1.0, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);
}

bool CapeLayer::shouldRender(const ::mc::Player& entity) const
{
    // 检查条件：
    // 1. 玩家开启了披风显示（PlayerModelPart::Cape）
    // 2. 玩家有披风纹理
    // 3. 玩家没有穿戴鞘翅（鞘翅会覆盖披风）

    // 检查是否开启了披风显示
    if (!entity.isWearing(::mc::PlayerModelPart::Cape)) {
        return false;
    }

    // 检查是否有披风纹理
    if (m_customCapeRegion == nullptr) {
        return false;
    }

    // 检查是否穿戴了鞘翅（鞘翅会覆盖披风）
    const auto& chest = entity.getEquipment(::mc::EquipmentSlot::Chest);
    if (!chest.isEmpty() && chest.getItem() == ::mc::Items::ELYTRA) {
        return false;
    }

    return true;
}

f32 CapeLayer::_calculateCapeSwing(
    ::mc::Player& entity, const mc::client::renderer::entity::core::AnimationContext& context, f32 partialTicks) const
{
    // 核心逻辑：
    // 1. 使用 chasingPos 系统计算平滑移动向量
    // 2. 使用 renderYawOffset 计算身体朝向
    // 3. 使用 cameraYaw 计算行走摆动
    // 4. 蹲伏时额外增加角度

    // 获取追踪位置
    f64 prevChasingX = static_cast<f64>(entity.prevX());
    f64 prevChasingY = static_cast<f64>(entity.prevY());
    f64 prevChasingZ = static_cast<f64>(entity.prevZ());
    f64 chasingX = static_cast<f64>(entity.x());
    f64 chasingY = static_cast<f64>(entity.y());
    f64 chasingZ = static_cast<f64>(entity.z());

    // 插值追踪位置
    f64 interpChasingX = prevChasingX + (chasingX - prevChasingX) * partialTicks;
    f64 interpChasingY = prevChasingY + (chasingY - prevChasingY) * partialTicks;
    f64 interpChasingZ = prevChasingZ + (chasingZ - prevChasingZ) * partialTicks;

    // 插值实际位置
    f64 interpX = static_cast<f64>(entity.prevX()) +
        (static_cast<f64>(entity.x()) - static_cast<f64>(entity.prevX())) * partialTicks;
    f64 interpY = static_cast<f64>(entity.prevY()) +
        (static_cast<f64>(entity.y()) - static_cast<f64>(entity.prevY())) * partialTicks;
    f64 interpZ = static_cast<f64>(entity.prevZ()) +
        (static_cast<f64>(entity.z()) - static_cast<f64>(entity.prevZ())) * partialTicks;

    // 计算移动向量
    f64 d0 = interpChasingX - interpX;
    f64 d1 = interpChasingY - interpY;
    f64 d2 = interpChasingZ - interpZ;

    // 获取身体朝向 (renderYawOffset)
    f64 renderYawOffset = static_cast<f64>(entity.yaw());
    f64 prevRenderYawOffset = static_cast<f64>(entity.prevYaw());
    f64 interpRenderYawOffset = prevRenderYawOffset + (renderYawOffset - prevRenderYawOffset) * partialTicks;

    // 计算方向向量
    f64 f = interpRenderYawOffset + (interpRenderYawOffset - prevRenderYawOffset); // body rotation
    f64 d3 = std::sin(mc::math::toRadians(static_cast<f32>(f)));
    f64 d4 = -std::cos(mc::math::toRadians(static_cast<f32>(f)));

    // 计算 Y 轴摆动角度
    f32 f1 = static_cast<f32>(d1 * 10.0);
    f1 = mc::math::clamp(f1, -6.0f, 32.0f);

    // 计算 X 轴摆动角度
    f32 f2 = static_cast<f32>((d0 * d3 + d2 * d4) * 100.0);
    f2 = mc::math::clamp(f2, 0.0f, 150.0f);

    // 计算 Z 轴摆动角度
    f32 f3 = static_cast<f32>((d0 * d4 - d2 * d3) * 100.0);
    f3 = mc::math::clamp(f3, -20.0f, 20.0f);

    // 获取相机偏航角
    f32 f4 = static_cast<f32>(context.limbSwingAmount);

    // 行走时增加摆动
    f32 limbSwing = static_cast<f32>(context.limbSwing);
    f1 += std::sin(limbSwing * 6.0f) * 32.0f * f4;

    // 蹲伏时增加角度
    if (entity.isSneaking()) {
        f1 += 25.0f;
    }

    // 返回综合摆动角度
    // 当前架构只支持一个角度，使用 f1 作为主要摆动角度
    return f1;
}

void CapeLayer::_buildCapeMesh(f32 swingAngle, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 斗篷是一个简单的矩形，带有摆动动画
    // 摆动使斗篷底部向后倾斜

    f32 halfWidth = CAPE_WIDTH / 2.0f;
    f32 halfHeight = CAPE_HEIGHT / 2.0f;

    // 将摆动角度转换为弧度
    f32 swingRad = mc::math::toRadians(swingAngle);
    f32 cosSwing = std::cos(swingRad);
    f32 sinSwing = std::sin(swingRad);

    // 斗篷顶点
    // 斗篷顶部固定在身体上，底部随摆动倾斜
    // 顶点格式: position xyz, texcoord uv, normal xyz, color rgba

    // 顶部两点（固定）
    f32 topY = halfHeight;
    f32 bottomY = -halfHeight;

    // 底部两点随摆动偏移
    f32 bottomZ = sinSwing * CAPE_HEIGHT;
    f32 bottomYOffset = (1.0f - cosSwing) * CAPE_HEIGHT * 0.5f;

    // UV 坐标（斗篷纹理在皮肤纹理中）
    f32 u0 = 0.0f;
    f32 u1 = 1.0f;
    f32 v0 = 0.0f;
    f32 v1 = 1.0f;

    // 构建顶点（双面渲染）
    // 正面
    vertices.push_back(model::ModelVertex(-halfWidth, topY, 0.02f, u0, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(halfWidth, topY, 0.02f, u1, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(
        model::ModelVertex(halfWidth, bottomY + bottomYOffset, bottomZ + 0.02f, u1, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(
        model::ModelVertex(-halfWidth, bottomY + bottomYOffset, bottomZ + 0.02f, u0, v1, 0.0f, 0.0f, 1.0f));

    // 反面
    vertices.push_back(model::ModelVertex(-halfWidth, topY, -0.02f, u0, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(halfWidth, topY, -0.02f, u1, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(
        model::ModelVertex(halfWidth, bottomY + bottomYOffset, bottomZ - 0.02f, u1, v1, 0.0f, 0.0f, -1.0f));
    vertices.push_back(
        model::ModelVertex(-halfWidth, bottomY + bottomYOffset, bottomZ - 0.02f, u0, v1, 0.0f, 0.0f, -1.0f));

    // 索引（两个三角形组成一个四边形，正反两面）
    // 正面
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    // 反面
    indices.push_back(4);
    indices.push_back(6);
    indices.push_back(5);
    indices.push_back(4);
    indices.push_back(7);
    indices.push_back(6);
}

pipeline::EntityMesh* CapeLayer::_getOrCreateCapeMesh(f32 swingAngle, pipeline::EntityPipeline& pipeline)
{
    // 将摆动角度离散化到桶中
    i32 bucket = static_cast<i32>(swingAngle / 10.0f * SWING_ANGLE_BUCKETS);
    bucket = mc::math::clamp(bucket, 0, SWING_ANGLE_BUCKETS - 1);

    auto it = m_capeMeshCache.find(bucket);
    if (it != m_capeMeshCache.end()) {
        return &it->second;
    }

    // 构建新的斗篷网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    _buildCapeMesh(swingAngle, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("CapeLayer: Failed to create cape mesh for angle {:.1f}", swingAngle);
        return nullptr;
    }

    m_capeMeshCache[bucket] = std::move(result.value());
    return &m_capeMeshCache[bucket];
}

} // namespace mc::client::renderer::entity::layer::cosmetic
