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
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/LivingRenderer.hpp"
#include "client/renderer/trident/entity/layer/effect/EnergyGlintLayer.hpp"
#include "client/renderer/trident/entity/layer/effect/EyesLayer.hpp"
#include "client/renderer/trident/entity/layer/entity/HeldBlockLayer.hpp"
#include "client/renderer/trident/entity/layer/equipment/HeldItemLayer.hpp"
#include "client/renderer/trident/entity/model/monster/EndermanModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpiderModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::renderer::monster {

using mc::LivingEntity;
namespace layer_equipment = layer::equipment;
namespace layer_effect = layer::effect;

// ==================== 僵尸渲染器 ====================

ZombieRenderer::ZombieRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    _setupLayers();
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

void ZombieRenderer::_setupLayers()
{
    // 僵尸层渲染器：HeldItemLayer（手持物品）
    // TODO: ArmorLayer 需要渲染器引用，待实现后添加
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
}

// ==================== 骷髅渲染器 ====================

SkeletonRenderer::SkeletonRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    _setupLayers();
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

void SkeletonRenderer::_setupLayers()
{
    // 骷髅层渲染器：HeldItemLayer（手持物品）
    // TODO: ArmorLayer 需要渲染器引用，待实现后添加
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
}

// ==================== 苦力怕渲染器 ====================

CreeperRenderer::CreeperRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    _setupLayers();
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

void CreeperRenderer::_setupLayers()
{
    // 苦力怕层渲染器：EnergySwirlLayer（充能光效，当闪电苦力怕时）
    addLayer<layer_effect::EnergyGlintLayer<LivingEntity>>();
}

// ==================== 蜘蛛渲染器 ====================

SpiderRenderer::SpiderRenderer()
{
    setShadowSize(0.7f);
    setShadowAlpha(0.8f);
    _setupLayers();
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

void SpiderRenderer::_setupLayers()
{
    // 蜘蛛层渲染器：EyesLayer（发光眼睛）
    // TODO: SaddleLayer（鞍，洞穴蜘蛛骑乘时），待实现后添加
    addLayer<layer_effect::EyesLayer<LivingEntity, model::monster::SpiderModel>>(*this);
}

// ==================== 末影人渲染器 ====================

EndermanRenderer::EndermanRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    _setupLayers();
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
    _updateEndermanState(living);
    LivingRenderer::render(entity, partialTicks);
}

void EndermanRenderer::_updateEndermanState(LivingEntity& entity)
{
    auto& enderman = static_cast<::mc::EndermanEntity&>(entity);
    m_model.setCarrying(enderman.isHoldingBlock());
    m_model.setAttacking(enderman.isScreaming());
}

void EndermanRenderer::_setupLayers()
{
    // 末影人层渲染器：
    // - HeldBlockLayer（手持方块）：直接持有，便于注入纹理图集
    // - EyesLayer（发光眼睛）：通过 addLayer 注册到 m_layers
    m_heldBlockLayer = std::make_unique<layer::entity::HeldBlockLayer>();
    addLayer<layer_effect::EyesLayer<LivingEntity, model::monster::EndermanModel>>(*this);
}

void EndermanRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 分发到手持方块层（对应 MC 1.21.11 EndermanRenderer 构造函数中的 addLayer(new CarriedBlockLayer(this))）
    if (m_heldBlockLayer != nullptr) {
        if (m_heldBlockLayer->shouldRender(entity)) {
            m_heldBlockLayer->renderPipeline(entity, cmd, context, pipeline);
        }
    }

    // 分发到 m_layers 中的层（EyesLayer 等）
    // 复用 LivingRenderer 的 renderLayersPipeline(Entity&) 路径不合适（需要 ClientEntity），
    // 这里手动遍历 m_layers 并调用 renderPipeline
    // 注意：EyesLayer<LivingEntity, EndermanModel> 是 LayerRenderer<LivingEntity>，
    // 而 ClientEntity 不是 LivingEntity 的派生类，因此不能直接调用。
    // EyesLayer 的 GPU 管线路径通过 EntityRendererManager 的主网格生成（_createModelForEntity）
    // 处理眼睛发光纹理，层本身只在 CPU 路径渲染。这里跳过 EyesLayer 的 GPU 管线路径。
    // TODO: 后续 EyesLayer 也应迁移为 ClientEntity 模板，以支持 GPU 管线路径。
}

void EndermanRenderer::setTextureAtlas(const pipeline::EntityTextureAtlas* atlas)
{
    // 调用父类方法（保存到 EntityRenderer 的 m_textureAtlas，供其他层使用）
    EntityRenderer::setTextureAtlas(atlas);

    // 将实体纹理图集传递给 HeldBlockLayer，供其在渲染方块后恢复纹理图集
    if (m_heldBlockLayer != nullptr) {
        m_heldBlockLayer->setEntityTextureAtlas(atlas);
    }
}

void EndermanRenderer::setBlockAtlas(VkImageView imageView, VkSampler sampler)
{
    // 将 blocks atlas 句柄传递给 HeldBlockLayer，供其在渲染方块时切换纹理图集
    if (m_heldBlockLayer != nullptr) {
        m_heldBlockLayer->setBlockAtlas(imageView, sampler);
    }
}

// ==================== 烈焰人渲染器 ====================

BlazeRenderer::BlazeRenderer()
{
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    _setupLayers();
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

void BlazeRenderer::_setupLayers()
{
    // 烈焰人没有特殊的层渲染器
}

} // namespace mc::client::renderer::entity::renderer::monster
