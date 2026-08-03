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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/IEntityRenderer.hpp"
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 鞍层渲染器
 *
 * 渲染可骑乘实体上的鞍（如马、猪等）。
 * 使用父模型的动画状态来驱动鞍模型。
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template <typename TEntity, typename TModel>
class SaddleLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     * @param saddleModel 鞍模型（可选，如果为 nullptr 将使用内置模型）
     */
    explicit SaddleLayer(mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer,
        std::shared_ptr<TModel> saddleModel = nullptr)
        : m_renderer(&renderer)
        , m_saddleModel(std::move(saddleModel))
    {}

    ~SaddleLayer() override = default;

    /**
     * @brief 渲染鞍层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染鞍
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

    /**
     * @brief 设置鞍纹理
     */
    void setSaddleTexture(const ResourceLocation& texture) { m_saddleTexture = texture; }

protected:
    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* getRenderer()
    {
        return m_renderer;
    }

    /**
     * @brief 获取父模型
     */
    [[nodiscard]] TModel* getParentModel() { return &m_renderer->getModel(); }

    /**
     * @brief 获取鞍模型
     */
    [[nodiscard]] TModel* getSaddleModel() { return m_saddleModel.get(); }

private:
    /**
     * @brief 构建鞍网格
     */
    void _buildSaddleMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建鞍网格
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateSaddleMesh(pipeline::EntityPipeline& pipeline);

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer;
    std::shared_ptr<TModel> m_saddleModel;

    ResourceLocation m_saddleTexture{"minecraft", "textures/entity/saddle.png"}; // TODO: 鞍纹理尚未在渲染中使用

    // 鞍网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_saddleMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
