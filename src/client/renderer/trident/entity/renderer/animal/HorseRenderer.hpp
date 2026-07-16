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
#include "client/renderer/trident/entity/model/animal/HorseModel.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
class HorseEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 马渲染器
 *
 * 支持多种马变种（普通马、驴、骡、骷髅马、僵尸马）。
 *
 * GPU 管线集成：重写 supportsAnimation() 返回 true，使
 * EntityRendererManager::renderWithPipeline 进入 Path B（ModelFactory +
 * AnimatedMeshCache）生成主模型网格，消除“No mesh path”告警。
 * 马模型的状态设置（setLivingAnimations）由 EntityRendererManager::
 * _createModelForEntity 的 horse 分支统一处理。层渲染（马铠等）暂未实现，
 * 保持 supportsLayers() 为基类默认 false。
 */
class HorseRenderer : public core::EntityRenderer {
public:
    HorseRenderer();
    ~HorseRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取马纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::HorseEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::HorseEntity& entity) const;

    // ========== GPU 管线支持 ==========

    /**
     * @brief 马支持动画
     *
     * 重写返回 true，使 EntityRendererManager 在 renderWithPipeline 中进入
     * Path B（ModelFactory + AnimatedMeshCache）生成主模型网格。
     * 马主模型的动画设置由 _createModelForEntity 的 horse 分支统一处理。
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

private:
    void _setupLayers();

    model::animal::HorseModel m_model;
    model::animal::HorseModel m_modelBaby;
};

/**
 * @brief 注册马渲染器
 */
void registerHorseRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
