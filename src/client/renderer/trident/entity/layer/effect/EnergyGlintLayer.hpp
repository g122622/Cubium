#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::layer::effect {

/**
 * @brief 附魔光效层渲染器
 *
 * 渲染附魔物品的紫色光效。
 *
 * 参考 MC 1.16.5 EnergyLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class EnergyGlintLayer : public core::LayerRenderer<TEntity> {
public:
    EnergyGlintLayer() = default;
    ~EnergyGlintLayer() override = default;

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

private:
    ResourceLocation m_glintTexture{"minecraft", "textures/misc/enchanted_item_glint.png"};

    /**
     * @brief 计算光效滚动偏移
     */
    [[nodiscard]] f32 calculateGlintOffset(f32 ageInTicks) const;
};

} // namespace mc::client::renderer::entity::layer::effect
