#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/animal/HorseModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class HorseEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 马渲染器
 *
 * 参考 MC 1.16.5 HorseRenderer
 * 支持多种马变种（普通马、驴、骡、骷髅马、僵尸马）。
 */
class HorseRenderer : public core::EntityRenderer {
public:
    HorseRenderer();
    ~HorseRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取马纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::HorseEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::HorseEntity& entity) const;

private:
    void setupLayers();

    model::animal::HorseModel m_model;
    model::animal::HorseModel m_modelBaby;
};

/**
 * @brief 注册马渲染器
 */
void registerHorseRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
