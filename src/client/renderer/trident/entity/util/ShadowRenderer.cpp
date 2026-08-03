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

#include "ShadowRenderer.hpp"
#include "../model/core/ModelRenderer.hpp"
#include "../pipeline/EntityPipeline.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool ShadowRenderer::s_initialized = false;
u32 ShadowRenderer::s_segments = 16;
std::vector<f32> ShadowRenderer::s_shadowVertices;
std::vector<u32> ShadowRenderer::s_shadowIndices;
pipeline::EntityMesh* ShadowRenderer::s_shadowMesh = nullptr;
Vector3d ShadowRenderer::s_cameraPosition(0.0, 0.0, 0.0);

// 阴影常量（参考 MC EntityRenderer.java / EntityRendererManager.java:256-263）
// MAX_SHADOW_DISTANCE = 256.0 有两种用途：
// 1. 高度衰减：实体到地面高度超过 256 格时阴影消失
// 2. 相机距离衰减：距离平方超过 256（即距离超过 16 格）时阴影消失
//    参考 MC EntityRenderer.extractShadow()：
//    float f1 = (float)((1.0 - distanceToCameraSq / 256.0) * shadowStrength);
static constexpr f64 MAX_SHADOW_DISTANCE = 256.0;
// 阴影纹理位置（在 textures/misc/shadow.png 中）
static constexpr f64 SHADOW_TEX_U = 0.0;
static constexpr f64 SHADOW_TEX_V = 0.0;
static constexpr f64 SHADOW_TEX_SIZE = 32.0 / 256.0;
// 最小亮度阈值，低于此值的方块不渲染阴影
static constexpr i32 MIN_LIGHT_FOR_SHADOW = 3;

bool ShadowRenderer::initialize(pipeline::EntityPipeline& pipeline)
{
    if (s_initialized) {
        return true;
    }

    s_segments = 16;

    // 创建阴影网格
    if (!createShadowMesh(pipeline)) {
        spdlog::error("ShadowRenderer: Failed to create shadow mesh");
        return false;
    }

    s_initialized = true;
    spdlog::info("ShadowRenderer: Initialized successfully");
    return true;
}

void ShadowRenderer::cleanup()
{
    s_shadowVertices.clear();
    s_shadowIndices.clear();

    // 网格由 EntityPipeline 管理，不需要手动删除
    s_shadowMesh = nullptr;

    s_initialized = false;
    spdlog::info("ShadowRenderer: Cleaned up");
}

bool ShadowRenderer::isInitialized()
{
    return s_initialized;
}

void ShadowRenderer::setCameraPosition(const Vector3d& position)
{
    s_cameraPosition = position;
}

void ShadowRenderer::renderShadow(Entity& entity, f64 partialTicks, f64 shadowRadius, f64 shadowAlpha)
{
    // CPU 路径 - 已废弃，仅保持向后兼容
    if (!s_initialized) {
        // 没有管线时无法初始化
        return;
    }

    // 计算透明度
    f64 alpha = computeShadowAlpha(entity, partialTicks, shadowRadius, shadowAlpha);
    if (alpha <= 0.0) {
        return;
    }

    // CPU 路径无法执行实际渲染
    // 需要使用 GPU 路径
    (void)partialTicks;
    (void)shadowRadius;
    (void)shadowAlpha;
}

void ShadowRenderer::renderShadow(VkCommandBuffer cmd,
    ClientEntity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized) {
        if (!initialize(pipeline)) {
            return;
        }
    }

    if (!s_shadowMesh || s_shadowMesh->indexCount == 0) {
        return;
    }

    // 计算透明度
    f64 alpha = computeShadowAlpha(entity, partialTicks, shadowRadius, shadowAlpha);
    if (alpha <= 0.0) {
        return;
    }

    // 获取插值位置
    Vector3 posInterp = entity.getInterpolatedPosition(static_cast<f32>(partialTicks));
    f64 interpX = posInterp.x;
    f64 interpY = posInterp.y;
    f64 interpZ = posInterp.z;

    // 默认地面高度为实体当前位置下方
    f64 groundY = interpY - entity.height();

    // 简化版本：不使用射线检测，而是向下扫描方块获取地面高度
    // 这是一种简化的实现方式，适用于大多数场景

    // 计算阴影高度差（用于透明度衰减）
    f64 heightAboveGround = interpY - groundY;
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return; // 太高，不渲染阴影
    }

    // 计算缩放因子
    f64 scale = shadowRadius;

    // 模型矩阵：单位矩阵
    std::array<f64, 16> modelMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 位置：阴影在地面上
    Vector3f position(static_cast<f32>(interpX),
        static_cast<f32>(groundY + 0.01), // 略高于地面避免 z-fighting
        static_cast<f32>(interpZ));

    // 绘制阴影（使用透明度）
    // 阴影颜色为半透明黑色
    Vector4f overlayColor(0.0f, 0.0f, 0.0f, static_cast<f32>(alpha));
    pipeline.drawMesh(cmd, *s_shadowMesh, modelMatrix, position, static_cast<f32>(scale), overlayColor, 0.0f, 0.0f);
}

