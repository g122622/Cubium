#pragma once

#include "../core/LayerRenderer.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "common/core/Types.hpp"

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::layer::cosmetic {

/**
 * @brief 鞘翅层渲染器
 *
 * 渲染玩家装备的鞘翅。
 *
 * 参考 MC 1.16.5 ElytraLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class ElytraLayer : public core::LayerRenderer<TEntity> {
public:
    ElytraLayer() = default;
    ~ElytraLayer() override = default;

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

    /**
     * @brief 设置自定义鞘翅纹理
     * @param region 纹理区域（可为 nullptr）
     */
    void setElytraTexture(const TextureRegion* region);

    /**
     * @brief 设置披风纹理（当没有鞘翅纹理时作为备选）
     * @param region 纹理区域（可为 nullptr）
     */
    void setCapeTexture(const TextureRegion* region);

    /**
     * @brief 获取当前鞘翅纹理
     */
    [[nodiscard]] const TextureRegion* getElytraTexture() const { return m_customElytraRegion; }

    /**
     * @brief 获取披风纹理
     */
    [[nodiscard]] const TextureRegion* getCapeTexture() const { return m_capeRegion; }

private:
    /**
     * @brief 计算鞘翅展开角度
     */
    [[nodiscard]] f32 calculateElytraAngle(TEntity& entity, f32 partialTicks) const;

    const TextureRegion* m_customElytraRegion = nullptr;
    const TextureRegion* m_capeRegion = nullptr;
};

} // namespace mc::client::renderer::entity::layer::cosmetic
