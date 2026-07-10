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

#include "SpecialEntityRenderers.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/renderer/trident/entity/util/BlockMeshBuilder.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::renderer::special {

// ==================== EnderCrystalRenderer ====================

EnderCrystalRenderer::EnderCrystalRenderer()
    : m_model(std::make_shared<model::projectile::EnderCrystalModel>())
{
    m_shadowSize = 0.0f;
}

void EnderCrystalRenderer::render(Entity& entity, f64 partialTicks)
{
    // 末影水晶渲染：
    // 1. 计算水晶旋转动画（innerRotation）
    // 2. 计算水晶浮动偏移（上下浮动）
    // 3. 渲染水晶核心和玻璃外壳
    // 4. 如果有束目标，渲染指向末影龙的光束
    // TODO: 实现末影水晶浮动偏移和光束渲染

    // 设置模型角度（旋转动画）
    f64 ageInTicks = static_cast<f64>(entity.ticksExisted()) + partialTicks;
    m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, 1.0);

    // 渲染模型
    m_model->render(1.0 / 16.0);
}

// ==================== ShulkerBulletRenderer ====================

ShulkerBulletRenderer::ShulkerBulletRenderer()
    : m_model(std::make_shared<model::projectile::ShulkerBulletModel>())
{
    m_shadowSize = 0.0f;
}

