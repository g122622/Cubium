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
#include "client/renderer/trident/entity/model/monster/MonsterVariantModels.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
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
 *
 * TODO: 未实现 MC 1.21.11 DrownedRenderer.setupRotations 的游泳身体倾斜效果。
 * 原版在游泳时（swimAmount > 0）将整个实体身体绕 X 轴倾斜
 *   lerp(swimAmount, 0, -10 - xRot) 度，枢轴位于包围盒垂直中心：
 *   poseStack.rotateAround(Axis.XP.rotationDegrees(f2), 0, boundingBoxHeight/2/scale, 0);
 * 该效果是渲染器层面的整体身体倾斜（区别于 DrownedModel::setAngles 中的手臂/腿部
 * 覆盖动画），与 MC 原版 DrownedRenderer.setupRotations 一一对应。
 *
 * 实现该效果需要：
 *   1. 在 AnimationContext 中新增 xRot（实体俯仰角，区别于 headPitch）与
 *      boundingBoxHeight 字段；
 *   2. 在 LivingRenderer/EntityRenderer 基类新增 setupRotations 虚函数（或在
 *      computeCustomModelMatrix 中组合），由 EntityRendererManager 构建实体矩阵时
 *      调用；
 *   3. DrownedRenderer 覆盖该方法，按上述公式应用 X 轴倾斜。
 * 由于当前实体矩阵构建集中在 EntityRendererManager.cpp（无 setupRotations 虚函数
 * 钩子），且 AnimationContext 缺少 xRot/boundingBoxHeight 字段，该效果需要较大的
 * 渲染管线架构调整，暂留作后续 TODO。
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

} // namespace mc::client::renderer::entity::renderer::monster
