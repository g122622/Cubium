#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace mc {
class ItemStack;
}

namespace mc::client::renderer::entity::layer::equipment {

/**
 * @brief 手持物品层渲染器
 *
 * 渲染实体手中持有的物品。
 * 支持主手和副手的物品渲染。
 *
 * 参考 MC 1.16.5 HeldItemLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class HeldItemLayer : public core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     */
    HeldItemLayer() = default;

    ~HeldItemLayer() override = default;

    /**
     * @brief 渲染手持物品层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染手持物品层（CPU路径 - 已废弃）
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
    virtual void renderHandItemPipeline(
        TEntity& entity,
        mc::Hand hand,
        mc::HandSide handSide,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    );

    /**
     * @brief 渲染特定手的物品（CPU路径 - 已废弃）
     */
    virtual void renderHandItem(
        TEntity& entity,
        mc::Hand hand,
        mc::HandSide handSide,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 scale
    );

    /**
     * @brief 获取手持物品
     * @param entity 实体
     * @param hand 手槽（主手或副手）
     * @return 物品堆指针，如果无物品返回 nullptr
     */
    [[nodiscard]] virtual const ItemStack* getHeldItem(
        const TEntity& entity,
        mc::Hand hand
    ) const;

    /**
     * @brief 计算手持物品变换矩阵
     * @param handSide 手侧（左手或右手）
     * @param limbSwing 步态周期
     * @param limbSwingAmount 步态速度
     * @param swingProgress 挥动进度
     * @param outMatrix 输出变换矩阵
     */
    virtual void computeItemTransform(
        mc::HandSide handSide,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 swingProgress,
        std::array<f64, 16>& outMatrix
    );
};

} // namespace mc::client::renderer::entity::layer::equipment
