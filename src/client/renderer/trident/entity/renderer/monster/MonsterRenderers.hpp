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
#include "client/renderer/trident/entity/layer/entity/HeldBlockLayer.hpp"
#include "client/renderer/trident/entity/model/monster/BlazeModel.hpp"
#include "client/renderer/trident/entity/model/monster/CreeperModel.hpp"
#include "client/renderer/trident/entity/model/monster/EndermanModel.hpp"
#include "client/renderer/trident/entity/model/monster/SkeletonModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpiderModel.hpp"
#include "client/renderer/trident/entity/model/monster/ZombieModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

namespace client {
struct ChunkTextureAtlas;
} // namespace client
} // namespace mc

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
 *
 * 继承自 LivingRenderer，重写 renderLayersPipelineClient() 以分发到
 * HeldBlockLayer（手持方块层）和 EyesLayer（发光眼睛层）。
 *
 * GPU 管线渲染路径：
 * 1. EntityRendererManager::renderWithPipeline 进入 Path B（ModelFactory + AnimatedMeshCache）
 *    （LivingRenderer::supportsAnimation() 返回 true）
 * 2. ModelRegistration 已注册 ET::ENDERMAN → EndermanModel，
 *    EntityRendererManager::_createModelForEntity 中的 enderman 分支负责设置
 *    setCarrying/setAttacking（基于 ClientEntity::endermanHeldBlockState/endermanScreaming）
 * 3. 主模型网格生成后，supportsLayers() 返回 true → 调用 renderLayersPipelineClient
 *    → 分发到 HeldBlockLayer 和 EyesLayer
 *
 * 层渲染数据流：
 * EntityRendererManager::renderWithPipeline
 *   → EndermanRenderer::renderLayersPipelineClient(ClientEntity&, cmd, context, pipeline)
 *     → HeldBlockLayer::shouldRender(ClientEntity&) 检查 endermanHeldBlockState() != nullptr
 *     → HeldBlockLayer::renderPipeline(ClientEntity&, cmd, context, pipeline)
 *       → _getHeldBlock(entity) 读取 endermanHeldBlockState()
 *       → _getOrCreateBlockMesh(pipeline, blockState) 获取/构建方块网格
 *       → _renderBlockPipeline(...) 应用 CarriedBlockLayer 变换链并绘制
 *     → EyesLayer::shouldRender/renderPipeline（继承自 LivingRenderer 的 m_layers）
 *
 * 纹理图集注入：
 * - EntityRendererManager 在调用 renderLayersPipelineClient 前，会通过 setTextureAtlas
 *   将实体纹理图集注入到本渲染器，本渲染器再通过 setEntityTextureAtlas 传递给 HeldBlockLayer，
 *   供其在渲染方块后恢复 EntityPipeline 的纹理图集到实体纹理图集。
 * - 方块纹理图集通过 setChunkTextureAtlas 注入（由 EntityRendererManager 在初始化时设置）。
 */
class EndermanRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::EndermanModel> {
public:
    EndermanRenderer();
    ~EndermanRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染层（GPU管线路径，ClientEntity 版本）
     *
     * 分发到已注册的层：
     * - HeldBlockLayer（手持方块）：通过 ClientEntity::endermanHeldBlockState() 读取
     * - EyesLayer（发光眼睛）：继承自 LivingRenderer 的 m_layers
     *
     * 对应 MC 1.21.11 EndermanRenderer 构造函数中的 addLayer 调用。
     */
    void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 设置实体纹理图集
     *
     * 由 EntityRendererManager 在调用 renderLayersPipelineClient 前调用，
     * 将实体纹理图集注入到本渲染器。本渲染器将其传递给 HeldBlockLayer，
     * 供其在渲染方块后恢复 EntityPipeline 的纹理图集到实体纹理图集。
     *
     * @param atlas 实体纹理图集指针
     */
    void setTextureAtlas(const pipeline::EntityTextureAtlas* atlas) override;

    /**
     * @brief 设置方块纹理图集
     *
     * 由 EntityRendererManager 在初始化时调用，将方块纹理图集注入到本渲染器。
     * 本渲染器将其传递给 HeldBlockLayer，供其在渲染方块时切换 EntityPipeline
     * 的纹理图集到方块纹理图集。
     *
     * @param atlas 方块纹理图集指针
     */
    void setChunkTextureAtlas(const ::mc::client::ChunkTextureAtlas* atlas);

private:
    void _setupLayers();
    void _updateEndermanState(::mc::LivingEntity& entity);

    // 末影人手持方块层（直接持有，便于注入纹理图集）
    std::unique_ptr<layer::entity::HeldBlockLayer> m_heldBlockLayer;
};

/**
 * @brief 烈焰人渲染器
 *
 * MC Java 中 BlazeRenderer.getBlockLightLevel() 返回 15，
 * 烈焰人在黑暗中也会发光，使用全亮光照。
 */
class BlazeRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::BlazeModel> {
public:
    BlazeRenderer();
    ~BlazeRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override;

    [[nodiscard]] bool isFullbright() const override { return true; }

private:
    void _setupLayers();
};

} // namespace mc::client::renderer::entity::renderer::monster
