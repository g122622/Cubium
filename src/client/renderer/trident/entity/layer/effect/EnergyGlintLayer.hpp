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

namespace mc::client::renderer::entity::layer::effect {

/**
 * @brief 附魔光效层渲染器
 *
 * 渲染附魔物品的紫色光效。使用滚动的光效纹理。
 *
 * 参考 MC 1.16.5 EnergyLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class EnergyGlintLayer : public core::LayerRenderer<TEntity> {
public:
    EnergyGlintLayer() = default;
    ~EnergyGlintLayer() override = default;

    /**
     * @brief 渲染附魔光效层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染附魔光效层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染附魔光效
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 计算光效滚动偏移
     */
    [[nodiscard]] f32 calculateGlintOffset(f32 ageInTicks) const;

    /**
     * @brief 构建光效网格
     */
    void buildGlintMesh(
        f32 glintOffset,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    ResourceLocation m_glintTexture{"minecraft", "textures/misc/enchanted_item_glint.png"};

    // 光效网格缓存
    std::unordered_map<i32, pipeline::EntityMesh> m_glintMeshCache;
};

} // namespace mc::client::renderer::entity::layer::effect
