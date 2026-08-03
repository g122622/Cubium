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

#include "PlayerRenderer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/layer/equipment/HeadLayer.hpp"
#include "client/renderer/trident/entity/layer/equipment/HeldItemLayer.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::renderer::player {

PlayerRenderer::PlayerRenderer(bool slimArms)
    : m_model(0.0f, slimArms)
    , m_slimArms(slimArms)
{
    // 设置阴影
    setShadowSize(0.5);
    setShadowAlpha(0.8);

    // 设置层渲染器
    _setupLayers();
}

void PlayerRenderer::render(Entity& entity, f64 partialTicks)
{
    // CPU 渲染路径已弃用：第三人称玩家由 renderWithPipeline → GPU 管线渲染。
    // 保留空实现仅满足基类纯虚契约，避免误用时静默无输出。
    (void)entity;
    (void)partialTicks;
}

void PlayerRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 分发到已注册的层（HeldItemLayer/HeadLayer）
    // 对应 MC 1.21.11 RenderLayer 遍历调用 submit() 的逻辑
    for (auto& layer : m_layers) {
        if (layer && layer->shouldRender(entity)) {
            layer->renderPipeline(entity, cmd, context, pipeline);
        }
    }
}

void PlayerRenderer::_setupLayers()
{
    // 手持物品层（主手/副手）：传入 *this 让 HeldItemLayer 通过
    // IEntityRenderer::getModel() 获取 PlayerModel，使 PlayerModel::translateHand
    // 的多态 override 生效（纤细手臂偏移等）。
    m_layers.push_back(
        std::make_unique<layer::equipment::HeldItemLayer<::mc::client::ClientEntity, model::player::PlayerModel>>(
            *this));

    // 头部物品层（头盔等）
    m_layers.push_back(
        std::make_unique<layer::equipment::HeadLayer<::mc::client::ClientEntity, model::player::PlayerModel>>(*this));

    spdlog::info("PlayerRenderer: Layer setup complete ({} layers registered)", m_layers.size());
}

ResourceLocation PlayerRenderer::getEntityTexture(::mc::client::ClientEntity& entity)
{
    (void)entity;
    // 玩家皮肤区域由 UvRemapFunc 按 entityId 解析，此处仅返回规范默认皮肤位置。
    return ResourceLocation("minecraft:textures/entity/player/slim/steve.png");
}

ResourceLocation PlayerRenderer::getEntityTexture(const ::mc::client::ClientEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft:textures/entity/player/slim/steve.png");
}

} // namespace mc::client::renderer::entity::renderer::player
