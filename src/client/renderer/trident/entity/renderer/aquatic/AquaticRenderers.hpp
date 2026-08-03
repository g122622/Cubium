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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/LivingRenderer.hpp"
#include "client/renderer/trident/entity/model/aquatic/AquaticModels.hpp"
#include "client/renderer/trident/entity/model/aquatic/PufferfishModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
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

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/cod.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
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

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/salmon.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
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

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/dolphin.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
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

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/turtle/big_sea_turtle.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/turtle/big_sea_turtle.png");
    }
};

/**
 * @brief 热带鱼A型渲染器（小体型）
 */
class TropicalFishARenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::TropicalFishAModel> {
public:
    TropicalFishARenderer() { m_shadowSize = 0.15f; }
    ~TropicalFishARenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/tropical_a.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/tropical_a.png");
    }
};

/**
 * @brief 热带鱼B型渲染器（大体型）
 */
class TropicalFishBRenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::TropicalFishBModel> {
public:
    TropicalFishBRenderer() { m_shadowSize = 0.2f; }
    ~TropicalFishBRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/tropical_b.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fish/tropical_b.png");
    }
};

/**
 * @brief 河豚渲染器
 *
 * 根据膨胀状态切换三种模型：小型、中型、大型。
 * 参考 MC 1.16.5 PufferfishRenderer。
 * 管线路径中由 EntityRendererManager 根据 puffState 选择模型。
 * 传统路径中由 render() 方法切换模型。
 * shadowSize 随膨胀状态动态调整: 0.1 + 0.1 * puffState
 */
class PufferfishRenderer : public core::EntityRenderer {
public:
    PufferfishRenderer()
        : m_smallModel()
        , m_mediumModel()
        , m_bigModel()
        , m_currentPuffState(-1)
    {
        m_shadowSize = 0.1f;
    }
    ~PufferfishRenderer() override = default;

    [[nodiscard]] bool supportsAnimation() const override { return true; }

    void render(Entity& entity, f64 partialTicks) override
    {
        // 传统渲染路径：根据膨胀状态切换模型
        i32 puffState = _getPuffState(entity);

        model::EntityModel* activeModel = nullptr;
        switch (puffState) {
            case 0:
                activeModel = &m_smallModel;
                break;
            case 1:
                activeModel = &m_mediumModel;
                break;
            case 2:
            default:
                activeModel = &m_bigModel;
                break;
        }

        m_currentPuffState = puffState;
        m_shadowSize = 0.1f + 0.1f * static_cast<f32>(puffState);

        // 设置模型动画角度
        auto& living = static_cast<::mc::LivingEntity&>(entity);
        f64 limbSwing = _getLimbSwing(living, partialTicks);
        f64 limbSwingAmount = _getLimbSwingAmount(living, partialTicks);
        f64 ageInTicks = _getAgeInTicks(living) + partialTicks;
        f64 headYaw = _getHeadYaw(living, partialTicks);
        f64 headPitch = _getHeadPitch(living, partialTicks);

        activeModel->setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, 1.0 / 16.0);
        activeModel->render(1.0 / 16.0);

        if (m_shadowSize > 0.0) {
            renderShadow(entity, partialTicks);
        }
    }

    void computeAnimationContext(Entity& entity,
        f64 partialTicks,
        core::AnimationContext& context,
        std::unique_ptr<model::EntityModel>& model) override
    {
        auto& living = static_cast<::mc::LivingEntity&>(entity);

        i32 puffState = _getPuffState(entity);
        m_currentPuffState = puffState;
        m_shadowSize = 0.1f + 0.1f * static_cast<f32>(puffState);

        context.partialTicks = partialTicks;
        context.limbSwing = _getLimbSwing(living, partialTicks);
        context.limbSwingAmount = _getLimbSwingAmount(living, partialTicks);
        context.ageInTicks = _getAgeInTicks(living);
        context.netHeadYaw = _getHeadYaw(living, partialTicks);
        context.headPitch = _getHeadPitch(living, partialTicks);
        context.scale = 1.0 / 16.0;

        context.isChild = false;
        context.isSitting = false;
        context.isSneaking = false;
        context.isSwimming = false;
        context.isRiding = false;
        context.swingProgress = 0.0f;
        context.puffState = puffState;

        context.computeHash();

        // 设置模型角度
        m_smallModel.setAngles(context.limbSwing,
            context.limbSwingAmount,
            context.ageInTicks,
            context.netHeadYaw,
            context.headPitch,
            context.scale * 16.0);

        model.reset();
    }

