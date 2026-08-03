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

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <array>
#include <string>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {

class Entity;

namespace client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
}

namespace client::renderer::entity::util {

/**
 * @brief 名称标签渲染器
 *
 * 负责在实体上方渲染名称标签。
 * 支持自定义颜色、背景和可见性控制。
 *
 * 参考 MC 1.16.5 EntityRenderer.renderNameTag()
 */
class NameTagRenderer {
public:
    /**
     * @brief 初始化名称标签渲染器
     */
    static bool initialize(pipeline::EntityPipeline& pipeline);

    /**
     * @brief 清理资源
     */
    static void cleanup();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * @brief 设置相机位置（用于计算距离和 billboard）
     */
    static void setCameraPosition(const Vector3d& position);

    /**
     * @brief 设置视图矩阵（用于 billboard 计算）
     */
    static void setViewMatrix(const std::array<f64, 16>& viewMatrix);

    /**
     * @brief 设置视锥体（用于视锥剔除）
     *
     * @param frustum 视锥体对象
     */
    static void setFrustum(const mc::math::frustum::Frustum& frustum);

    /**
     * @brief 渲染实体名称标签（GPU管线路径）
     */
    static void renderNameTag(VkCommandBuffer cmd,
        Entity& entity,
        const std::string& displayName,
        f64 partialTicks,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 渲染实体名称标签（CPU路径 - 已废弃）
     */
    static void renderNameTag(Entity& entity, const std::string& displayName, f64 partialTicks);

    /**
     * @brief 检查是否应该渲染名称标签
     */
    [[nodiscard]] static bool shouldRenderNameTag(Entity& entity, f64 distanceToCamera);

    /**
     * @brief 设置最大可见距离
     */
    static void setMaxDistance(f64 distance);

    /**
     * @brief 获取最大可见距离
     */
    [[nodiscard]] static f64 maxDistance();

    // ========== 样式设置 ==========

    static void setScale(f64 scale);
    static void setBackgroundColor(u8 r, u8 g, u8 b, u8 a);
    static void setShowBackground(bool show);

private:
    [[nodiscard]] static Vector3d calculateNameTagPosition(Entity& entity, f64 partialTicks);

    [[nodiscard]] static f64 calculateScale(f64 distanceToCamera);

    static void computeBillboardMatrix(const Vector3d& position, std::array<f64, 16>& outMatrix);

    static void renderBackground(VkCommandBuffer cmd,
        const Vector3d& position,
        f64 width,
        f64 height,
        f64 scale,
        pipeline::EntityPipeline& pipeline);

    static bool s_initialized;
    static f64 s_maxDistance;
    static f64 s_scale;
    static bool s_showBackground;
    static u8 s_bgColorR;
    static u8 s_bgColorG;
    static u8 s_bgColorB;
    static u8 s_bgColorA;
    static Vector3d s_cameraPosition;
    static std::array<f64, 16> s_viewMatrix;
    static mc::math::frustum::Frustum s_frustum; // 视锥体（用于视锥剔除）

    static constexpr f64 DEFAULT_MAX_DISTANCE = 64.0;
    static constexpr f64 DEFAULT_SCALE = 0.025;
    static constexpr f64 BACKGROUND_PADDING = 0.25;
    static constexpr f64 HEIGHT_OFFSET = 0.5; // MC 1.16.5: 实体高度之上的偏移
    static constexpr f64 CHAR_WIDTH = 0.5;
    static constexpr f64 CHAR_HEIGHT = 1.0;
};

} // namespace client::renderer::entity::util
} // namespace mc
