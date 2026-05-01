#pragma once

#include "../../core/EntityRenderer.hpp"
#include "../../model/projectile/ProjectileModels.hpp"
#include <memory>

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
 */
class ShulkerBulletRenderer : public core::EntityRenderer {
public:
    ShulkerBulletRenderer();
    ~ShulkerBulletRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

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
 */
class WitherSkullRenderer : public core::EntityRenderer {
public:
    WitherSkullRenderer();
    ~WitherSkullRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::WitherSkullModel> m_model;
};

/**
 * @brief 龙火球渲染器
 */
class DragonFireballRenderer : public core::EntityRenderer {
public:
    DragonFireballRenderer();
    ~DragonFireballRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

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
 * 参考 MC 1.16.5 LightningBoltRenderer
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
    void renderLayersPipelineClient(
        ::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

private:
    /**
     * @brief 渲染一个闪电四边形条带段
     *
     * 参考 MC 1.16.5 LightningBoltRenderer.func_229116_a_
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
    static void renderQuadSegment(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        f32 x1, f32 z1, i32 segmentY,
        f32 prevX, f32 prevZ,
        f32 r, f32 g, f32 b,
        f32 topWidth, f32 bottomWidth,
        bool flipX1, bool flipZ1,
        bool flipX2, bool flipZ2);

    /**
     * @brief 生成闪电网格
     *
     * @param boltVertex 随机种子
     * @param vertices 顶点输出数组
     * @param indices 索引输出数组
     */
    static void generateLightningMesh(
        u64 boltVertex,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);
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
 */
class FallingBlockRenderer : public core::EntityRenderer {
public:
    FallingBlockRenderer();
    ~FallingBlockRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
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
 */
class TNTRenderer : public core::EntityRenderer {
public:
    TNTRenderer();
    ~TNTRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
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

void registerSpecialEntityRenderers(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::special
