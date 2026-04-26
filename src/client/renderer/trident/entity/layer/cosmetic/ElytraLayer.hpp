#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>

namespace mc {
class LivingEntity;
struct TextureRegion;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::cosmetic {

/**
 * @brief 鞘翅层渲染器
 *
 * 渲染玩家装备的鞘翅。支持滑翔时的展开动画。
 *
 * 参考 MC 1.16.5 ElytraLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class ElytraLayer : public core::LayerRenderer<TEntity> {
public:
    ElytraLayer() = default;
    ~ElytraLayer() override = default;

    /**
     * @brief 渲染鞘翅层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染鞘翅层（CPU路径 - 已废弃）
     */
    void render(
        TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    /**
     * @brief 检查是否应该渲染鞘翅
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

    /**
     * @brief 设置自定义鞘翅纹理
     * @param region 纹理区域（可为 nullptr）
     */
    void setElytraTexture(const TextureRegion* region);

    /**
     * @brief 设置披风纹理（当没有鞘翅纹理时作为备选）
     * @param region 纹理区域（可为 nullptr）
     */
    void setCapeTexture(const TextureRegion* region);

    /**
     * @brief 获取当前鞘翅纹理
     */
    [[nodiscard]] const TextureRegion* getElytraTexture() const { return m_customElytraRegion; }

    /**
     * @brief 获取披风纹理
     */
    [[nodiscard]] const TextureRegion* getCapeTexture() const { return m_capeRegion; }

private:
    /**
     * @brief 计算鞘翅展开角度
     */
    [[nodiscard]] f32 calculateElytraAngle(TEntity& entity, const mc::client::renderer::entity::core::AnimationContext& context, f32 partialTicks) const;

    /**
     * @brief 构建鞘翅网格
     */
    void buildElytraMesh(
        f32 spreadAngle,
        bool isLeftWing,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    /**
     * @brief 获取或创建鞘翅网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateElytraMesh(
        f32 spreadAngle,
        pipeline::EntityPipeline& pipeline
    );

    const TextureRegion* m_customElytraRegion = nullptr;
    const TextureRegion* m_capeRegion = nullptr;

    // 鞘翅网格缓存（按展开角度离散化）
    std::unordered_map<i32, pipeline::EntityMesh> m_elytraMeshCache;
};

} // namespace mc::client::renderer::entity::layer::cosmetic
