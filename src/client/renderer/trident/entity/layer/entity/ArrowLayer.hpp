#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 箭矢附着层渲染器
 *
 * 渲染生物身上插着的箭矢。
 *
 * 参考 MC 1.16.5 ArrowLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class ArrowLayer : public core::LayerRenderer<TEntity> {
public:
    ArrowLayer() = default;
    ~ArrowLayer() override = default;

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
    /**
     * @brief 渲染单个箭矢
     */
    void renderArrow(f32 x, f32 y, f32 z, f32 yaw, f32 pitch, f32 scale);

    ResourceLocation m_arrowTexture{"minecraft", "textures/entity/arrow.png"};
};

} // namespace mc::client::renderer::entity::layer::entity