void ShulkerBulletRenderer::render(Entity& entity, f64 partialTicks)
{
    // 潜影贝子弹渲染：
    // 1. 计算子弹飞行方向
    // 2. 渲染子弹模型（旋转）

    f64 ageInTicks = static_cast<f64>(entity.ticksExisted()) + partialTicks;
    m_model->setAngles(0.0, 0.0, ageInTicks, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
}

// ==================== LlamaSpitRenderer ====================

LlamaSpitRenderer::LlamaSpitRenderer()
    : m_model(std::make_shared<model::projectile::LlamaSpitModel>())
{
    m_shadowSize = 0.0f;
}

void LlamaSpitRenderer::render(Entity& entity, f64 partialTicks)
{
    // 羊驼唾沫渲染：简单的投射物

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== SpectralArrowRenderer ====================

SpectralArrowRenderer::SpectralArrowRenderer()
    : m_model(std::make_shared<model::projectile::SpectralArrowModel>())
{
    m_shadowSize = 0.0f;
}

void SpectralArrowRenderer::render(Entity& entity, f64 partialTicks)
{
    // 光灵箭渲染：与普通箭类似，但有发光效果

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== WitherSkullRenderer ====================

WitherSkullRenderer::WitherSkullRenderer()
    : m_model(std::make_shared<model::projectile::WitherSkullModel>())
{
    m_shadowSize = 0.0f;
}

void WitherSkullRenderer::render(Entity& entity, f64 partialTicks)
{
    // 凋灵之首渲染

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== DragonFireballRenderer ====================

DragonFireballRenderer::DragonFireballRenderer()
    : m_model(std::make_shared<model::projectile::DragonFireballModel>())
{
    m_shadowSize = 0.0f;
}

void DragonFireballRenderer::render(Entity& entity, f64 partialTicks)
{
    // 龙火球渲染

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== EvokerFangsRenderer ====================

EvokerFangsRenderer::EvokerFangsRenderer()
    : m_model(std::make_shared<model::projectile::EvokerFangsModel>())
{
    m_shadowSize = 0.0f;
}

void EvokerFangsRenderer::render(Entity& entity, f64 partialTicks)
{
    // 唤魔者尖牙渲染

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), 0.0, 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== LightningBoltRenderer ====================

LightningBoltRenderer::LightningBoltRenderer()
{
    m_shadowSize = 0.0f;
}

void LightningBoltRenderer::render(Entity& entity, f64 partialTicks)
{
    // CPU 路径 - 已废弃，使用 GPU 路径
    (void)entity;
    (void)partialTicks;
}

void LightningBoltRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 闪电渲染：
    // 1. 使用 boltVertex 作为随机种子，创建确定性随机数生成器
    // 2. 生成 8 个点的偏移数组
    // 3. 渲染 4 次循环 (j=0..3)，每次使用不同的宽度参数
    // 4. 每次循环有 3 个分支 (k=0..2)，k=0 为主闪电，k>0 为分支
    // 5. 颜色：内部亮白色 (0.45, 0.45, 0.5)，外部暗蓝色
    // 6. Alpha 固定为 0.3
    // 7. 使用 additive blending

    (void)context;
    (void)cmd;

    // 获取闪电形状随机种子
    u64 boltVertex = entity.boltVertex();
    if (boltVertex == 0) {
        // 使用 ticksExisted 作为后备种子
        boltVertex = static_cast<u64>(entity.ticksExisted()) + 1;
    }

    // 生成闪电网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    _generateLightningMesh(boltVertex, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // 创建临时网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        return;
    }

    // 获取实体位置
    Vector3f position(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 模型矩阵：单位矩阵
    std::array<f64, 16> modelMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 使用加法混合模式渲染闪电
    pipeline.bind(cmd, pipeline::BlendMode::Additive);

    // 闪电颜色为亮蓝白色，alpha 0.3
    // 颜色通过顶点传递，这里使用白色作为基础
    Vector4f overlayColor(0.45f, 0.45f, 0.5f, 0.3f);

    pipeline.drawMesh(cmd, result.value(), modelMatrix, position, 1.0, overlayColor, 0.0f, 0.0f);

    // 恢复 Alpha 混合模式
    pipeline.bind(cmd, pipeline::BlendMode::Alpha);

    // 销毁临时网格
    pipeline.destroyMesh(result.value());
}

void LightningBoltRenderer::_renderQuadSegment(std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    f32 x1,
    f32 z1,
    i32 segmentY,
    f32 prevX,
    f32 prevZ,
    f32 r,
    f32 g,
    f32 b,
    f32 topWidth,
    f32 bottomWidth,
    bool flipX1,
    bool flipZ1,
    bool flipX2,
    bool flipZ2)
{
    // 渲染一个四边形条带段
    // 顶点位置计算：
    // - 第一个顶点：(x1 ± bottomWidth, segmentY * 16, z1 ± bottomWidth)
    // - 第二个顶点：(prevX ± topWidth, (segmentY + 1) * 16, prevZ ± topWidth)
    // - 第三个顶点：(prevX ± topWidth, (segmentY + 1) * 16, prevZ ± topWidth)
    // - 第四个顶点：(x1 ± bottomWidth, segmentY * 16, z1 ± bottomWidth)

    // TODO: r, g, b 参数当前未使用，待支持逐顶点颜色后启用
    (void)r;
    (void)g;
    (void)b;

    f32 y1 = static_cast<f32>(segmentY * 16);
    f32 y2 = static_cast<f32>((segmentY + 1) * 16);

    // 计算四个顶点
    f32 v0x = x1 + (flipX1 ? bottomWidth : -bottomWidth);
    f32 v0z = z1 + (flipZ1 ? bottomWidth : -bottomWidth);
    f32 v1x = prevX + (flipX1 ? topWidth : -topWidth);
    f32 v1z = prevZ + (flipZ1 ? topWidth : -topWidth);
    f32 v2x = prevX + (flipX2 ? topWidth : -topWidth);
    f32 v2z = prevZ + (flipZ2 ? topWidth : -topWidth);
    f32 v3x = x1 + (flipX2 ? bottomWidth : -bottomWidth);
    f32 v3z = z1 + (flipZ2 ? bottomWidth : -bottomWidth);

    u32 baseIndex = static_cast<u32>(vertices.size());

    // 添加四个顶点
    // 闪电不需要纹理坐标，使用法线朝向 +Y
    vertices.push_back(model::ModelVertex(v0x, y1, v0z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(v1x, y2, v1z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(v2x, y2, v2z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(v3x, y1, v3z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));

    // TODO: 顶点颜色目前通过 overlayColor 统一传递，未来应支持逐顶点颜色

    // 添加两个三角形
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);

    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

void LightningBoltRenderer::_generateLightningMesh(
    u64 boltVertex, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 使用确定性随机数生成器

    // 创建随机数生成器
    mc::math::Random rng(static_cast<u64>(boltVertex));

    // 生成 8 个点的偏移数组
    std::array<f32, 8> offsetX;
    std::array<f32, 8> offsetZ;
    f32 f = 0.0f;
    f32 f1 = 0.0f;

    for (i32 i = 7; i >= 0; --i) {
        // MC 原始偏移逻辑：f += nextInt(11) - 5
        offsetX[i] = f;
        offsetZ[i] = f1;
        f += static_cast<f32>(rng.nextInt(11) - 5);
        f1 += static_cast<f32>(rng.nextInt(11) - 5);
    }

    // 闪电颜色：内部亮蓝白色 (0.45, 0.45, 0.5)
    constexpr f32 r = 0.45f;
    constexpr f32 g = 0.45f;
    constexpr f32 b = 0.5f;

    // 渲染 4 次循环 (j=0..3)
    for (i32 j = 0; j < 4; ++j) {
        // 重新创建随机数生成器，确保每层使用相同的随机序列
        mc::math::Random rng2(static_cast<u64>(boltVertex));

        // 每次循环有 3 个分支 (k=0..2)
        for (i32 k = 0; k < 3; ++k) {
            i32 l = 7;  // 起始段索引
            i32 i1 = 0; // 结束段索引

            // k > 0 时，渲染分支（更短）
            if (k > 0) {
                l = 7 - k;
            }

            if (k > 0) {
                i1 = l - 2;
            }

            // 计算当前段的偏移
            f32 f2 = offsetX[static_cast<size_t>(l)] - offsetX[0];
            f32 f3 = offsetZ[static_cast<size_t>(l)] - offsetZ[0];

            // 从上到下渲染每一段
            for (i32 j1 = l; j1 >= i1; --j1) {
                f32 f4 = f2;
                f32 f5 = f3;

                // 分支使用更大的随机偏移
                if (k == 0) {
                    f2 += static_cast<f32>(rng2.nextInt(11) - 5);
                    f3 += static_cast<f32>(rng2.nextInt(11) - 5);
                } else {
                    f2 += static_cast<f32>(rng2.nextInt(31) - 15);
                    f3 += static_cast<f32>(rng2.nextInt(31) - 15);
                }

                // 计算宽度
                f32 topWidth = 0.1f + static_cast<f32>(j) * 0.2f;
                if (k == 0) {
                    topWidth *= static_cast<f32>(static_cast<f64>(j1) * 0.1 + 1.0);
                }

                f32 bottomWidth = 0.1f + static_cast<f32>(j) * 0.2f;
                if (k == 0) {
                    bottomWidth *= static_cast<f32>((j1 - 1) * 0.1 + 1.0);
                }

                // 渲染 4 个四边形（围绕闪电）
                _renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, false, false, true, false);
                _renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, true, false, true, true);
                _renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, true, true, false, true);
                _renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, false, true, false, false);
            }
        }
    }
}

// ==================== AreaEffectCloudRenderer ====================

AreaEffectCloudRenderer::AreaEffectCloudRenderer()
{
    m_shadowSize = 0.0f;
}

void AreaEffectCloudRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现区域效果云渲染
    // 1. 根据半径缩放
    // 2. 根据效果类型选择颜色
    // 3. 半透明渲染

    (void)entity;
    (void)partialTicks;
}

// ==================== FallingBlockRenderer ====================

FallingBlockRenderer::FallingBlockRenderer()
{
    m_shadowSize = 0.5f;
}

void FallingBlockRenderer::render(Entity& entity, f64 partialTicks)
{
    // CPU 路径 - 已废弃，使用 GPU 管线路径
    (void)entity;
    (void)partialTicks;
}

void FallingBlockRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 从 ClientEntity 读取下落方块状态
    // 对应 MC 1.21.11 FallingBlockRenderer.extractRenderState 中 blockState = entity.getBlockState()
    const ::mc::BlockState* blockState = entity.fallingBlockState();
    if (blockState == nullptr) {
        // 未设置方块状态，不渲染
        return;
    }

    // 获取或创建方块网格
    pipeline::EntityMesh* mesh = _getOrCreateBlockMesh(pipeline, *blockState);
    if (mesh == nullptr || mesh->indexCount == 0) {
        return;
    }

    // ---- 复刻 MC 1.21.11 FallingBlockRenderer.submit() 的变换链 ----
    //   translate(-0.5, 0, -0.5)  // 方块中心对齐实体原点
    //
    // 方块网格顶点已在 BlockMeshBuilder 中乘以 1/16 转换为世界单位（0-1 范围）。
    // 实体位置即为方块原点位置，需要将方块中心偏移到实体位置：
    //   方块范围 [0, 1]，中心在 (0.5, 0.5, 0.5)
    //   translate(-0.5, 0, -0.5) 使方块底部中心对齐实体原点
    //
    // 注意：MC 原版使用 translate(-0.5, 0, -0.5) 而非 translate(-0.5, -0.5, -0.5)，
    // 因为下落方块的实体原点在方块底部（y=0）。

    std::array<f64, 16> modelMatrix = {
        1.0, 0.0, 0.0, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, -0.5, 0.0, 0.0, 0.0, 1.0};

    // 实体位置（插值）
    Vector3f position(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 获取方块默认着色颜色（如草方块等需要 tint 的方块）
    const u32 tintColor = ChunkMesher::getDefaultBlockTintColor(blockState);
    const f32 r = static_cast<f32>(tintColor & 0xFFu) / 255.0f;
    const f32 g = static_cast<f32>((tintColor >> 8) & 0xFFu) / 255.0f;
    const f32 b = static_cast<f32>((tintColor >> 16) & 0xFFu) / 255.0f;
    const f32 a = static_cast<f32>((tintColor >> 24) & 0xFFu) / 255.0f;
    Vector4f overlayColor(r, g, b, a);

    // ---- 切换到方块纹理图集 ----
    // 方块纹理 UV 基于区块纹理图集（ChunkTextureAtlas），而非实体纹理图集。
    // 渲染前切换到方块图集，渲染后恢复为实体图集，避免污染后续实体渲染。
    const bool needAtlasSwitch = (m_chunkTextureAtlas != nullptr && m_chunkTextureAtlas->isValid);
    if (needAtlasSwitch) {
        pipeline.setTextureAtlas(m_chunkTextureAtlas->imageView, m_chunkTextureAtlas->sampler);
    }

    pipeline.drawMesh(cmd, *mesh, modelMatrix, position, 1.0, overlayColor, 0.0f, 0.0f);

    // 恢复实体纹理图集
    if (needAtlasSwitch && m_entityTextureAtlas != nullptr) {
        pipeline.setTextureAtlas(m_entityTextureAtlas->imageView(), m_entityTextureAtlas->sampler());
    }

    (void)context;
    (void)cmd;
}

pipeline::EntityMesh* FallingBlockRenderer::_getOrCreateBlockMesh(
    pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState)
{
    // 按 BlockState 指针缓存网格（项目中方块状态指针来自 BlockRegistry，是稳定的）
    const auto it = m_blockMeshCache.find(&blockState);
    if (it != m_blockMeshCache.end() && it->second && it->second->indexCount > 0) {
        return it->second.get();
    }

    // 构建方块网格（委托给公共工具 util::BlockMeshBuilder）
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    util::BlockMeshBuilder::buildBlockMesh(blockState, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("FallingBlockRenderer: Failed to create block mesh for blockState {}",
            static_cast<const void*>(&blockState));
        return nullptr;
    }

    auto meshPtr = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    pipeline::EntityMesh* rawPtr = meshPtr.get();
    m_blockMeshCache[&blockState] = std::move(meshPtr);
    return rawPtr;
}

// ==================== ItemFrameRenderer ====================

ItemFrameRenderer::ItemFrameRenderer()
{
    m_shadowSize = 0.0f;
}

void ItemFrameRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现物品展示框渲染
    // 1. 渲染边框
    // 2. 渲染物品（如果有的话）
    // 3. 渲染地图（如果是地图）

    (void)entity;
    (void)partialTicks;
}

// ==================== PaintingRenderer ====================

PaintingRenderer::PaintingRenderer()
{
    m_shadowSize = 0.0f;
}

void PaintingRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现画渲染
    // 1. 根据画类型选择纹理
    // 2. 渲染画布

    (void)entity;
    (void)partialTicks;
}

