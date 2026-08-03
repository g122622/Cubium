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
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class LivingEntity;
class SheepEntity;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 羊毛层渲染器
 *
 * 渲染羊身上的羊毛。支持染色羊毛的颜色和 jeb_ 彩虹羊。
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template <typename TEntity = ::mc::LivingEntity, typename TModel = ::mc::client::renderer::entity::model::BipedModel>
class SheepWoolLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     * @param woolModel 羊毛模型（可选）
     */
    explicit SheepWoolLayer(mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer,
        std::shared_ptr<TModel> woolModel = nullptr)
        : m_renderer(&renderer)
        , m_woolModel(std::move(woolModel))
    {}

    ~SheepWoolLayer() override = default;

    /**
     * @brief 渲染羊毛层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染羊毛
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

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
    [[nodiscard]] TModel* getParentModel() { return m_renderer ? &m_renderer->getModel() : nullptr; }

    /**
     * @brief 获取羊毛模型
     */
    [[nodiscard]] TModel* getWoolModel() { return m_woolModel.get(); }

    /**
     * @brief 获取羊毛颜色
     * @param entity 羊实体
     * @param ticksExisted 实体存活时间（用于彩虹羊）
     * @return RGB 颜色值
     */
    [[nodiscard]] static Vector3f getWoolColor(const TEntity& entity, u32 ticksExisted);

    /**
     * @brief 计算 jeb_ 彩虹羊颜色
     * @param ticksExisted 实体存活时间
     * @return RGB 颜色值
     */
    [[nodiscard]] static Vector3f computeRainbowColor(u32 ticksExisted);

    /**
     * @brief 检查是否为 jeb_ 彩虹羊
     * @param entity 羊实体
     * @return 如果是彩虹羊返回 true
     */
    [[nodiscard]] static bool isRainbowSheep(const TEntity& entity);

private:
    /**
     * @brief 构建羊毛网格
     */
    void _buildWoolMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建羊毛网格
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateWoolMesh(pipeline::EntityPipeline& pipeline);

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
    std::shared_ptr<TModel> m_woolModel;

    // 羊毛网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_woolMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