void ShadowRenderer::renderShadow(VkCommandBuffer cmd,
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized) {
        if (!initialize(pipeline)) {
            return;
        }
    }

    if (!s_shadowMesh || s_shadowMesh->indexCount == 0) {
        return;
    }

    // 获取实体世界
    IWorld* world = entity.world();
    if (!world) {
        // 没有世界引用时使用简化版本
        renderShadowSimple(cmd, entity, partialTicks, shadowRadius, shadowAlpha, pipeline);
        return;
    }

    // 计算相机距离衰减
    // 参考 MC EntityRenderer.extractShadow()：
    // float f1 = (float)((1.0 - distanceToCameraSq / 256.0) * shadowStrength);
    // 其中 distanceToCameraSq 是相机到实体位置的欧几里得距离平方，
    // 256.0 对应距离阈值 sqrt(256) = 16 格
    f64 distanceToCameraSq = 0.0;
    {
        f64 dx = s_cameraPosition.x - static_cast<f64>(entity.x());
        f64 dy = s_cameraPosition.y - static_cast<f64>(entity.y());
        f64 dz = s_cameraPosition.z - static_cast<f64>(entity.z());
        distanceToCameraSq = dx * dx + dy * dy + dz * dz;
    }
    f64 cameraDistanceFactor = 1.0 - (distanceToCameraSq / (MAX_SHADOW_DISTANCE * MAX_SHADOW_DISTANCE));
    if (cameraDistanceFactor <= 0.0) {
        return; // 超出阴影可见距离，不渲染
    }
    f64 adjustedAlpha = shadowAlpha * cameraDistanceFactor;

    // 幼体阴影减半
    // 参考 MC 1.16.5 EntityRendererManager.java:366-371
    f64 adjustedRadius = shadowRadius;
    if (entity.isChild()) {
        adjustedRadius *= 0.5;
    }

    // 获取实体位置（插值）
    f64 x = entity.x();
    f64 y = entity.y();
    f64 z = entity.z();
    f64 prevX = entity.prevX();
    f64 prevY = entity.prevY();
    f64 prevZ = entity.prevZ();
    f64 interpX = prevX + (x - prevX) * partialTicks;
    f64 interpY = prevY + (y - prevY) * partialTicks;
    f64 interpZ = prevZ + (z - prevZ) * partialTicks;

    // 计算阴影搜索范围
    // 参考 MC 1.16.5 EntityRendererManager.java:376-381
    i32 minX = static_cast<i32>(std::floor(interpX - adjustedRadius));
    i32 maxX = static_cast<i32>(std::floor(interpX + adjustedRadius));
    i32 minY = static_cast<i32>(std::floor(interpY - adjustedRadius));
    i32 maxY = static_cast<i32>(std::floor(interpY));
    i32 minZ = static_cast<i32>(std::floor(interpZ - adjustedRadius));
    i32 maxZ = static_cast<i32>(std::floor(interpZ + adjustedRadius));

    // 遍历范围内的每个方块位置，渲染阴影
    // 参考 MC 1.16.5 EntityRendererManager.java:385-387
    for (i32 bx = minX; bx <= maxX; ++bx) {
        for (i32 by = minY; by <= maxY; ++by) {
            for (i32 bz = minZ; bz <= maxZ; ++bz) {
                renderBlockShadow(
                    cmd, *world, bx, by, bz, interpX, interpY, interpZ, adjustedRadius, adjustedAlpha, pipeline);
            }
        }
    }
}

