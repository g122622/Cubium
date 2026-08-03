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

#include "NameTagRenderer.hpp"
#include "../pipeline/EntityPipeline.hpp"
#include "WorldTextRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <array>
#include <cmath>
#include <string>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool NameTagRenderer::s_initialized = false;
f64 NameTagRenderer::s_maxDistance = DEFAULT_MAX_DISTANCE;
f64 NameTagRenderer::s_scale = DEFAULT_SCALE;
bool NameTagRenderer::s_showBackground = true;
u8 NameTagRenderer::s_bgColorR = 0;
u8 NameTagRenderer::s_bgColorG = 0;
u8 NameTagRenderer::s_bgColorB = 0;
u8 NameTagRenderer::s_bgColorA = 128;
Vector3d NameTagRenderer::s_cameraPosition(0.0, 0.0, 0.0);
std::array<f64, 16> NameTagRenderer::s_viewMatrix = {
    1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
mc::math::frustum::Frustum NameTagRenderer::s_frustum;

bool NameTagRenderer::initialize(pipeline::EntityPipeline& pipeline)
{
    if (s_initialized) {
        return true;
    }

    // NameTagRenderer 是 WorldTextRenderer 的轻量级包装器
    // 实际的字体资源和网格由 WorldTextRenderer 管理
    s_initialized = true;
    spdlog::info("NameTagRenderer: Initialized successfully");

    (void)pipeline;
    return true;
}

void NameTagRenderer::cleanup()
{
    s_initialized = false;
    spdlog::info("NameTagRenderer: Cleaned up");
}

bool NameTagRenderer::isInitialized()
{
    return s_initialized;
}

void NameTagRenderer::setCameraPosition(const Vector3d& position)
{
    s_cameraPosition = position;
}

void NameTagRenderer::setViewMatrix(const std::array<f64, 16>& viewMatrix)
{
    s_viewMatrix = viewMatrix;
}

void NameTagRenderer::setFrustum(const mc::math::frustum::Frustum& frustum)
{
    s_frustum = frustum;
}

void NameTagRenderer::renderNameTag(Entity& entity, const std::string& displayName, f64 partialTicks)
{
    // CPU 路径 - 已废弃
    if (displayName.empty()) {
        return;
    }

    // 计算名称标签位置
    Vector3d position = calculateNameTagPosition(entity, partialTicks);

    // 计算到相机的距离
    Vector3d toCamera = s_cameraPosition - position;
    f64 distanceToCamera = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

    // 检查是否应该渲染
    if (!shouldRenderNameTag(entity, distanceToCamera)) {
        return;
    }

    // CPU 路径无法执行实际渲染
    (void)position;
}

void NameTagRenderer::renderNameTag(VkCommandBuffer cmd,
    Entity& entity,
    const std::string& displayName,
    f64 partialTicks,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized) {
        if (!initialize(pipeline)) {
            return;
        }
    }

    if (displayName.empty()) {
        return;
    }

    // 检查 WorldTextRenderer 是否已初始化
    if (!WorldTextRenderer::isInitialized()) {
        return;
    }

    // 计算名称标签位置
    Vector3d position = calculateNameTagPosition(entity, partialTicks);

    // 计算到相机的距离
    Vector3d toCamera = s_cameraPosition - position;
    f64 distanceToCamera = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

    // 检查是否应该渲染
    if (!shouldRenderNameTag(entity, distanceToCamera)) {
        return;
    }

    // 转换为 float 类型位置
    Vector3f entityPos(static_cast<f32>(entity.prevX() + (entity.x() - entity.prevX()) * partialTicks),
        static_cast<f32>(entity.prevY() + (entity.y() - entity.prevY()) * partialTicks),
        static_cast<f32>(entity.prevZ() + (entity.z() - entity.prevZ()) * partialTicks));
    f32 entityHeight = static_cast<f32>(entity.height());

    // 设置相机信息到 WorldTextRenderer
    WorldTextRenderer::setCameraPosition(s_cameraPosition);
    WorldTextRenderer::setViewMatrix(s_viewMatrix);
    WorldTextRenderer::setFrustum(s_frustum);
    WorldTextRenderer::setShowBackground(s_showBackground);
    WorldTextRenderer::setBackgroundColor(s_bgColorR, s_bgColorG, s_bgColorB, s_bgColorA);
    WorldTextRenderer::setMaxDistance(static_cast<f32>(s_maxDistance));

    // 调用 WorldTextRenderer 进行实际渲染
    WorldTextRenderer::renderNameTag(cmd, displayName, entityPos, entityHeight, pipeline);
}

