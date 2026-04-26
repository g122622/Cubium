#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "../../core/IEntityRenderer.hpp"
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
 * 使用父模型的动画状态来驱动鞍模型。
 *
 * 参考 MC 1.16.5 SaddleLayer
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template<typename TEntity, typename TModel>
class SaddleLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     * @param saddleModel 鞍模型（可选，如果为 nullptr 将使用内置模型）
     */
    explicit SaddleLayer(
        mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer,
        std::shared_ptr<TModel> saddleModel = nullptr)
        : m_renderer(&renderer)
        , m_saddleModel(std::move(saddleModel)) {}

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

    /**
     * @brief 设置鞍纹理
     */
    void setSaddleTexture(const ResourceLocation& texture) {
        m_saddleTexture = texture;
    }

protected:
    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* getRenderer() {
        return m_renderer;
    }

    /**
     * @brief 获取父模型
     */
    [[nodiscard]] TModel* getParentModel() {
        return m_renderer ? &m_renderer->getModel() : nullptr;
    }

    /**
     * @brief 获取鞍模型
     */
    [[nodiscard]] TModel* getSaddleModel() {
        return m_saddleModel.get();
    }

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

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
    std::shared_ptr<TModel> m_saddleModel;

    ResourceLocation m_saddleTexture{"minecraft", "textures/entity/saddle.png"};

    // 鞍网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_saddleMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
