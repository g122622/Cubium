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

#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/core/LivingRenderer.hpp"
#include "client/renderer/trident/entity/model/monster/BlazeModel.hpp"
#include "client/renderer/trident/entity/model/monster/CreeperModel.hpp"
#include "client/renderer/trident/entity/model/monster/EndermanModel.hpp"
#include "client/renderer/trident/entity/model/monster/SkeletonModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpiderModel.hpp"
#include "client/renderer/trident/entity/model/monster/ZombieModel.hpp"
#include <memory>

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::renderer::monster {

using mc::LivingEntity;

/**
 * @brief 僵尸渲染器
 */
class ZombieRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::ZombieModel> {
public:
    ZombieRenderer();
    ~ZombieRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

private:
    void _setupLayers();
};

/**
 * @brief 骷髅渲染器
 */
class SkeletonRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::SkeletonModel> {
public:
    SkeletonRenderer();
    ~SkeletonRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

private:
    void _setupLayers();
};

/**
 * @brief 苦力怕渲染器
 */
class CreeperRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::CreeperModel> {
public:
    CreeperRenderer();
    ~CreeperRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

private:
    void _setupLayers();
};

/**
 * @brief 蜘蛛渲染器
 */
class SpiderRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::SpiderModel> {
public:
    SpiderRenderer();
    ~SpiderRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

private:
    void _setupLayers();
};

/**
 * @brief 末影人渲染器
 */
class EndermanRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::EndermanModel> {
public:
    EndermanRenderer();
    ~EndermanRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

    void render(Entity& entity, f64 partialTicks) override;

private:
    void _setupLayers();
    void _updateEndermanState(::mc::LivingEntity& entity);
};

/**
 * @brief 烈焰人渲染器
 */
class BlazeRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::BlazeModel> {
public:
    BlazeRenderer();
    ~BlazeRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

private:
    void _setupLayers();
};

} // namespace mc::client::renderer::entity::renderer::monster
