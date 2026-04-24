#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"

namespace mc {
class Player;
}

namespace mc::client::renderer::entity::layer::cosmetic {

/**
 * @brief 斗篷层渲染器
 *
 * 渲染玩家的斗篷。
 *
 * 参考 MC 1.16.5 CapeLayer
 */
class CapeLayer : public core::LayerRenderer<::mc::Player> {
public:
    CapeLayer() = default;
    ~CapeLayer() override = default;

    void render(
        ::mc::Player& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    [[nodiscard]] bool shouldRender(const ::mc::Player& entity) const override;

private:
    /**
     * @brief 计算斗篷摆动
     * @param entity 玩家实体
     * @param partialTicks 部分 tick
     * @return 斗篷旋转角度
     */
    [[nodiscard]] f32 calculateCapeSwing(::mc::Player& entity, f32 partialTicks) const;
};

} // namespace mc::client::renderer::entity::layer::cosmetic
