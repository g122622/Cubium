#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/EntityModel.hpp"
#include "../../core/IEntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace mc {
class ItemStack;
class LivingEntity;
class Player;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::equipment {

/**
 * @brief 头部物品层渲染器
 *
 * 渲染实体头部装备的物品（如南瓜、玩家头颅、头盔等）。
 *
 * 参考 MC 1.16.5 HeadLayer
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template<typename TEntity, typename TModel = model::EntityModel>
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
        : m_renderer(&renderer) {}

    ~HeadLayer() override = default;

    /**
     * @brief 渲染头部物品层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染头部物品层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染头部物品层
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 渲染头部物品（GPU管线路径）
     */
    virtual void renderHeadItemPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    );

    /**
     * @brief 渲染头部物品（CPU路径 - 已废弃）
     */
    virtual void renderHeadItem(
        TEntity& entity,
        const ItemStack& itemStack,
        f32 headYaw,
        f32 headPitch,
        f32 scale
    );

    /**
     * @brief 获取头部装备物品
     */
    [[nodiscard]] virtual const ItemStack* getHeadItem(const TEntity& entity) const;

    /**
     * @brief 计算头部物品变换矩阵
     */
    virtual void computeHeadTransform(
        f32 headYaw,
        f32 headPitch,
        std::array<f64, 16>& outMatrix
    );

    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] entity::core::IEntityRenderer<TEntity, TModel>* getRenderer() {
        return m_renderer;
    }

    /**
     * @brief 获取父模型
     */
    [[nodiscard]] TModel* getParentModel() {
        return m_renderer ? &m_renderer->getModel() : nullptr;
    }

private:
    entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;

    // 头部物品网格缓存
    static std::unordered_map<u32, pipeline::EntityMesh> s_headItemMeshCache;
};

} // namespace mc::client::renderer::entity::layer::equipment
