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

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/projectile/ProjectileModels.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
class BlockState;
} // namespace mc

namespace mc::client {
struct ChunkTextureAtlas;
} // namespace mc::client

namespace mc::client::renderer::entity::pipeline {
class EntityTextureAtlas;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::special {

/**
 * @brief 末影水晶渲染器
 */
class EnderCrystalRenderer : public core::EntityRenderer {
public:
    EnderCrystalRenderer();
    ~EnderCrystalRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::EnderCrystalModel> m_model;
};

/**
 * @brief 潜影贝子弹渲染器
 *
 * MC Java 中 ShulkerBulletRenderer.getBlockLightLevel() 返回 15，
 * 潜影贝子弹在黑暗中也会发光，使用全亮光照。
 */
class ShulkerBulletRenderer : public core::EntityRenderer {
public:
    ShulkerBulletRenderer();
    ~ShulkerBulletRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] bool isFullbright() const override { return true; }

private:
    std::shared_ptr<model::projectile::ShulkerBulletModel> m_model;
};

/**
 * @brief 羊驼唾沫渲染器
 */
class LlamaSpitRenderer : public core::EntityRenderer {
public:
    LlamaSpitRenderer();
    ~LlamaSpitRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::LlamaSpitModel> m_model;
};

/**
 * @brief 光灵箭渲染器
 */
class SpectralArrowRenderer : public core::EntityRenderer {
public:
    SpectralArrowRenderer();
    ~SpectralArrowRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::SpectralArrowModel> m_model;
};

/**
 * @brief 凋灵之首渲染器
 *
 * MC Java 中 WitherSkullRenderer.getBlockLightLevel() 返回 15，
 * 凋灵之首在黑暗中也会发光，使用全亮光照。
 */
class WitherSkullRenderer : public core::EntityRenderer {
public:
    WitherSkullRenderer();
    ~WitherSkullRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] bool isFullbright() const override { return true; }

private:
    std::shared_ptr<model::projectile::WitherSkullModel> m_model;
};

/**
 * @brief 龙火球渲染器
 *
 * MC Java 中 DragonFireballRenderer.getBlockLightLevel() 返回 15，
 * 龙火球在黑暗中也会发光，使用全亮光照。
 */
class DragonFireballRenderer : public core::EntityRenderer {
public:
    DragonFireballRenderer();
    ~DragonFireballRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] bool isFullbright() const override { return true; }

private:
    std::shared_ptr<model::projectile::DragonFireballModel> m_model;
};

/**
 * @brief 唤魔者尖牙渲染器
 */
class EvokerFangsRenderer : public core::EntityRenderer {
public:
    EvokerFangsRenderer();
    ~EvokerFangsRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::EvokerFangsModel> m_model;
};

/**
 * @brief 闪电渲染器
 *
 * 闪电使用程序化生成，渲染为多段四边形条带
 */
class LightningBoltRenderer : public core::EntityRenderer {
public:
    LightningBoltRenderer();
    ~LightningBoltRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染层（GPU管线路径，ClientEntity 版本）
     *
     * 闪电使用 GPU 管线渲染，程序化生成顶点
     */
    void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

private:
    /**
     * @brief 渲染一个闪电四边形条带段
     *
     * @param vertices 顶点输出数组
     * @param indices 索引输出数组
     * @param x1 当前X偏移
     * @param z1 当前Z偏移
     * @param segmentY 当前段Y坐标（段索引 * 16）
     * @param prevX 前一段X偏移
     * @param prevZ 前一段Z偏移
     * @param r 红色分量
     * @param g 绿色分量
     * @param b 蓝色分量
     * @param topWidth 顶部宽度
     * @param bottomWidth 底部宽度
     * @param flipX1 是否翻转X1
     * @param flipZ1 是否翻转Z1
     * @param flipX2 是否翻转X2
     * @param flipZ2 是否翻转Z2
     */
    static void _renderQuadSegment(std::vector<model::ModelVertex>& vertices,
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
        bool flipZ2);

    /**
     * @brief 生成闪电网格
     *
     * @param boltVertex 随机种子
     * @param vertices 顶点输出数组
     * @param indices 索引输出数组
     */
    static void _generateLightningMesh(
        u64 boltVertex, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);
};

/**
 * @brief 区域效果云渲染器
 */
