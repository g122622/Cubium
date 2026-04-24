#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/animal/OcelotModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class OcelotEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 豹猫渲染器
 *
 * 参考 MC 1.16.5 OcelotRenderer
 */
class OcelotRenderer : public core::EntityRenderer {
public:
    OcelotRenderer();
    ~OcelotRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取豹猫纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::OcelotEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::OcelotEntity& entity) const;

private:
    model::animal::OcelotModel m_model;
    model::animal::OcelotModel m_modelBaby;
};

/**
 * @brief 注册豹猫渲染器
 */
void registerOcelotRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