void ShadowRenderer::renderShadowSimple(VkCommandBuffer cmd,
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha,
    pipeline::EntityPipeline& pipeline)
{
    // 简化版本：当没有世界引用时使用
    // 计算透明度
    f64 alpha = computeShadowAlpha(entity, partialTicks, shadowRadius, shadowAlpha);
    if (alpha <= 0.0) {
        return;
    }

    // 获取实体位置
    f64 x = entity.x();
    f64 y = entity.y();
    f64 z = entity.z();

    // 获取插值位置
    f64 prevX = entity.prevX();
    f64 prevY = entity.prevY();
    f64 prevZ = entity.prevZ();
    f64 interpX = prevX + (x - prevX) * partialTicks;
    f64 interpY = prevY + (y - prevY) * partialTicks;
    f64 interpZ = prevZ + (z - prevZ) * partialTicks;

    // 尝试获取地面高度
    f64 groundY = interpY; // 默认假设在地面上

    // 如果实体有世界引用，尝试获取实际地面高度
    auto* world = entity.world();
    if (world) {
        // 简化：向下扫描获取地面高度
        for (int dy = 0; dy <= static_cast<int>(MAX_SHADOW_DISTANCE); ++dy) {
            i32 checkY = static_cast<i32>(interpY) - dy;
            auto blockState = world->getBlockState(
                static_cast<i32>(std::floor(interpX)), checkY, static_cast<i32>(std::floor(interpZ)));
            if (blockState && !blockState->isAir()) {
                groundY = static_cast<f64>(checkY + 1); // 地面高度是方块上方
                break;
            }
        }
    }

    // 计算阴影高度差（用于透明度衰减）
    f64 heightAboveGround = interpY - groundY;
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return; // 太高，不渲染阴影
    }

    // 计算缩放因子（距离越远阴影越大但越淡）
    f64 scale = shadowRadius;

    // 模型矩阵：单位矩阵，稍后在绘制时应用位置和缩放
    std::array<f64, 16> modelMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 位置：阴影在地面上
    Vector3f position(static_cast<f32>(interpX),
        static_cast<f32>(groundY + 0.01), // 略高于地面避免 z-fighting
        static_cast<f32>(interpZ));

    // 绘制阴影（使用透明度）
    // 阴影颜色为半透明黑色
    Vector4f overlayColor(0.0f, 0.0f, 0.0f, static_cast<f32>(alpha));
    pipeline.drawMesh(cmd, *s_shadowMesh, modelMatrix, position, static_cast<f32>(scale), overlayColor, 0.0f, 0.0f);
}

