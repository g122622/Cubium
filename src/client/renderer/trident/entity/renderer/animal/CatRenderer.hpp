#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/animal/CatModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <array>
#include <memory>

namespace mc {
class CatEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 猫渲染器
 *
 * 参考 MC 1.16.5 CatRenderer
 * 支持 11 种猫皮肤（类型）。
 */
class CatRenderer : public core::EntityRenderer {
public:
    CatRenderer();
    ~CatRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取猫纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::CatEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::CatEntity& entity) const;

private:
    model::animal::CatModel m_model;
    model::animal::CatModel m_modelBaby;

    /**
     * @brief 获取猫类型对应的纹理
     * @param catType 猫类型 (0-10)
     * @return 纹理位置
     */
    [[nodiscard]] static ResourceLocation getCatTexture(u32 catType);
};

/**
 * @brief 注册猫渲染器
 */
void registerCatRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