// ==================== LeashKnotRenderer ====================

LeashKnotRenderer::LeashKnotRenderer()
{
    m_shadowSize = 0.0f;
}

void LeashKnotRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现拴绳结渲染

    (void)entity;
    (void)partialTicks;
}

// ==================== ArmorStandRenderer ====================

ArmorStandRenderer::ArmorStandRenderer()
{
    m_shadowSize = 0.0f;
}

void ArmorStandRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现盔甲架渲染
    // 1. 渲染盔甲架基座
    // 2. 渲染装备的盔甲
    // 3. 渲染手持物品
    // 4. 处理破坏动画

    (void)entity;
    (void)partialTicks;
}

// ==================== TNTRenderer ====================

TNTRenderer::TNTRenderer()
{
    m_shadowSize = 0.5f;
}

void TNTRenderer::render(Entity& entity, f64 partialTicks)
{
    // CPU 路径 - 已废弃，使用 GPU 管线路径
    (void)entity;
    (void)partialTicks;
}

void TNTRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 从 ClientEntity 读取 TNT 方块状态
    // 对应 MC 1.21.11 TntRenderer.extractRenderState 中 blockState = entity.getBlockState()
    const ::mc::BlockState* blockState = entity.tntBlockState();
    if (blockState == nullptr) {
        // 未设置方块状态，不渲染
        return;
    }

    // 获取或创建方块网格
    pipeline::EntityMesh* mesh = _getOrCreateBlockMesh(pipeline, *blockState);
    if (mesh == nullptr || mesh->indexCount == 0) {
        return;
    }

    // ---- 读取引信并计算闪烁 ----
    // 对应 MC 1.21.11 TntRenderer.extractRenderState:
    //   fuseRemainingInTicks = entity.getFuse() - partialTicks + 1.0F
    const f32 fuseRemaining = static_cast<f32>(entity.tntFuse()) - static_cast<f32>(context.partialTicks) + 1.0f;

    // ---- 复刻 MC 1.21.11 TntRenderer.submit() 的变换链 ----
    //   translate(0, 0.5, 0)              // 抬高半个方块
    //   [scale(flashScale)]               // 引信 < 10 时闪烁缩放
    //   rotateY(-90°)
    //   translate(-0.5, -0.5, 0.5)
    //   rotateY(90°)
    //
    // 方块网格顶点已在 BlockMeshBuilder 中乘以 1/16 转换为世界单位（0-1 范围）。
    //
    // MC 的 PoseStack 使用右乘：current = current * newTransform
    // 因此最终矩阵 = T1 * [S] * R1 * T2 * R2，顶点 v 变换为 T1 * [S] * R1 * T2 * R2 * v
    // 最右侧的 R2 最先作用于顶点。
    //
    // 行主序矩阵乘法 helper：result = a * b（即 a 右乘 b）
    auto matMul = [](const std::array<f64, 16>& a, const std::array<f64, 16>& b) -> std::array<f64, 16> {
        std::array<f64, 16> result{};
        for (i32 row = 0; row < 4; ++row) {
            for (i32 col = 0; col < 4; ++col) {
                const auto idx = static_cast<std::size_t>(row * 4 + col);
                result[idx] = 0.0;
                for (i32 k = 0; k < 4; ++k) {
                    result[idx] += a[static_cast<std::size_t>(row * 4 + k)] * b[static_cast<std::size_t>(k * 4 + col)];
                }
            }
        }
        return result;
    };

    // translate(0, 0.5, 0)
    std::array<f64, 16> modelMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 闪烁缩放：fuseRemaining < 10 时，scale = 1 + (1 - fuse/10)^4 * 0.3
    // 对齐 MC TntRenderer.submit / TntMinecartRenderer.submitMinecartContents
    if (fuseRemaining >= 0.0f && fuseRemaining < 10.0f) {
        f32 f = 1.0f - fuseRemaining / 10.0f;
        f = mc::math::clamp(f, 0.0f, 1.0f);
        f *= f;
        f *= f; // 4 次方
        const f32 flashScale = 1.0f + f * 0.3f;
        const f64 fs = static_cast<f64>(flashScale);
        std::array<f64, 16> scaleMatrix = {fs, 0.0, 0.0, 0.0, 0.0, fs, 0.0, 0.0, 0.0, 0.0, fs, 0.0, 0.0, 0.0, 0.0, 1.0};
        modelMatrix = matMul(modelMatrix, scaleMatrix); // current = current * S
    }

    // rotateY(-90°)：cos(-90°)=0, sin(-90°)=-1
    {
        const f64 c = 0.0;
        const f64 s = -1.0;
        std::array<f64, 16> rotMatrix = {c, 0.0, s, 0.0, 0.0, 1.0, 0.0, 0.0, -s, 0.0, c, 0.0, 0.0, 0.0, 0.0, 1.0};
        modelMatrix = matMul(modelMatrix, rotMatrix); // current = current * R1
    }

    // translate(-0.5, -0.5, 0.5)
    {
        std::array<f64, 16> transMatrix = {
            1.0, 0.0, 0.0, -0.5, 0.0, 1.0, 0.0, -0.5, 0.0, 0.0, 1.0, 0.5, 0.0, 0.0, 0.0, 1.0};
        modelMatrix = matMul(modelMatrix, transMatrix); // current = current * T2
    }

    // rotateY(90°)：cos(90°)=0, sin(90°)=1
    {
        const f64 c = 0.0;
        const f64 s = 1.0;
        std::array<f64, 16> rotMatrix = {c, 0.0, s, 0.0, 0.0, 1.0, 0.0, 0.0, -s, 0.0, c, 0.0, 0.0, 0.0, 0.0, 1.0};
        modelMatrix = matMul(modelMatrix, rotMatrix); // current = current * R2
    }

    // 实体位置（插值）
    Vector3f position(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 获取方块默认着色颜色
    const u32 tintColor = ChunkMesher::getDefaultBlockTintColor(blockState);
    const f32 r = static_cast<f32>(tintColor & 0xFFu) / 255.0f;
    const f32 g = static_cast<f32>((tintColor >> 8) & 0xFFu) / 255.0f;
    const f32 b = static_cast<f32>((tintColor >> 16) & 0xFFu) / 255.0f;
    const f32 a = static_cast<f32>((tintColor >> 24) & 0xFFu) / 255.0f;
    Vector4f overlayColor(r, g, b, a);

    // ---- 切换到方块纹理图集 ----
    const bool needAtlasSwitch = (m_chunkTextureAtlas != nullptr && m_chunkTextureAtlas->isValid);
    if (needAtlasSwitch) {
        pipeline.setTextureAtlas(m_chunkTextureAtlas->imageView, m_chunkTextureAtlas->sampler);
    }

    // hurtTime 用于白色闪烁（对齐 MC TntMinecartRenderer 的 OverlayTexture.pack(10)）
    // 当 (fuse/5) % 2 == 0 时，设置 hurtTime = 1.0 实现白色闪烁叠加
    f32 hurtTime = 0.0f;
    if (fuseRemaining > -1.0f) {
        const i32 fuseInt = static_cast<i32>(fuseRemaining);
        if (fuseInt / 5 % 2 == 0) {
            hurtTime = 1.0f;
        }
    }

    pipeline.drawMesh(cmd, *mesh, modelMatrix, position, 1.0, overlayColor, hurtTime, 0.0f);

    // 恢复实体纹理图集
    if (needAtlasSwitch && m_entityTextureAtlas != nullptr) {
        pipeline.setTextureAtlas(m_entityTextureAtlas->imageView(), m_entityTextureAtlas->sampler());
    }

    (void)cmd;
}

pipeline::EntityMesh* TNTRenderer::_getOrCreateBlockMesh(
    pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState)
{
    // 按 BlockState 指针缓存网格
    const auto it = m_blockMeshCache.find(&blockState);
    if (it != m_blockMeshCache.end() && it->second && it->second->indexCount > 0) {
        return it->second.get();
    }

    // 构建方块网格（委托给公共工具 util::BlockMeshBuilder）
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    util::BlockMeshBuilder::buildBlockMesh(blockState, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn(
            "TNTRenderer: Failed to create block mesh for blockState {}", static_cast<const void*>(&blockState));
        return nullptr;
    }

    auto meshPtr = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    pipeline::EntityMesh* rawPtr = meshPtr.get();
    m_blockMeshCache[&blockState] = std::move(meshPtr);
    return rawPtr;
}

// ==================== FireworkRocketRenderer ====================

FireworkRocketRenderer::FireworkRocketRenderer()
{
    m_shadowSize = 0.0f;
}

void FireworkRocketRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现烟花火箭渲染
    // 1. 渲染火箭模型
    // 2. 处理粒子尾迹

    (void)entity;
    (void)partialTicks;
}

} // namespace mc::client::renderer::entity::renderer::special