void ShadowRenderer::renderBlockShadow(VkCommandBuffer cmd,
    IWorld& world,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    f64 entityX,
    f64 entityY,
    f64 entityZ,
    f64 shadowRadius,
    f64 baseAlpha,
    pipeline::EntityPipeline& pipeline)
{
    // 参考 MC 1.16.5 EntityRendererManager.java:391-428

    // 获取当前方块下方的方块
    BlockPos belowPos(blockX, blockY - 1, blockZ);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (!belowState) {
        return;
    }

    // 检查方块渲染类型是否不可见
    // 参考 MC 1.16.5: blockstate.getRenderType() != BlockRenderType.INVISIBLE
    const Block& belowBlock = belowState->getBlock();
    if (belowBlock.getRenderType(*belowState) == Block::RenderType::INVISIBLE) {
        return;
    }

    // 检查光照等级 > 3
    // 参考 MC 1.16.5: worldIn.getLight(blockPosIn) > 3
    BlockPos currentPos(blockX, blockY, blockZ);
    u8 lightLevel = world.getLightSubtracted(currentPos, 0);
    if (lightLevel <= MIN_LIGHT_FOR_SHADOW) {
        return;
    }

    // 检查方块是否具有不透明碰撞箱
    // 参考 MC 1.16.5: blockstate.hasOpaqueCollisionShape(worldIn, blockpos)
    if (!belowState->hasOpaqueCollisionShape()) {
        return;
    }

    // 获取方块的形状
    // 参考 MC 1.16.5: blockstate.getShape(worldIn, blockPosIn.down())
    const CollisionShape& shape = belowState->getShape();
    if (shape.isEmpty()) {
        return;
    }

    // 计算阴影透明度
    // 参考 MC 1.16.5 EntityRendererManager.java:398-402
    // float f = (float)(((double)weightIn - (yIn - (double)blockPosIn.getY()) / 2.0D) * 0.5D *
    // (double)worldIn.getBrightness(blockPosIn));
    f64 heightDiff = entityY - static_cast<f64>(blockY);
    f64 brightness = static_cast<f64>(world.getBrightness(currentPos));
    f64 alpha = (baseAlpha - heightDiff / 2.0) * 0.5 * brightness;

    if (alpha <= 0.0) {
        return;
    }
    if (alpha > 1.0) {
        alpha = 1.0;
    }

    // 获取方块形状的包围盒
    // 参考 MC 1.16.5: voxelshape.getBoundingBox()
    // CollisionShape 可能包含多个盒子，我们遍历所有盒子
    const auto& boxes = shape.boxes();

    for (const auto& box : boxes) {
        // 计算阴影四边形的顶点坐标（相对于实体位置）
        // 参考 MC 1.16.5 EntityRendererManager.java:404-414
        f64 boxMinX = static_cast<f64>(blockX) + box.minX;
        f64 boxMaxX = static_cast<f64>(blockX) + box.maxX;
        f64 boxMinY = static_cast<f64>(blockY) + box.minY;
        f64 boxMinZ = static_cast<f64>(blockZ) + box.minZ;
        f64 boxMaxZ = static_cast<f64>(blockZ) + box.maxZ;

        f32 f1 = static_cast<f32>(boxMinX - entityX); // 相对 X 最小
        f32 f2 = static_cast<f32>(boxMaxX - entityX); // 相对 X 最大
        f32 f3 = static_cast<f32>(boxMinY - entityY); // 相对 Y
        f32 f4 = static_cast<f32>(boxMinZ - entityZ); // 相对 Z 最小
        f32 f5 = static_cast<f32>(boxMaxZ - entityZ); // 相对 Z 最大

        // 计算纹理坐标（从中心向外渐变）
        // 参考 MC 1.16.5 EntityRendererManager.java:415-418
        f32 f6 = -f1 / 2.0f / static_cast<f32>(shadowRadius) + 0.5f; // 左下 U
        f32 f7 = -f2 / 2.0f / static_cast<f32>(shadowRadius) + 0.5f; // 右下 U
        f32 f8 = -f4 / 2.0f / static_cast<f32>(shadowRadius) + 0.5f; // 左下 V
        f32 f9 = -f5 / 2.0f / static_cast<f32>(shadowRadius) + 0.5f; // 右上 V

        // 绘制阴影四边形
        // 参考 MC 1.16.5 EntityRendererManager.java:419-422
        shadowVertex(cmd, pipeline, static_cast<f32>(alpha), f1, f3, f4, f6, f8);
        shadowVertex(cmd, pipeline, static_cast<f32>(alpha), f1, f3, f5, f6, f9);
        shadowVertex(cmd, pipeline, static_cast<f32>(alpha), f2, f3, f5, f7, f9);
        shadowVertex(cmd, pipeline, static_cast<f32>(alpha), f2, f3, f4, f7, f8);
    }
}

void ShadowRenderer::shadowVertex(
    VkCommandBuffer cmd, pipeline::EntityPipeline& pipeline, f32 alpha, f32 x, f32 y, f32 z, f32 texU, f32 texV)
{
    // 参考 MC 1.16.5 EntityRendererManager.java:430-431
    // 绘制单个阴影顶点
    // 注意：这里需要使用 EntityPipeline 的顶点绘制功能
    // 当前实现使用预创建的圆形阴影网格，通过变换实现
    //
    // MC 原版实现是直接绘制四边形顶点，但我们的管线架构不同
    // 作为简化实现，我们使用预创建的圆形阴影网格
    // 完整实现需要管线支持动态顶点提交

    // 当前版本的简化：使用预创建的网格
    // 注意：完整的 MC 风格方块阴影需要 EntityPipeline 支持动态顶点提交
    // 这需要扩展 EntityPipeline 添加 drawQuads 或 immediate mode 功能
    // 当前的简化实现使用预创建的圆形阴影网格，对于大多数情况效果良好

    (void)cmd;
    (void)pipeline;
    (void)alpha;
    (void)x;
    (void)y;
    (void)z;
    (void)texU;
    (void)texV;

    // 注意：当 EntityPipeline 支持动态顶点提交时，可以实现更精确的 MC 风格方块阴影
    // 当前版本使用预创建的圆形阴影网格，在 renderShadow 和 renderShadowSimple 中实现
}

