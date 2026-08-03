/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/entity/core/LivingRenderer.hpp"
#include "client/renderer/trident/entity/model/nether/NetherModels.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::nether {

/**
 * @brief 恶魂渲染器
 */
class GhastRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::GhastModel> {
public:
    GhastRenderer() { m_shadowSize = 0.0f; }
    ~GhastRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/ghast/ghast.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/ghast/ghast.png");
    }
};

/**
 * @brief 恶魂（发射火球时）渲染器
 */
class GhastShootingRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::GhastModel> {
public:
    GhastShootingRenderer() { m_shadowSize = 0.0f; }
    ~GhastShootingRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/ghast/ghast_shooting.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/ghast/ghast_shooting.png");
    }
};

/**
 * @brief 岩浆怪渲染器
 *
 * MC Java 中 MagmaCubeRenderer.getBlockLightLevel() 返回 15，
 * 岩浆怪在黑暗中也会发光，使用全亮光照。
 */
class MagmaCubeRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::MagmaCubeModel> {
public:
    MagmaCubeRenderer() { m_shadowSize = 0.0f; }
    ~MagmaCubeRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/slime/magmacube.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/slime/magmacube.png");
    }

    [[nodiscard]] bool isFullbright() const override { return true; }
};

/**
 * @brief 猪灵渲染器
 */
class PiglinRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::PiglinModel> {
public:
    PiglinRenderer() { m_shadowSize = 0.5f; }
    ~PiglinRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/piglin/piglin.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/piglin/piglin.png");
    }
};

/**
 * @brief 猪灵蛮兵渲染器
 */
class PiglinBruteRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::PiglinModel> {
public:
    PiglinBruteRenderer() { m_shadowSize = 0.5f; }
    ~PiglinBruteRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/piglin/piglin_brute.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/piglin/piglin_brute.png");
    }
};

/**
 * @brief 疣猪兽渲染器
 */
class HoglinRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::BoarModel> {
public:
    HoglinRenderer() { m_shadowSize = 0.7f; }
    ~HoglinRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/hoglin/hoglin.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/hoglin/hoglin.png");
    }
};

/**
 * @brief 僵尸疣兽渲染器
 */
class ZoglinRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::BoarModel> {
public:
    ZoglinRenderer() { m_shadowSize = 0.7f; }
    ~ZoglinRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zoglin/zoglin.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/zoglin/zoglin.png");
    }
};

/**
 * @brief 炽足兽渲染器
 */
class StriderRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::StriderModel> {
public:
    StriderRenderer() { m_shadowSize = 0.0f; }
    ~StriderRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/strider/strider.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/strider/strider.png");
    }
};

/**
 * @brief 炽足兽（颤抖时）渲染器
 */
class StriderShiveringRenderer : public core::LivingRenderer<::mc::LivingEntity, model::nether::StriderModel> {
public:
    StriderShiveringRenderer() { m_shadowSize = 0.0f; }
    ~StriderShiveringRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/strider/strider_cold.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/strider/strider_cold.png");
    }
};

} // namespace mc::client::renderer::entity::renderer::nether
