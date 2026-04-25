#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc {
class LivingEntity;
class SheepEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 羊毛层渲染器
 *
 * 渲染羊身上的羊毛。支持染色羊毛的颜色。
 *
 * 参考 MC 1.16.5 SheepWoolLayer
 *
 * @tparam TEntity 实体类型（需要支持 hasWool() 和 getWoolColor() 方法）
 */
template<typename TEntity = ::mc::LivingEntity>
class SheepWoolLayer : public core::LayerRenderer<TEntity> {
public:
    SheepWoolLayer() = default;
    ~SheepWoolLayer() override = default;

    /**
     * @brief 渲染羊毛层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染羊毛层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染羊毛
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

private:
    /**
     * @brief 获取羊毛颜色
     * @param entity 羊实体
     * @return RGB 颜色值
     */
    [[nodiscard]] static Vector3f getWoolColor(const TEntity& entity);

    /**
     * @brief 检查实体是否有羊毛
     */
    [[nodiscard]] static bool checkHasWool(const TEntity& entity);

    /**
     * @brief 构建羊毛网格
     */
    void buildWoolMesh(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    /**
     * @brief 获取或创建羊毛网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateWoolMesh(pipeline::EntityPipeline& pipeline);

    // 羊毛网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_woolMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