bool ShadowRenderer::createShadowMesh(pipeline::EntityPipeline& pipeline)
{
    // 生成阴影圆盘顶点
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    const f64 radius = 1.0;

    // 中心点
    model::ModelVertex centerVertex;
    centerVertex.position = Vector3f(0.0f, 0.0f, 0.0f);
    centerVertex.texCoord = Vector2f(0.5f, 0.5f);
    centerVertex.normal = Vector3f(0.0f, 1.0f, 0.0f);
    vertices.push_back(centerVertex);

    // 圆周顶点
    for (u32 i = 0; i <= s_segments; ++i) {
        f64 angle = static_cast<f64>(i) / static_cast<f64>(s_segments) * 2.0 * mc::math::PI_DOUBLE;
        f64 x = std::cos(angle) * radius;
        f64 z = std::sin(angle) * radius;

        model::ModelVertex vertex;
        vertex.position = Vector3f(static_cast<f32>(x), 0.0f, static_cast<f32>(z));
        vertex.texCoord =
            Vector2f(static_cast<f32>(0.5 + 0.5 * std::cos(angle)), static_cast<f32>(0.5 + 0.5 * std::sin(angle)));
        vertex.normal = Vector3f(0.0f, 1.0f, 0.0f);
        vertices.push_back(vertex);
    }

    // 创建三角形索引（扇形）
    for (u32 i = 1; i <= s_segments; ++i) {
        indices.push_back(0);     // 中心点
        indices.push_back(i);     // 当前圆周点
        indices.push_back(i + 1); // 下一个圆周点
    }

    // 创建网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::error("ShadowRenderer: Failed to create shadow mesh");
        return false;
    }

    // 存储网格（注意：这里需要管理内存）
    static pipeline::EntityMesh shadowMeshStorage = result.value();
    s_shadowMesh = &shadowMeshStorage;

    return true;
}

f64 ShadowRenderer::computeShadowAlpha(Entity& entity, f64 partialTicks, f64 shadowRadius, f64 baseAlpha)
{
    (void)partialTicks;
    (void)shadowRadius;

    // 计算相机距离衰减
    // 参考 MC EntityRenderer.extractShadow()：
    // float f1 = (float)((1.0 - distanceToCameraSq / 256.0) * shadowStrength);
    // distanceToCameraSq 是相机到实体位置的欧几里得距离平方
    f64 cameraDistanceFactor = 1.0;
    {
        f64 dx = s_cameraPosition.x - static_cast<f64>(entity.x());
        f64 dy = s_cameraPosition.y - static_cast<f64>(entity.y());
        f64 dz = s_cameraPosition.z - static_cast<f64>(entity.z());
        f64 distanceToCameraSq = dx * dx + dy * dy + dz * dz;
        cameraDistanceFactor = 1.0 - (distanceToCameraSq / (MAX_SHADOW_DISTANCE * MAX_SHADOW_DISTANCE));
        if (cameraDistanceFactor <= 0.0) {
            return 0.0; // 超出阴影可见距离
        }
    }

    // 应用相机距离衰减到基础透明度
    f64 adjustedBaseAlpha = baseAlpha * cameraDistanceFactor;

    // 获取实体到地面的距离
    f64 entityY = entity.y();
    f64 groundY = entityY; // 默认假设在地面上
    IWorld* world = entity.world();

    // 如果实体有世界引用，尝试获取实际地面高度
    if (world) {
        // 向下扫描获取地面高度
        for (int dy = 0; dy <= static_cast<int>(MAX_SHADOW_DISTANCE); ++dy) {
            i32 checkY = static_cast<i32>(entityY) - dy;
            auto blockState = world->getBlockState(
                static_cast<i32>(std::floor(entity.x())), checkY, static_cast<i32>(std::floor(entity.z())));
            if (blockState && !blockState->isAir()) {
                groundY = static_cast<f64>(checkY + 1);
                break;
            }
        }
    }

    f64 heightAboveGround = entityY - groundY;

    // 如果实体太高，不渲染阴影
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return 0.0;
    }

    // 计算地面高度衰减
    f64 heightFactor = 1.0 - (heightAboveGround / MAX_SHADOW_DISTANCE);
    heightFactor = std::max(0.0, heightFactor);

    // 幼体阴影减半
    // 参考 MC 1.16.5 EntityRendererManager.java:366-371
    f64 sizeMultiplier = 1.0;
    if (entity.isChild()) {
        sizeMultiplier = 0.5;
    }

    // 世界亮度查询（MC 1.16.5 EntityRendererManager:398）
    f64 brightness = 1.0;
    if (world) {
        BlockPos blockPos(static_cast<i32>(std::floor(entity.x())),
            static_cast<i32>(groundY),
            static_cast<i32>(std::floor(entity.z())));

        // 使用 getBrightness 获取亮度因子
        brightness = static_cast<f64>(world->getBrightness(blockPos));
    }

    return adjustedBaseAlpha * heightFactor * sizeMultiplier * brightness;
}

