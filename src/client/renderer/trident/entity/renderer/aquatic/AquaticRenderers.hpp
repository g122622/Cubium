#pragma once

#include "../../core/LivingRenderer.hpp"
#include "../../model/aquatic/AquaticModels.hpp"
#include <memory>

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::aquatic {

/**
 * @brief 鳕鱼渲染器
 */
class CodRenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::CodModel> {
public:
    CodRenderer() { m_shadowSize = 0.3f; }
    ~CodRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/cod.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/cod.png");
    }
};

/**
 * @brief 鲑鱼渲染器
 */
class SalmonRenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::SalmonModel> {
public:
    SalmonRenderer() { m_shadowSize = 0.3f; }
    ~SalmonRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/salmon.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/salmon.png");
    }
};

/**
 * @brief 海豚渲染器
 */
class DolphinRenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::DolphinModel> {
public:
    DolphinRenderer() { m_shadowSize = 0.5f; }
    ~DolphinRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/dolphin.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/dolphin.png");
    }
};

/**
 * @brief 海龟渲染器
 */
class TurtleRenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::TurtleModel> {
public:
    TurtleRenderer() { m_shadowSize = 0.5f; }
    ~TurtleRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/turtle/sea_turtle.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/turtle/sea_turtle.png");
    }
};

void registerAquaticRenderers(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::aquatic