class AreaEffectCloudRenderer : public core::EntityRenderer {
public:
    AreaEffectCloudRenderer();
    ~AreaEffectCloudRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 下落方块渲染器
 *
 * 渲染下落中的方块（沙子、砾石、铁砧等）。对应 MC 1.21.11 FallingBlockRenderer。
 *
 * 渲染方式：
 * - 在 renderLayersPipelineClient 中完成全部渲染（不使用 PipelineMeshProvider 主网格路径）
 * - 从 ClientEntity::fallingBlockState() 读取方块状态
 * - 通过 util::BlockMeshBuilder 构建方块网格（按 BlockState* 缓存）
 * - 切换到方块纹理图集（ChunkTextureAtlas）渲染，渲染后恢复实体纹理图集
 *
 * 变换链（对齐 MC 1.21.11 FallingBlockRenderer.submit）：
 *   translate(-0.5, 0, -0.5)  // 方块中心对齐实体原点
 *
 * 方块网格顶点已在 BlockMeshBuilder 中乘以 1/16 转换为世界单位（0-1 范围）。
 */
class FallingBlockRenderer : public core::EntityRenderer {
public:
    FallingBlockRenderer();
    ~FallingBlockRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染下落方块（GPU管线路径）
     *
     * 在此方法中完成全部渲染：获取方块状态、构建/缓存网格、切换纹理图集、绘制。
     */
    void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 设置方块纹理图集引用
     *
     * 由 EntityRendererManager::setChunkTextureAtlas 注入。
     * 用于在渲染时切换 EntityPipeline 的纹理图集到方块纹理图集。
     */
    void setChunkTextureAtlas(const ::mc::client::ChunkTextureAtlas* atlas) { m_chunkTextureAtlas = atlas; }

    /**
     * @brief 设置实体纹理图集引用
     *
     * 由 EntityRendererManager 注入。用于在渲染方块后恢复纹理图集。
     */
    void setEntityTextureAtlas(const pipeline::EntityTextureAtlas* atlas) { m_entityTextureAtlas = atlas; }

private:
    /**
     * @brief 获取或创建方块网格（按 BlockState* 缓存）
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateBlockMesh(
        pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState);

    const ::mc::client::ChunkTextureAtlas* m_chunkTextureAtlas = nullptr;
    const pipeline::EntityTextureAtlas* m_entityTextureAtlas = nullptr;
    std::unordered_map<const ::mc::BlockState*, std::unique_ptr<pipeline::EntityMesh>> m_blockMeshCache;
};

/**
 * @brief 物品展示框渲染器
 */
class ItemFrameRenderer : public core::EntityRenderer {
public:
    ItemFrameRenderer();
    ~ItemFrameRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 画渲染器
 */
class PaintingRenderer : public core::EntityRenderer {
public:
    PaintingRenderer();
    ~PaintingRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 栓绳结渲染器
 */
class LeashKnotRenderer : public core::EntityRenderer {
public:
    LeashKnotRenderer();
    ~LeashKnotRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 盔甲架渲染器
 */
class ArmorStandRenderer : public core::EntityRenderer {
public:
    ArmorStandRenderer();
    ~ArmorStandRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief TNT渲染器
 *
 * 渲染点燃的 TNT 实体。对应 MC 1.21.11 TntRenderer。
 *
 * 渲染方式：
 * - 在 renderLayersPipelineClient 中完成全部渲染（不使用 PipelineMeshProvider 主网格路径）
 * - 从 ClientEntity::tntBlockState() 读取方块状态（默认 TNT）
 * - 从 ClientEntity::tntFuse() 读取引信剩余 tick
 * - 通过 util::BlockMeshBuilder 构建方块网格（按 BlockState* 缓存）
 * - 切换到方块纹理图集（ChunkTextureAtlas）渲染，渲染后恢复实体纹理图集
 *
 * 变换链（对齐 MC 1.21.11 TntRenderer.submit）：
 *   translate(0, 0.5, 0)       // 抬高半个方块
 *   [scale(flashScale)]        // 引信 < 10 时闪烁缩放
 *   rotateY(-90°)
 *   translate(-0.5, -0.5, 0.5)
 *   rotateY(90°)
 *
 * 闪烁效果（对齐 MC TntRenderer / TntMinecartRenderer）：
 * - fuse < 10 时，scale = 1 + (1 - fuse/10)^4 * 0.3
 * - (fuse/5) % 2 == 0 时，通过 overlayColor 传递白色闪烁
 */
class TNTRenderer : public core::EntityRenderer {
public:
    TNTRenderer();
    ~TNTRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染 TNT（GPU管线路径）
     *
     * 在此方法中完成全部渲染：获取方块状态和引信、构建/缓存网格、
     * 切换纹理图集、应用闪烁缩放和白色闪烁、绘制。
     */
    void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 设置方块纹理图集引用
     */
    void setChunkTextureAtlas(const ::mc::client::ChunkTextureAtlas* atlas) { m_chunkTextureAtlas = atlas; }

    /**
     * @brief 设置实体纹理图集引用
     */
    void setEntityTextureAtlas(const pipeline::EntityTextureAtlas* atlas) { m_entityTextureAtlas = atlas; }

private:
    /**
     * @brief 获取或创建方块网格（按 BlockState* 缓存）
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateBlockMesh(
        pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState);

    const ::mc::client::ChunkTextureAtlas* m_chunkTextureAtlas = nullptr;
    const pipeline::EntityTextureAtlas* m_entityTextureAtlas = nullptr;
    std::unordered_map<const ::mc::BlockState*, std::unique_ptr<pipeline::EntityMesh>> m_blockMeshCache;
};

/**
 * @brief 烟花火箭渲染器
 */
class FireworkRocketRenderer : public core::EntityRenderer {
public:
    FireworkRocketRenderer();
    ~FireworkRocketRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

} // namespace mc::client::renderer::entity::renderer::special
