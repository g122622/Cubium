#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 鞍层渲染器
 *
 * 渲染可骑乘实体上的鞍。
 *
 * 参考 MC 1.16.5 SaddleLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class SaddleLayer : public core::LayerRenderer<TEntity> {
public:
    SaddleLayer() = default;
    ~SaddleLayer() override = default;

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
    ResourceLocation m_saddleTexture{"minecraft", "textures/entity/saddle.png"};
};

} // namespace mc::client::renderer::entity::layer::entity
