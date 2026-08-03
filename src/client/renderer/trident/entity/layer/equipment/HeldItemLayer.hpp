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
#include "common/core/Types.hpp"
#include <array>
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class ItemStack;

namespace client::renderer::entity::core {
template <typename TEntity, typename TModel>
class IEntityRenderer;
}

namespace client::renderer::entity::pipeline {
class EntityPipeline;
}

namespace client::renderer::entity::layer::equipment {

/**
 * @brief 手持物品层渲染器
 *
 * 渲染实体手中持有的物品。
 * 支持主手和副手的物品渲染。
 * 物品会跟随手臂动画正确变换。
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型（需要支持 translateHand 方法），默认为 BipedModel
 */
template <typename TEntity, typename TModel = model::BipedModel>
class HeldItemLayer : public core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 默认构造函数
     */
    HeldItemLayer() = default;

    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器，用于获取模型
     */
    explicit HeldItemLayer(mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer)
        : m_renderer(&renderer)
    {}

    ~HeldItemLayer() override = default;

    /**
     * @brief 渲染手持物品层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染手持物品层
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 渲染特定手的物品（GPU管线路径）
     * @param entity 实体
     * @param hand 手槽（主手或副手）
     * @param handSide 手侧（左手或右手，用于变换）
     * @param cmd 命令缓冲区
     * @param context 动画上下文
     * @param pipeline 渲染管线
     */
    virtual void renderHandItemPipeline(TEntity& entity,
        mc::Hand hand,
        mc::HandSide handSide,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 获取手持物品
     * @param entity 实体
     * @param hand 手槽（主手或副手）
     * @return 物品堆指针，如果无物品返回 nullptr
     */
    [[nodiscard]] virtual const ItemStack* getHeldItem(const TEntity& entity, mc::Hand hand) const;

    /**
     * @brief 计算手持物品变换矩阵
     *
     * 物品变换由以下步骤组成：
     * 1. 调用 model.translateHand() 获取手臂的变换矩阵
     * 2. 应用 X 轴 -90° 旋转
     * 3. 应用 Y 轴 180° 旋转
     * 4. 应用手部偏移
     *
     * @param model 模型（用于获取手臂变换）
     * @param handSide 手侧（左手或右手）
     * @param outMatrix 输出变换矩阵
     */
    virtual void computeItemTransform(const TModel* model, mc::HandSide handSide, std::array<f64, 16>& outMatrix);

    /**
     * @brief 静态方法：计算手持物品变换矩阵（供测试使用）
     *
     * 物品变换由以下步骤组成：
     * 1. 调用 model.translateHand() 获取手臂的变换矩阵
     * 2. 应用 X 轴 -90° 旋转
     * 3. 应用 Y 轴 180° 旋转
     * 4. 应用手部偏移
     *
     * @param model 模型（用于获取手臂变换，可以为空）
     * @param handSide 手侧（左手或右手）
     * @param outMatrix 输出变换矩阵
     */
    static void computeItemTransformStatic(const TModel* model, mc::HandSide handSide, std::array<f64, 16>& outMatrix);

    /**
     * @brief 获取关联的模型
     */
    [[nodiscard]] TModel* getParentModel() { return m_renderer ? &m_renderer->getModel() : nullptr; }

private:
    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
};

} // namespace client::renderer::entity::layer::equipment
} // namespace mc