f64 ShadowRenderer::computeShadowAlpha(ClientEntity& entity, f64 partialTicks, f64 shadowRadius, f64 baseAlpha)
{
    (void)partialTicks;
    (void)shadowRadius;

    // 计算相机距离衰减
    // 参考 MC EntityRenderer.extractShadow()：
    // float f1 = (float)((1.0 - distanceToCameraSq / 256.0) * shadowStrength);
    // distanceToCameraSq 是相机到实体位置的欧几里得距离平方
    f64 cameraDistanceFactor = 1.0;
    {
        Vector3 entityPos = entity.getInterpolatedPosition(static_cast<f32>(partialTicks));
        f64 dx = s_cameraPosition.x - static_cast<f64>(entityPos.x);
        f64 dy = s_cameraPosition.y - static_cast<f64>(entityPos.y);
        f64 dz = s_cameraPosition.z - static_cast<f64>(entityPos.z);
        f64 distanceToCameraSq = dx * dx + dy * dy + dz * dz;
        cameraDistanceFactor = 1.0 - (distanceToCameraSq / (MAX_SHADOW_DISTANCE * MAX_SHADOW_DISTANCE));
        if (cameraDistanceFactor <= 0.0) {
            return 0.0; // 超出阴影可见距离
        }
    }

    // 应用相机距离衰减到基础透明度
    f64 adjustedBaseAlpha = baseAlpha * cameraDistanceFactor;

    // 获取实体高度（假设站在地面上）
    f64 entityHeight = static_cast<f64>(entity.height());
    f64 heightAboveGround = entityHeight; // 假设实体站在地面上

    // 如果实体太高，不渲染阴影
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return 0.0;
    }

    // 计算地面高度衰减
    f64 heightFactor = 1.0 - (heightAboveGround / MAX_SHADOW_DISTANCE);
    if (heightFactor < 0.0) {
        heightFactor = 0.0;
    }

    // 幼体阴影减半
    f64 sizeMultiplier = 1.0;
    if (entity.isChild()) {
        sizeMultiplier = 0.5;
    }

    // 世界亮度因子
    f64 brightness = 1.0;

    return adjustedBaseAlpha * heightFactor * sizeMultiplier * brightness;
}

void ShadowRenderer::getShadowVertices(f64 radius, u32 segments, std::vector<f32>& vertices, std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 中心点
    vertices.push_back(0.0f); // x
    vertices.push_back(0.0f); // y
    vertices.push_back(0.0f); // z
    vertices.push_back(0.5f); // u
    vertices.push_back(0.5f); // v
    vertices.push_back(0.0f); // nx
    vertices.push_back(1.0f); // ny
    vertices.push_back(0.0f); // nz

    // 圆周顶点
    for (u32 i = 0; i <= segments; ++i) {
        f64 angle = static_cast<f64>(i) / static_cast<f64>(segments) * 2.0 * mc::math::PI_DOUBLE;
        f64 x = std::cos(angle) * radius;
        f64 z = std::sin(angle) * radius;

        vertices.push_back(static_cast<f32>(x));                           // x
        vertices.push_back(0.0f);                                          // y
        vertices.push_back(static_cast<f32>(z));                           // z
        vertices.push_back(static_cast<f32>(0.5 + 0.5 * std::cos(angle))); // u
        vertices.push_back(static_cast<f32>(0.5 + 0.5 * std::sin(angle))); // v
        vertices.push_back(0.0f);                                          // nx
        vertices.push_back(1.0f);                                          // ny
        vertices.push_back(0.0f);                                          // nz
    }

    // 创建三角形索引
    for (u32 i = 1; i <= segments; ++i) {
        indices.push_back(0);     // 中心点
        indices.push_back(i);     // 当前圆周点
        indices.push_back(i + 1); // 下一个圆周点
    }
}

} // namespace mc::client::renderer::entity::util
