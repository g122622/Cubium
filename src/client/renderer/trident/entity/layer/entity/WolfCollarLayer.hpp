#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/animal/WolfModel.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
class WolfEntity;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 狼项圈层渲染器
 *
 * 渲染驯服狼的项圈。
 *
 * 参考 MC 1.16.5 WolfCollarLayer
 */
class WolfCollarLayer : public core::LayerRenderer<::mc::WolfEntity> {
public:
    WolfCollarLayer() = default;
    ~WolfCollarLayer() override = default;

    void render(
        ::mc::WolfEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    [[nodiscard]] bool shouldRender(const ::mc::WolfEntity& entity) const override;

private:
    /**
     * @brief 获取项圈颜色
     * @param entity 狼实体
     * @return RGB颜色值
     */
    [[nodiscard]] static Vector3f getCollarColor(const ::mc::WolfEntity& entity);
};

} // namespace mc::client::renderer::entity::layer::entity
