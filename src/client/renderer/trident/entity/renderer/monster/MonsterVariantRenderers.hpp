#pragma once

#include "../../core/LivingRenderer.hpp"
#include "../../model/monster/MonsterVariantModels.hpp"
#include <memory>

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::monster {

/**
 * @brief 僵尸村民渲染器
 */
class ZombieVillagerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::ZombieVillagerModel> {
public:
    ZombieVillagerRenderer() { m_shadowSize = 0.5f; }
    ~ZombieVillagerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie_villager/zombie_villager.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie_villager/zombie_villager.png");
    }
};

/**
 * @brief 溺尸渲染器
 */
class DrownedRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::DrownedModel> {
public:
    DrownedRenderer() { m_shadowSize = 0.5f; }
    ~DrownedRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie/drowned.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie/drowned.png");
    }
};

/**
 * @brief 尸壳渲染器
 */
class HuskRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::HuskModel> {
public:
    HuskRenderer() { m_shadowSize = 0.5f; }
    ~HuskRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie/husk.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie/husk.png");
    }
};

/**
 * @brief 流浪者渲染器
 */
class StrayRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::StrayModel> {
public:
    StrayRenderer() { m_shadowSize = 0.5f; }
    ~StrayRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/skeleton/stray.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/skeleton/stray.png");
    }
};

/**
 * @brief 洞穴蜘蛛渲染器
 */
class CaveSpiderRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::CaveSpiderModel> {
public:
    CaveSpiderRenderer() { m_shadowSize = 0.5f; }
    ~CaveSpiderRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/spider/cave_spider.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/spider/cave_spider.png");
    }
};

/**
 * @brief 巨人渲染器
 */
class GiantRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::GiantModel> {
public:
    GiantRenderer() { m_shadowSize = 0.5f; }
    ~GiantRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie/zombie.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zombie/zombie.png");
    }
};

void registerMonsterVariantRenderers(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::monster
