#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/animal/VillagerModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class VillagerEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 村民渲染器
 *
 * 参考 MC 1.16.5 VillagerRenderer
 * 支持不同职业村民的纹理。
 */
class VillagerRenderer : public core::EntityRenderer {
public:
    VillagerRenderer();
    ~VillagerRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取村民纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::VillagerEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::VillagerEntity& entity) const;

private:
    model::animal::VillagerModel m_model;
};

/**
 * @brief 注册村民渲染器
 */
void registerVillagerRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
