#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/animal/WolfModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class WolfEntity;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 狼渲染器
 *
 * 参考 MC 1.16.5 WolfRenderer
 */
class WolfRenderer : public core::EntityRenderer {
public:
    WolfRenderer();
    ~WolfRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取狼纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::WolfEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::WolfEntity& entity) const;

private:
    void setupLayers();

    model::animal::WolfModel m_model;
    model::animal::WolfModel m_modelBaby;
};

/**
 * @brief 注册狼渲染器
 */
void registerWolfRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
