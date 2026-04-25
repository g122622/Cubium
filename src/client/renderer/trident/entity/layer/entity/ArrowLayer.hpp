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
 * @brief 箭矢附着层渲染器
 *
 * 渲染生物身上插着的箭矢。
 *
 * 参考 MC 1.16.5 ArrowLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class ArrowLayer : public core::LayerRenderer<TEntity> {
public:
    ArrowLayer() = default;
    ~ArrowLayer() override = default;

    /**
     * @brief 渲染箭矢层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染箭矢层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染箭矢
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

private:
    /**
     * @brief 构建单个箭矢网格
     */
    void buildArrowMesh(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    /**
     * @brief 渲染单个箭矢（GPU管线路径）
     */
    void renderArrowPipeline(
        TEntity& entity,
        f32 x, f32 y, f32 z,
        f32 yaw, f32 pitch,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    );

    /**
     * @brief 获取或创建箭矢网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateArrowMesh(pipeline::EntityPipeline& pipeline);

    ResourceLocation m_arrowTexture{"minecraft", "textures/entity/arrow.png"};

    // 箭矢网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_arrowMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
