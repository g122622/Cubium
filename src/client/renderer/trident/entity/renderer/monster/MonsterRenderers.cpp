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

#include "MonsterRenderers.hpp"
#include "../../layer/effect/EnergyGlintLayer.hpp"
#include "../../layer/effect/EyesLayer.hpp"
#include "../../layer/entity/HeldBlockLayer.hpp"
#include "../../layer/equipment/HeldItemLayer.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"

namespace mc::client::renderer::entity::renderer::monster {

using mc::LivingEntity;
namespace layer_equipment = layer::equipment;
namespace layer_effect = layer::effect;

// ==================== 僵尸渲染器 ====================

ZombieRenderer::ZombieRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation ZombieRenderer::getEntityTexture(LivingEntity& entity)
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/zombie/zombie.png");
}

ResourceLocation ZombieRenderer::getEntityTexture(const LivingEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/zombie/zombie.png");
}

void ZombieRenderer::setupLayers()
{
    // 参考 MC 1.16.5 ZombieRenderer 构造函数
    // 僵尸有以下层渲染器：
    // - BipedArmorLayer（盔甲）
    // - HeldItemLayer（手持物品）
    // - HeadLayer（头部物品 - 仅僵尸村民）
    // 注意：ArmorLayer 需要渲染器引用，暂时注释
    // addLayer<layer_equipment::ArmorLayer<LivingEntity, model::monster::ZombieModel>>(*this);
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
}

// ==================== 骷髅渲染器 ====================

SkeletonRenderer::SkeletonRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation SkeletonRenderer::getEntityTexture(LivingEntity& entity)
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/skeleton/skeleton.png");
}

ResourceLocation SkeletonRenderer::getEntityTexture(const LivingEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/skeleton/skeleton.png");
}

void SkeletonRenderer::setupLayers()
{
    // 参考 MC 1.16.5 SkeletonRenderer 构造函数
    // 骷髅有以下层渲染器：
    // - BipedArmorLayer（盔甲）
    // - HeldItemLayer（手持物品）
    // addLayer<layer_equipment::ArmorLayer<LivingEntity, model::monster::SkeletonModel>>(*this);
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
}

// ==================== 苦力怕渲染器 ====================

CreeperRenderer::CreeperRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation CreeperRenderer::getEntityTexture(LivingEntity& entity)
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/creeper/creeper.png");
}

ResourceLocation CreeperRenderer::getEntityTexture(const LivingEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/creeper/creeper.png");
}

void CreeperRenderer::setupLayers()
{
    // 参考 MC 1.16.5 CreeperRenderer 构造函数
    // 苦力怕有以下层渲染器：
    // - EnergySwirlLayer（充能光效，当闪电苦力怕时）
    addLayer<layer_effect::EnergyGlintLayer<LivingEntity>>();
}

// ==================== 蜘蛛渲染器 ====================

SpiderRenderer::SpiderRenderer()
{
    setShadowSize(0.7f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation SpiderRenderer::getEntityTexture(LivingEntity& entity)
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/spider/spider.png");
}

ResourceLocation SpiderRenderer::getEntityTexture(const LivingEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/spider/spider.png");
}

void SpiderRenderer::setupLayers()
{
    // 参考 MC 1.16.5 SpiderRenderer 构造函数
    // 蜘蛛有以下层渲染器：
    // - EyesLayer（发光眼睛）
    // - SaddleLayer（鞍，如果是洞穴蜘蛛骑乘时）
    addLayer<layer_effect::EyesLayer<LivingEntity, model::monster::SpiderModel>>(*this);
}

// ==================== 末影人渲染器 ====================

EndermanRenderer::EndermanRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation EndermanRenderer::getEntityTexture(LivingEntity& entity)
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/enderman/enderman.png");
}

ResourceLocation EndermanRenderer::getEntityTexture(const LivingEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/enderman/enderman.png");
}

void EndermanRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& living = static_cast<LivingEntity&>(entity);
    updateEndermanState(living);
    LivingRenderer::render(entity, partialTicks);
}

void EndermanRenderer::updateEndermanState(LivingEntity& entity)
{
    // 参考 MC 1.16.5 EndermanRenderer.render()
    // model.isCarrying = blockstate != null;
    // model.isAttacking = entityIn.isScreaming();

    // 使用 dynamic_cast 安全地转换为 EndermanEntity
    auto* enderman = dynamic_cast<::mc::EndermanEntity*>(&entity);
    if (enderman != nullptr) {
        m_model.setCarrying(enderman->isHoldingBlock());
        m_model.setAttacking(enderman->isScreaming());
    } else {
        m_model.setCarrying(false);
        m_model.setAttacking(false);
    }
}

void EndermanRenderer::setupLayers()
{
    // 参考 MC 1.16.5 EndermanRenderer 构造函数
    // 末影人有以下层渲染器：
    // - HeldBlockLayer（手持方块）
    // - EyesLayer（发光眼睛）
    // 注意：HeldBlockLayer<LivingEntity> 使用 if constexpr 在运行时检查 EndermanEntity
    addLayer<layer::entity::HeldBlockLayer<LivingEntity>>();
    addLayer<layer_effect::EyesLayer<LivingEntity, model::monster::EndermanModel>>(*this);
}

// ==================== 烈焰人渲染器 ====================

BlazeRenderer::BlazeRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation BlazeRenderer::getEntityTexture(LivingEntity& entity)
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/blaze.png");
}

ResourceLocation BlazeRenderer::getEntityTexture(const LivingEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/blaze.png");
}

void BlazeRenderer::setupLayers()
{
    // 参考 MC 1.16.5 BlazeRenderer 构造函数
    // 烈焰人没有特殊的层渲染器
}

} // namespace mc::client::renderer::entity::renderer::monster
