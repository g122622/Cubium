#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class LivingEntity;
class BlockState;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 方块持有层渲染器
 *
 * 渲染末影人手持的方块。
 *
 * 参考 MC 1.16.5 HeldBlockLayer (for Enderman)
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class HeldBlockLayer : public core::LayerRenderer<TEntity> {
public:
    HeldBlockLayer();
    ~HeldBlockLayer() override = default;

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
     * @brief 渲染持有的方块
     * @param blockState 方块状态
     * @param x X偏移
     * @param y Y偏移
     * @param z Z偏移
     * @param scale 缩放因子
     */
    void renderBlock(
        const mc::BlockState& blockState,
        f32 x, f32 y, f32 z,
        f32 scale
    );
};

} // namespace mc::client::renderer::entity::layer::entity
