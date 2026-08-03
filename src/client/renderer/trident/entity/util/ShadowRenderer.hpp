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
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {

class Entity;
class IWorld;

namespace client {
class ClientEntity;
}

namespace client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
struct EntityMesh;    // 前向声明
} // namespace client::renderer::entity::pipeline

namespace client::renderer::entity::util {

/**
 * @brief 阴影渲染器
 *
 * 负责在实体下方渲染阴影。
 * 阴影大小根据实体尺寸和与地面的距离动态调整。
 *
 * 阴影渲染算法：
 * 1. 计算阴影覆盖的方块范围 [x-radius, x+radius] × [y-radius, y] × [z-radius, z+radius]
 * 2. 对范围内的每个方块位置：
 *    a. 获取下方方块状态
 *    b. 检查渲染类型、光照等级、碰撞形状
 *    c. 计算阴影透明度（基础透明度 × 相机距离衰减 × 高度衰减 × 亮度）
 *    d. 绘制阴影四边形（根据方块形状裁剪）
 * 3. 透明度受以下因素影响：
 *    - 到相机的距离衰减（距离 > 16 格时消失，参考 MC EntityRenderer.extractShadow()）
 *    - 实体到地面的高度
 *    - 幼年实体减半
 *    - 方块位置亮度
 *
 * 相机位置由 EntityRendererManager::setCameraInfo() 每帧设置，
 * 通过 setCameraPosition() 存储在 s_cameraPosition 中。
 */
class ShadowRenderer {
public:
    /**
     * @brief 初始化阴影渲染器
     *
     * @param pipeline 实体渲染管线
     * @return 成功或错误
     */
    static bool initialize(pipeline::EntityPipeline& pipeline);

    /**
     * @brief 清理阴影渲染器资源
     */
    static void cleanup();

    /**
     * @brief 检查阴影是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * @brief 设置相机位置（每帧调用）
     *
     * 参考 MC EntityRenderDispatcher.distanceToSqr()：
     * 阴影透明度需要根据实体到相机的距离进行衰减。
     * 由 EntityRendererManager::setCameraInfo() 每帧调用。
     *
     * @param position 相机世界坐标
     */
    static void setCameraPosition(const Vector3d& position);

    /**
     * @brief 渲染实体阴影（GPU管线路径 - ClientEntity 版本）
     *
     * @param cmd Vulkan 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分 tick
     * @param shadowRadius 阴影半径
     * @param shadowAlpha 阴影透明度
     * @param pipeline 实体渲染管线
     */
    static void renderShadow(VkCommandBuffer cmd,
        ClientEntity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 shadowAlpha,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 渲染实体阴影（GPU管线路径 - Entity 版本）
     *
     * 使用 MC 1.16.5 风格的方块级阴影渲染：
     * - 遍历实体周围的方块
     * - 在每个有合适表面的方块上绘制阴影四边形
     * - 阴影形状根据方块碰撞箱裁剪
     *
     * @param cmd Vulkan 命令缓冲区
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @param shadowRadius 阴影半径
     * @param shadowAlpha 阴影透明度
     * @param pipeline 实体渲染管线
     */
    static void renderShadow(VkCommandBuffer cmd,
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 shadowAlpha,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 渲染实体阴影（CPU路径 - 已废弃）
     *
     * @deprecated 使用 renderShadow(cmd, entity, ...) 代替
     */
    static void renderShadow(Entity& entity, f64 partialTicks, f64 shadowRadius, f64 shadowAlpha);

private:
    /**
     * @brief 创建阴影网格
     */
    static bool createShadowMesh(pipeline::EntityPipeline& pipeline);

    /**
     * @brief 计算阴影透明度（Entity 版本）
     */
    [[nodiscard]] static f64 computeShadowAlpha(Entity& entity, f64 partialTicks, f64 shadowRadius, f64 baseAlpha);

    /**
     * @brief 计算阴影透明度（ClientEntity 版本）
     */
    [[nodiscard]] static f64 computeShadowAlpha(
        ClientEntity& entity, f64 partialTicks, f64 shadowRadius, f64 baseAlpha);

    /**
     * @brief 在单个方块上渲染阴影
     *
     * 参考 MC 1.16.5 EntityRendererManager.renderBlockShadow()
     *
     * @param cmd Vulkan 命令缓冲区
     * @param world 世界引用
     * @param blockX 方块 X 坐标
     * @param blockY 方块 Y 坐标
     * @param blockZ 方块 Z 坐标
     * @param entityX 实体 X 坐标（插值后）
     * @param entityY 实体 Y 坐标（插值后）
     * @param entityZ 实体 Z 坐标（插值后）
     * @param shadowRadius 阴影半径
     * @param baseAlpha 基础透明度
     * @param pipeline 实体渲染管线
     */
    static void renderBlockShadow(VkCommandBuffer cmd,
        IWorld& world,
        i32 blockX,
        i32 blockY,
        i32 blockZ,
        f64 entityX,
        f64 entityY,
        f64 entityZ,
        f64 shadowRadius,
        f64 baseAlpha,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 渲染简化阴影（无世界引用时使用）
     *
     * 当实体没有世界引用时，使用简化的阴影渲染：
     * - 使用预创建的圆形阴影网格
     * - 向下扫描检测地面高度
     *
     * @param cmd Vulkan 命令缓冲区
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @param shadowRadius 阴影半径
     * @param shadowAlpha 阴影透明度
     * @param pipeline 实体渲染管线
     */
    static void renderShadowSimple(VkCommandBuffer cmd,
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 shadowAlpha,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 绘制单个阴影顶点
     *
     * 参考 MC 1.16.5 EntityRendererManager.shadowVertex()
     *
     * @param cmd Vulkan 命令缓冲区
     * @param pipeline 实体渲染管线
     * @param alpha 透明度
     * @param x 相对 X 坐标
     * @param y 相对 Y 坐标（阴影高度）
     * @param z 相对 Z 坐标
     * @param texU 纹理 U 坐标
     * @param texV 纹理 V 坐标
     */
    static void shadowVertex(
        VkCommandBuffer cmd, pipeline::EntityPipeline& pipeline, f32 alpha, f32 x, f32 y, f32 z, f32 texU, f32 texV);

    /**
     * @brief 获取阴影圆盘顶点
     */
    static void getShadowVertices(f64 radius, u32 segments, std::vector<f32>& vertices, std::vector<u32>& indices);

    static bool s_initialized;
    static u32 s_segments;
    static std::vector<f32> s_shadowVertices;
    static std::vector<u32> s_shadowIndices;
    static pipeline::EntityMesh* s_shadowMesh;
    static Vector3d s_cameraPosition;
};

} // namespace client::renderer::entity::util
} // namespace mc
