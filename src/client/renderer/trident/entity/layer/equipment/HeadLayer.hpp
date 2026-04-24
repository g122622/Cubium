#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {
class ItemStack;
class LivingEntity;
}

namespace mc::client::renderer::entity::layer::equipment {

/**
 * @brief 头部物品层渲染器
 *
 * 渲染实体头部装备的物品（如南瓜、玩家头颅等）。
 *
 * 参考 MC 1.16.5 HeadLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class HeadLayer : public core::LayerRenderer<TEntity> {
public:
    HeadLayer() = default;
    ~HeadLayer() override = default;

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

    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 渲染头部物品
     * @param entity 实体
     * @param itemStack 物品堆
     * @param headYaw 头部偏航角
     * @param headPitch 头部俯仰角
     * @param scale 缩放因子
     */
    virtual void renderHeadItem(
        TEntity& entity,
        const ItemStack& itemStack,
        f32 headYaw,
        f32 headPitch,
        f32 scale
    );
};

} // namespace mc::client::renderer::entity::layer::equipment
