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

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/layer/entity/WolfCollarLayer.hpp"
#include "client/renderer/trident/entity/model/animal/WolfModel.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace mc {
class WolfEntity;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 狼渲染器
 *
 * 继承自 EntityRenderer，重写 supportsAnimation()、supportsLayers() 和
 * renderLayersPipelineClient() 以支持 WolfCollarLayer 的注册和分发。
 *
 * 狼通过 GPU 管线路径渲染（EntityRendererManager::renderWithPipeline）：
 * 1. supportsAnimation() 返回 true → 进入 Path B（ModelFactory + AnimatedMeshCache）
 * 2. ModelRegistration 已注册 ET::WOLF → WolfModel，EntityRendererManager::_createModelForEntity
 *    中的 wolf 分支负责设置 setAnimState/setLivingAnimations/setTint
 * 3. 主模型网格生成后，supportsLayers() 返回 true → 调用 renderLayersPipelineClient
 *    → 分发到 WolfCollarLayer
 *
 * 层渲染数据流：
 * EntityRendererManager::renderWithPipeline
 *   → WolfRenderer::renderLayersPipelineClient(ClientEntity&, cmd, context, pipeline)
 *     → WolfCollarLayer::shouldRender(ClientEntity&) 检查 wolfTamed()
 *     → WolfCollarLayer::renderPipeline(ClientEntity&, cmd, context, pipeline)
 *       → _getCollarColor(entity) 读取 wolfCollarColor()
 *
 * WolfCollarLayer 通过 ClientEntity 的元数据镜像字段读取驯服状态和颈圈颜色，
 * 这些字段由 ClientEntity::syncMetadataFromDataManager 从服务端 DataParameter 同步：
 * - wolfTamed() ← TameableEntity::DATA_TAMED_PARAM (bool)
 * - wolfCollarColor() ← WolfEntity::DATA_COLLAR_COLOR_PARAM (i32, DyeColor)
 *
 * CatRenderer/OcelotRenderer/HorseRenderer/LlamaRenderer 已按相同模式重写
 * supportsAnimation() 进入 GPU 管线（层渲染暂未实现，详见 animal/README.md）。
 */
class WolfRenderer : public core::EntityRenderer {
public:
    WolfRenderer();
    ~WolfRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取狼纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::WolfEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::WolfEntity& entity) const;

    // ========== GPU 管线支持 ==========

    /**
     * @brief 狼支持动画
     *
     * 重写返回 true，使 EntityRendererManager 在 renderWithPipeline 中进入
     * Path B（ModelFactory + AnimatedMeshCache）生成主模型网格。
     * 这是狼进入 GPU 管线渲染并最终触发 renderLayersPipelineClient 的前置条件。
     *
     * 狼主模型的具体动画设置（setAnimState/setLivingAnimations/setTint）
     * 由 EntityRendererManager::_createModelForEntity 的 wolf 分支统一处理，
     * 不需要本渲染器实现 computeAnimationContext()。
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

    // ========== 层渲染支持 ==========

    /**
     * @brief 是否支持层渲染
     *
     * 重写返回 true，使 EntityRendererManager 在渲染主模型后
     * 调用 renderLayersPipelineClient 分发到已注册的层。
     */
    [[nodiscard]] bool supportsLayers() const override { return true; }

    /**
     * @brief 渲染层（GPU管线路径，ClientEntity 版本）
     *
     * 分发到已注册的层（WolfCollarLayer 等）。
     * 对应 MC 1.21.11 WolfRenderer 构造函数中的 addLayer 调用。
     *
     * @param entity 客户端实体
     * @param cmd Vulkan 命令缓冲区
     * @param context 动画上下文
     * @param pipeline 实体渲染管线
     */
    void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

private:
    void _setupLayers();

    model::animal::WolfModel m_model;
    model::animal::WolfModel m_modelBaby;

    // 已注册的层
    std::unique_ptr<layer::entity::WolfCollarLayer> m_collarLayer;
};

/**
 * @brief 注册狼渲染器
 */
void registerWolfRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