private:
    model::aquatic::PufferfishSmallModel m_smallModel;
    model::aquatic::PufferfishMediumModel m_mediumModel;
    model::aquatic::PufferfishBigModel m_bigModel;
    i32 m_currentPuffState;

    /**
     * @brief 从实体读取膨胀状态
     */
    static i32 _getPuffState(Entity& entity)
    {
        auto* clientEntity = dynamic_cast<::mc::client::ClientEntity*>(&entity);
        if (clientEntity != nullptr) {
            return clientEntity->puffState();
        }
        return 0;
    }

    // 辅助方法（与 LivingRenderer 相同的计算逻辑）
    static f64 _getLimbSwing(::mc::LivingEntity& entity, f64 partialTicks)
    {
        f64 limbSwingAmount = entity.limbSwingAmount();
        f64 result = entity.limbSwing() - limbSwingAmount * (1.0 - partialTicks);
        return result;
    }

    static f64 _getLimbSwingAmount(::mc::LivingEntity& entity, f64 partialTicks)
    {
        f64 prevAmount = entity.prevLimbSwingAmount();
        f64 amount = entity.limbSwingAmount();
        f64 result = prevAmount + (amount - prevAmount) * partialTicks;
        if (result > 1.0) result = 1.0;
        return result;
    }

    static f64 _getHeadYaw(::mc::LivingEntity& entity, f64 partialTicks)
    {
        f64 bodyYaw =
            entity.prevRenderYawOffset() + (entity.renderYawOffset() - entity.prevRenderYawOffset()) * partialTicks;
        f64 headYaw =
            entity.prevRotationYawHead() + (entity.rotationYawHead() - entity.prevRotationYawHead()) * partialTicks;
        f64 diff = headYaw - bodyYaw;
        while (diff < -180.0)
            diff += 360.0;
        while (diff > 180.0)
            diff -= 360.0;
        return diff;
    }

    static f64 _getHeadPitch(::mc::LivingEntity& entity, f64 partialTicks)
    {
        return entity.prevPitch() + (entity.pitch() - entity.prevPitch()) * partialTicks;
    }

    static f64 _getAgeInTicks(::mc::LivingEntity& entity) { return static_cast<f64>(entity.ticksExisted()); }
};

/**
 * @brief 美西螈渲染器
 *
 * 支持5种颜色变体：Lucy(白化/粉色), Wild(野生/棕色), Gold(金色), Cyan(青色), Blue(蓝色-稀有)
 * 根据 AxolotlEntity 的变体数据选择纹理。
 */
class AxolotlRenderer : public core::LivingRenderer<::mc::LivingEntity, model::aquatic::AxolotlModel> {
public:
    AxolotlRenderer() { m_shadowSize = 0.5f; }
    ~AxolotlRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        i32 variant = _getVariant(entity);
        return _getTextureForVariant(variant);
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        // const版本使用默认变体
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_lucy.png");
    }

private:
    /**
     * @brief 从实体读取变体数据
     */
    static i32 _getVariant(Entity& entity)
    {
        auto* clientEntity = dynamic_cast<::mc::client::ClientEntity*>(&entity);
        if (clientEntity != nullptr) {
            return clientEntity->axolotlVariant();
        }
        return 0;
    }

    /**
     * @brief 根据变体获取纹理路径
     */
    static ResourceLocation _getTextureForVariant(i32 variant)
    {
        switch (variant) {
            case 0:
                return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_lucy.png");
            case 1:
                return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_wild.png");
            case 2:
                return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_gold.png");
            case 3:
                return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_cyan.png");
            case 4:
                return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_blue.png");
            default:
                return ResourceLocation("minecraft", "textures/entity/axolotl/axolotl_lucy.png");
        }
    }
};

} // namespace mc::client::renderer::entity::renderer::aquatic
