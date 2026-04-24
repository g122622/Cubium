#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"

namespace mc {
class LivingEntity;
class SheepEntity;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 羊毛层渲染器
 *
 * 渲染羊身上的羊毛。
 *
 * 参考 MC 1.16.5 SheepWoolLayer
 *
 * @tparam TEntity 实体类型（需要支持 hasWool() 和 getWoolColor() 方法）
 */
template<typename TEntity = ::mc::LivingEntity>
class SheepWoolLayer : public core::LayerRenderer<TEntity> {
public:
    SheepWoolLayer() = default;
    ~SheepWoolLayer() override = default;

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
     * @brief 获取羊毛颜色
     * @param entity 羊实体
     * @return RGB 颜色值
     */
    [[nodiscard]] static Vector3f getWoolColor(const TEntity& entity);

    /**
     * @brief 检查实体是否有羊毛
     */
    [[nodiscard]] static bool checkHasWool(const TEntity& entity);
};

} // namespace mc::client::renderer::entity::layer::entity
