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
#include "client/renderer/trident/entity/model/animal/BatModel.hpp"
#include "client/renderer/trident/entity/model/animal/ChickenModel.hpp"
#include "client/renderer/trident/entity/model/animal/CowModel.hpp"
#include "client/renderer/trident/entity/model/animal/PigModel.hpp"
#include "client/renderer/trident/entity/model/animal/RabbitModel.hpp"
#include "client/renderer/trident/entity/model/animal/SheepModel.hpp"
#include "client/renderer/trident/entity/model/animal/SquidModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class LivingEntity;
class SheepEntity;
} // namespace mc

namespace mc::client::renderer::entity::renderer::animal {

using mc::LivingEntity;
using mc::SheepEntity;

/**
 * @brief 猪渲染器
 */
class PigRenderer : public core::LivingRenderer<LivingEntity, model::animal::PigModel> {
public:
    PigRenderer() { m_shadowSize = 0.7f; }
    ~PigRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/pig/temperate_pig.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/pig/temperate_pig.png");
    }
};

/**
 * @brief 牛渲染器
 */
class CowRenderer : public core::LivingRenderer<LivingEntity, model::animal::CowModel> {
public:
    CowRenderer() { m_shadowSize = 0.7f; }
    ~CowRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/temperate_cow.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/temperate_cow.png");
    }
};

/**
 * @brief 羊渲染器
 *
 * 包含羊毛层渲染器。
 */
class SheepRenderer : public core::LivingRenderer<LivingEntity, model::animal::SheepModel> {
public:
    SheepRenderer() { m_shadowSize = 0.7f; }
    ~SheepRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/sheep/sheep.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/sheep/sheep.png");
    }
};

/**
 * @brief 哞菇渲染器
 *
 * 复用牛模型
 */
class MooshroomRenderer : public core::LivingRenderer<LivingEntity, model::animal::CowModel> {
public:
    MooshroomRenderer() { m_shadowSize = 0.7f; }
    ~MooshroomRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/red_mooshroom.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/red_mooshroom.png");
    }
};

/**
 * @brief 鸡渲染器
 */
class ChickenRenderer : public core::LivingRenderer<LivingEntity, model::animal::ChickenModel> {
public:
    ChickenRenderer() { m_shadowSize = 0.3f; }
    ~ChickenRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/chicken/temperate_chicken.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/chicken/temperate_chicken.png");
    }
};

/**
 * @brief 兔子渲染器
 */
class RabbitRenderer : public core::LivingRenderer<LivingEntity, model::animal::RabbitModel> {
public:
    RabbitRenderer() { m_shadowSize = 0.3f; }
    ~RabbitRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/rabbit/brown.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/rabbit/brown.png");
    }
};

/**
 * @brief 蝙蝠渲染器
 */
class BatRenderer : public core::LivingRenderer<LivingEntity, model::animal::BatModel> {
public:
    BatRenderer() { m_shadowSize = 0.3f; }
    ~BatRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bat.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bat.png");
    }
};

/**
 * @brief 鱿鱼渲染器
 */
class SquidRenderer : public core::LivingRenderer<LivingEntity, model::animal::SquidModel> {
public:
    SquidRenderer() { m_shadowSize = 0.7f; }
    ~SquidRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/squid/squid.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/squid/squid.png");
    }
};

/**
 * @brief 发光鱿鱼渲染器
 *
 * 复用 SquidModel，仅纹理不同。对应 MC Java GlowSquidRenderer extends SquidRenderer。
 */
class GlowSquidRenderer : public core::LivingRenderer<LivingEntity, model::animal::SquidModel> {
public:
    GlowSquidRenderer() { m_shadowSize = 0.7f; }
    ~GlowSquidRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/squid/glow_squid.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/squid/glow_squid.png");
    }
};

} // namespace mc::client::renderer::entity::renderer::animal
