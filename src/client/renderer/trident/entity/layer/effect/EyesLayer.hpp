#pragma once

#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::layer::effect {

/**
 * @brief 发光眼睛层渲染器基类
 *
 * 渲染实体的发光眼睛（如末影人、蜘蛛、幻翼等）。
 *
 * 参考 MC 1.16.5 AbstractEyesLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class EyesLayer : public core::LayerRenderer<TEntity> {
public:
    EyesLayer() = default;
    ~EyesLayer() override = default;

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
     * @brief 获取眼睛发光纹理
     * 子类可以重写此方法以提供特定纹理
     */
    [[nodiscard]] virtual ResourceLocation getEyesTexture(const TEntity& entity) const {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/eyes.png");
    }

    /**
     * @brief 获取发光颜色
     */
    [[nodiscard]] virtual Vector3f getEyesColor(const TEntity& entity) const {
        (void)entity;
        return Vector3f(1.0f, 1.0f, 1.0f);
    }
};

} // namespace mc::client::renderer::entity::layer::effect
