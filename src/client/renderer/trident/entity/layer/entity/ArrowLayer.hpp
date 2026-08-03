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
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
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
 * @brief 箭矢附着层渲染器
 *
 * 渲染生物身上插着的箭矢。
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class ArrowLayer : public core::LayerRenderer<TEntity> {
public:
    ArrowLayer() = default;
    ~ArrowLayer() override = default;

    /**
     * @brief 渲染箭矢层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染箭矢
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

private:
    /**
     * @brief 构建单个箭矢网格
     */
    void _buildArrowMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 渲染单个箭矢（GPU管线路径）
     */
    void _renderArrowPipeline(TEntity& entity,
        f32 x,
        f32 y,
        f32 z,
        f32 yaw,
        f32 pitch,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 获取或创建箭矢网格
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateArrowMesh(pipeline::EntityPipeline& pipeline);

    ResourceLocation m_arrowTexture{"minecraft", "textures/entity/arrow.png"};

    // 箭矢网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_arrowMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
