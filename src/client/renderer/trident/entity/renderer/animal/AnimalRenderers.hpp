#pragma once

#include "LivingRenderer.hpp"
#include "../model/animal/AnimalModels.hpp"
#include <memory>

namespace mc {
class LivingEntity;
class SheepEntity;
}

namespace mc::client::renderer::entity::renderer::animal {

using mc::LivingEntity;
using mc::SheepEntity;

/**
 * @brief 猪渲染器
 *
 * 参考 MC 1.16.5 PigRenderer
 */
class PigRenderer : public core::LivingRenderer<LivingEntity, model::animal::PigModel> {
public:
    PigRenderer() {
        m_shadowSize = 0.5f;
    }
    ~PigRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/pig/pig.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/pig/pig.png");
    }
};

/**
 * @brief 牛渲染器
 *
 * 参考 MC 1.16.5 CowRenderer
 */
class CowRenderer : public core::LivingRenderer<LivingEntity, model::animal::CowModel> {
public:
    CowRenderer() {
        m_shadowSize = 0.7f;
    }
    ~CowRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/cow.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/cow.png");
    }
};

/**
 * @brief 羊渲染器
 *
 * 参考 MC 1.16.5 SheepRenderer
 * 包含羊毛层渲染器。
 */
class SheepRenderer : public core::LivingRenderer<LivingEntity, model::animal::SheepModel> {
public:
    SheepRenderer() {
        m_shadowSize = 0.7f;
        // TODO: 当 LayerRenderer 支持无参构造后添加羊毛层
        // setupLayers();
    }
    ~SheepRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/sheep/sheep.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/sheep/sheep.png");
    }
};

/**
 * @brief 鸡渲染器
 *
 * 参考 MC 1.16.5 ChickenRenderer
 */
class ChickenRenderer : public core::LivingRenderer<LivingEntity, model::animal::ChickenModel> {
public:
    ChickenRenderer() {
        m_shadowSize = 0.3f;
    }
    ~ChickenRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/chicken.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/chicken.png");
    }
};

/**
 * @brief 注册所有动物渲染器
 *
 * @param manager 实体渲染器管理器引用
 */
void registerAnimalRenderers(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
