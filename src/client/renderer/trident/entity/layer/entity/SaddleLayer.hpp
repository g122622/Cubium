#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 鞍层渲染器
 *
 * 渲染可骑乘实体上的鞍（如马、猪等）。
 *
 * 参考 MC 1.16.5 SaddleLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class SaddleLayer : public core::LayerRenderer<TEntity> {
public:
    SaddleLayer() = default;
    ~SaddleLayer() override = default;

    /**
     * @brief 渲染鞍层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染鞍层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染鞍
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

private:
    /**
     * @brief 构建鞍网格
     */
    void buildSaddleMesh(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    /**
     * @brief 获取或创建鞍网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateSaddleMesh(pipeline::EntityPipeline& pipeline);

    ResourceLocation m_saddleTexture{"minecraft", "textures/entity/saddle.png"};

    // 鞍网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_saddleMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
