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
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class ItemStack;
class LivingEntity;
class Player;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::equipment {

/**
 * @brief 头部物品层渲染器
 *
 * 渲染实体头部装备的物品（如南瓜、玩家头颅、头盔等）。
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template <typename TEntity, typename TModel = model::EntityModel>
class HeadLayer : public core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 默认构造函数
     */
    HeadLayer() = default;

    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     */
    explicit HeadLayer(entity::core::IEntityRenderer<TEntity, TModel>& renderer)
        : m_renderer(&renderer)
    {}

    ~HeadLayer() override = default;

    /**
     * @brief 渲染头部物品层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染头部物品层
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 渲染头部物品（GPU管线路径）
     */
    virtual void renderHeadItemPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 获取头部装备物品
     */
    [[nodiscard]] virtual const ItemStack* getHeadItem(const TEntity& entity) const;

    /**
     * @brief 计算头部物品变换矩阵
     */
    virtual void computeHeadTransform(f32 headYaw, f32 headPitch, std::array<f64, 16>& outMatrix);

    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] entity::core::IEntityRenderer<TEntity, TModel>* getRenderer() { return m_renderer; }

    /**
     * @brief 获取父模型
     */
    [[nodiscard]] TModel* getParentModel() { return m_renderer ? &m_renderer->getModel() : nullptr; }

private:
    entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;

    // 头部物品网格缓存
    static std::unordered_map<u32, pipeline::EntityMesh> s_headItemMeshCache;
};

} // namespace mc::client::renderer::entity::layer::equipment
