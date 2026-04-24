#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
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
     * @brief 手部类型
     */
    enum class HandSide : u8 {
        MainHand,  // 主手
        OffHand    // 副手
    };

    /**
     * @brief 构造函数
     */
    HeldItemLayer() = default;

    ~HeldItemLayer() override = default;

    /**
     * @brief 渲染手持物品层
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
     * @brief 渲染特定手的物品
     * @param entity 实体
     * @param hand 手部类型
     * @param limbSwing 步态动画周期
     * @param limbSwingAmount 步态动画强度
     * @param partialTicks 部分 tick
     * @param scale 缩放因子
     */
    virtual void renderHandItem(
        TEntity& entity,
        HandSide hand,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 scale
    );

    /**
     * @brief 获取手持物品
     * @param entity 实体
     * @param hand 手部类型
     * @return 物品堆，如果没有则返回 nullptr
     */
    [[nodiscard]] virtual const ItemStack* getHeldItem(
        const TEntity& entity,
        HandSide hand
    ) const;

    /**
     * @brief 应用物品变换
     * @param hand 手部类型
     * @param limbSwing 步态动画周期
     * @param limbSwingAmount 步态动画强度
     */
    virtual void applyItemTransform(
        HandSide hand,
        f32 limbSwing,
        f32 limbSwingAmount
    );
};

} // namespace mc::client::renderer::entity::layer::equipment
