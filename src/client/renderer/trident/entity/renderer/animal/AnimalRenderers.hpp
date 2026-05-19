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

#include "../../core/LivingRenderer.hpp"
#include "../../model/animal/AnimalModels.hpp"
#include "../../model/animal/BatModel.hpp"
#include "../../model/animal/RabbitModel.hpp"
#include "../../model/animal/SquidModel.hpp"
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
 *
 * 参考 MC 1.16.5 PigRenderer
 */
class PigRenderer : public core::LivingRenderer<LivingEntity, model::animal::PigModel> {
public:
    PigRenderer() { m_shadowSize = 0.7f; }
    ~PigRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/pig/pig.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
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
    CowRenderer() { m_shadowSize = 0.7f; }
    ~CowRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/cow.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
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
 * 参考 MC 1.16.5 MooshroomRenderer
 * 复用牛模型
 */
class MooshroomRenderer : public core::LivingRenderer<LivingEntity, model::animal::CowModel> {
public:
    MooshroomRenderer() { m_shadowSize = 0.7f; }
    ~MooshroomRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/mooshroom.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/cow/mooshroom.png");
    }
};

/**
 * @brief 鸡渲染器
 *
 * 参考 MC 1.16.5 ChickenRenderer
 */
class ChickenRenderer : public core::LivingRenderer<LivingEntity, model::animal::ChickenModel> {
public:
    ChickenRenderer() { m_shadowSize = 0.3f; }
    ~ChickenRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/chicken.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/chicken.png");
    }
};

/**
 * @brief 兔子渲染器
 *
 * 参考 MC 1.16.5 RabbitRenderer
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
 *
 * 参考 MC 1.16.5 BatRenderer
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
 *
 * 参考 MC 1.16.5 SquidRenderer
 */
class SquidRenderer : public core::LivingRenderer<LivingEntity, model::animal::SquidModel> {
public:
    SquidRenderer() { m_shadowSize = 0.7f; }
    ~SquidRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/squid.png");
    }

    [[nodiscard]] ResourceLocation getEntityTexture(const LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/squid.png");
    }
};

/**
 * @brief 注册所有动物渲染器
 *
 * @param manager 实体渲染器管理器引用
 */
void registerAnimalRenderers(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
