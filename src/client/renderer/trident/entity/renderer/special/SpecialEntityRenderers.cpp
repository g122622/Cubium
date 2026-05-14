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
#include "../../core/EntityRendererManager.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::entity::renderer::special {

// ==================== EnderCrystalRenderer ====================

EnderCrystalRenderer::EnderCrystalRenderer()
    : m_model(std::make_shared<model::projectile::EnderCrystalModel>())
{
    m_shadowSize = 0.0f;
}

void EnderCrystalRenderer::render(Entity& entity, f64 partialTicks)
{
    // 参考 MC 1.16.5 EnderCrystalRenderer.render()
    // 末影水晶渲染：
    // 1. 计算水晶旋转动画（innerRotation）
    // 2. 计算水晶浮动偏移（上下浮动）
    // 3. 渲染水晶核心和玻璃外壳
    // 4. 如果有束目标，渲染指向末影龙的光束

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
    // 参考 MC 1.16.5 ShulkerBulletRenderer.render()
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
    // 参考 MC 1.16.5 LlamaSpitRenderer.render()
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
    // 参考 MC 1.16.5 SpectralArrowRenderer.render()
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
    // 参考 MC 1.16.5 WitherSkullRenderer.render()
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
    // 参考 MC 1.16.5 DragonFireballRenderer.render()
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
    // 参考 MC 1.16.5 EvokerFangsRenderer.render()
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
    // 参考 MC 1.16.5 LightningBoltRenderer.render()
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
    generateLightningMesh(boltVertex, vertices, indices);

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
    // 参考 MC 1.16.5 RenderType.getLightning()
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

void LightningBoltRenderer::renderQuadSegment(std::vector<model::ModelVertex>& vertices,
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
    // 参考 MC 1.16.5 LightningBoltRenderer.func_229116_a_
    // 渲染一个四边形条带段
    // 顶点位置计算：
    // - 第一个顶点：(x1 ± bottomWidth, segmentY * 16, z1 ± bottomWidth)
    // - 第二个顶点：(prevX ± topWidth, (segmentY + 1) * 16, prevZ ± topWidth)
    // - 第三个顶点：(prevX ± topWidth, (segmentY + 1) * 16, prevZ ± topWidth)
    // - 第四个顶点：(x1 ± bottomWidth, segmentY * 16, z1 ± bottomWidth)

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

    // 顶点颜色 (alpha 固定为 0.3)
    constexpr f32 alpha = 0.3f;

    u32 baseIndex = static_cast<u32>(vertices.size());

    // 添加四个顶点
    // 闪电不需要纹理坐标，使用法线朝向 +Y
    vertices.push_back(model::ModelVertex(v0x, y1, v0z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(v1x, y2, v1z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(v2x, y2, v2z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(v3x, y1, v3z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));

    // 设置顶点颜色（存储在顶点中）
    // ModelVertex 没有颜色字段，颜色通过 overlayColor 传递
    // 这里我们需要使用一种方式传递颜色
    // 暂时使用固定颜色

    // 添加两个三角形
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);

    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

void LightningBoltRenderer::generateLightningMesh(
    u64 boltVertex, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 参考 MC 1.16.5 LightningBoltRenderer.render()
    // 使用确定性随机数生成器

    // 创建随机数生成器
    mc::math::Random rng(static_cast<u64>(boltVertex));

    // 生成 8 个点的偏移数组
    // MC 1.16.5: float[] afloat = new float[8]; float[] afloat1 = new float[8];
    std::array<f32, 8> offsetX;
    std::array<f32, 8> offsetZ;
    f32 f = 0.0f;
    f32 f1 = 0.0f;

    for (i32 i = 7; i >= 0; --i) {
        // MC 1.16.5: f += (float)(random.nextInt(11) - 5);
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
                // MC 1.16.5:
                // float f10 = 0.1F + (float)j * 0.2F;
                // if (k == 0) { f10 *= (float)(j1 * 0.1D + 1.0D); }
                f32 topWidth = 0.1f + static_cast<f32>(j) * 0.2f;
                if (k == 0) {
                    topWidth *= static_cast<f32>(static_cast<f64>(j1) * 0.1 + 1.0);
                }

                // float f11 = 0.1F + (float)j * 0.2F;
                // if (k == 0) { f11 *= (float)((j1 - 1) * 0.1D + 1.0D); }
                f32 bottomWidth = 0.1f + static_cast<f32>(j) * 0.2f;
                if (k == 0) {
                    bottomWidth *= static_cast<f32>((j1 - 1) * 0.1 + 1.0);
                }

                // 渲染 4 个四边形（围绕闪电）
                // MC 1.16.5 使用 Matrix4f 和 IVertexBuilder 渲染
                // 我们直接生成顶点
                renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, false, false, true, false);
                renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, true, false, true, true);
                renderQuadSegment(
                    vertices, indices, f2, f3, j1, f4, f5, r, g, b, topWidth, bottomWidth, true, true, false, true);
                renderQuadSegment(
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
    // 参考 MC 1.16.5 AreaEffectCloudRenderer.render()
    // 区域效果云渲染：
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
    // 参考 MC 1.16.5 FallingBlockRenderer.render()
    // 下落方块渲染：
    // 1. 获取方块状态
    // 2. 渲染方块模型
    // 3. 位置插值

    (void)entity;
    (void)partialTicks;
}

// ==================== ItemFrameRenderer ====================

ItemFrameRenderer::ItemFrameRenderer()
{
    m_shadowSize = 0.0f;
}

void ItemFrameRenderer::render(Entity& entity, f64 partialTicks)
{
    // 参考 MC 1.16.5 ItemFrameRenderer.render()
    // 物品展示框渲染：
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
    // 参考 MC 1.16.5 PaintingRenderer.render()
    // 画渲染：
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
    // 参考 MC 1.16.5 LeashKnotRenderer.render()
    // 拴绳结渲染：简单的模型

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
    // 参考 MC 1.16.5 ArmorStandRenderer.render()
    // 盔甲架渲染：
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
    // 参考 MC 1.16.5 TNTRenderer.render()
    // TNT渲染：
    // 1. 渲染TNT方块
    // 2. 处理点燃闪烁

    (void)entity;
    (void)partialTicks;
}

// ==================== FireworkRocketRenderer ====================

FireworkRocketRenderer::FireworkRocketRenderer()
{
    m_shadowSize = 0.0f;
}

void FireworkRocketRenderer::render(Entity& entity, f64 partialTicks)
{
    // 参考 MC 1.16.5 FireworkRocketRenderer.render()
    // 烟花火箭渲染：
    // 1. 渲染火箭模型
    // 2. 处理粒子尾迹

    (void)entity;
    (void)partialTicks;
}

// ==================== Registration ====================

void registerSpecialEntityRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:end_crystal",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<EnderCrystalRenderer>(); });

    manager.registerRenderer("minecraft:shulker_bullet",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ShulkerBulletRenderer>(); });

    manager.registerRenderer("minecraft:llama_spit",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<LlamaSpitRenderer>(); });

    manager.registerRenderer("minecraft:spectral_arrow",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SpectralArrowRenderer>(); });

    manager.registerRenderer("minecraft:wither_skull",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<WitherSkullRenderer>(); });

    manager.registerRenderer("minecraft:dragon_fireball",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<DragonFireballRenderer>(); });

    manager.registerRenderer("minecraft:evoker_fangs",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<EvokerFangsRenderer>(); });

    manager.registerRenderer("minecraft:lightning_bolt",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<LightningBoltRenderer>(); });

    manager.registerRenderer("minecraft:area_effect_cloud",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<AreaEffectCloudRenderer>(); });

    manager.registerRenderer("minecraft:falling_block",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<FallingBlockRenderer>(); });

    manager.registerRenderer("minecraft:item_frame",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ItemFrameRenderer>(); });

    manager.registerRenderer("minecraft:painting",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PaintingRenderer>(); });

    manager.registerRenderer("minecraft:leash_knot",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<LeashKnotRenderer>(); });

    manager.registerRenderer("minecraft:armor_stand",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ArmorStandRenderer>(); });

    manager.registerRenderer(
        "minecraft:tnt", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<TNTRenderer>(); });

    manager.registerRenderer("minecraft:firework_rocket",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<FireworkRocketRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::special