bool NameTagRenderer::shouldRenderNameTag(Entity& entity, f64 distanceToCamera)
{
    // 检查距离
    if (distanceToCamera > s_maxDistance) {
        return false;
    }

    // 检查实体是否有自定义名称或是否被命名
    const std::string customName = entity.customNameText();
    bool hasCustomName = !customName.empty();
    bool isCustomNameVisible = entity.isCustomNameVisible();

    // 如果有自定义名称且设置为可见，总是渲染
    if (hasCustomName && isCustomNameVisible) {
        return true;
    }

    return hasCustomName;
}

void NameTagRenderer::setMaxDistance(f64 distance)
{
    s_maxDistance = distance;
}

f64 NameTagRenderer::maxDistance()
{
    return s_maxDistance;
}

void NameTagRenderer::setScale(f64 scale)
{
    s_scale = scale;
}

void NameTagRenderer::setBackgroundColor(u8 r, u8 g, u8 b, u8 a)
{
    s_bgColorR = r;
    s_bgColorG = g;
    s_bgColorB = b;
    s_bgColorA = a;
}

void NameTagRenderer::setShowBackground(bool show)
{
    s_showBackground = show;
}

Vector3d NameTagRenderer::calculateNameTagPosition(Entity& entity, f64 partialTicks)
{
    // 获取插值位置
    f64 x = entity.prevX() + (entity.x() - entity.prevX()) * partialTicks;
    f64 y = entity.prevY() + (entity.y() - entity.prevY()) * partialTicks;
    f64 z = entity.prevZ() + (entity.z() - entity.prevZ()) * partialTicks;

    // 在实体高度之上
    f64 height = static_cast<f64>(entity.height());
    f64 nameTagY = y + height + HEIGHT_OFFSET;

    // 如果实体正在蹲伏，调整高度（蹲伏时玩家变矮）
    if (entity.pose() == mc::EntityPose::Crouching) {
        // 蹲伏时高度约为正常高度的 5/8
        nameTagY = y + height * 0.625 + HEIGHT_OFFSET;
    }

    return Vector3d(x, nameTagY, z);
}

f64 NameTagRenderer::calculateScale(f64 distanceToCamera)
{
    // 参考 MC 1.16.5: 名称标签使用固定缩放
    // MC 1.16.5 不随距离缩放名称标签

    // 如果距离太近，稍微放大
    if (distanceToCamera < 1.0) {
        return s_scale * 1.5;
    }

    return s_scale;
}

void NameTagRenderer::computeBillboardMatrix(const Vector3d& position, std::array<f64, 16>& outMatrix)
{
    // 参考 MC 1.16.5: 名称标签始终面向相机（billboard）
    // 从视图矩阵中提取旋转部分，然后反转

    // 提取视图矩阵的上方向和右方向
    // 视图矩阵的前三行是相机的旋转矩阵
    f64 view00 = s_viewMatrix[0];
    f64 view01 = s_viewMatrix[1];
    f64 view02 = s_viewMatrix[2];
    f64 view10 = s_viewMatrix[4];
    f64 view11 = s_viewMatrix[5];
    f64 view12 = s_viewMatrix[6];
    f64 view20 = s_viewMatrix[8];
    f64 view21 = s_viewMatrix[9];
    f64 view22 = s_viewMatrix[10];

    // billboard 矩阵是视图矩阵旋转部分的转置（即逆）
    // 加上位置
    outMatrix[0] = view00;
    outMatrix[1] = view10;
    outMatrix[2] = view20;
    outMatrix[3] = position.x;
    outMatrix[4] = view01;
    outMatrix[5] = view11;
    outMatrix[6] = view21;
    outMatrix[7] = position.y;
    outMatrix[8] = view02;
    outMatrix[9] = view12;
    outMatrix[10] = view22;
    outMatrix[11] = position.z;
    outMatrix[12] = 0.0;
    outMatrix[13] = 0.0;
    outMatrix[14] = 0.0;
    outMatrix[15] = 1.0;
}

void NameTagRenderer::renderBackground(
    VkCommandBuffer cmd, const Vector3d& position, f64 width, f64 height, f64 scale, pipeline::EntityPipeline& pipeline)
{
    // 背景渲染已在 WorldTextRenderer::renderText 中实现
    // 此方法保留用于未来扩展（如需要特殊背景效果）
    (void)cmd;
    (void)position;
    (void)width;
    (void)height;
    (void)scale;
    (void)pipeline;
}

} // namespace mc::client::renderer::entity::util
