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
#include "client/renderer/trident/entity/model/animal/LlamaModel.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {
class LlamaEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 羊驼渲染器
 *
 * 支持 4 种颜色变体：Creamy, White, Brown, Gray
 *
 * 纹理路径：
 * - creamy: textures/entity/llama/creamy.png
 * - white: textures/entity/llama/white.png
 * - brown: textures/entity/llama/brown.png
 * - gray: textures/entity/llama/gray.png
 */
class LlamaRenderer : public core::EntityRenderer {
public:
    LlamaRenderer();
    ~LlamaRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取羊驼纹理
     *
     * 根据羊驼的颜色变体选择对应的纹理。
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LlamaEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LlamaEntity& entity) const;

    // ========== GPU 管线支持 ==========

    /**
     * @brief 羊驼支持动画
     *
     * 重写返回 true，使 EntityRendererManager 在 renderWithPipeline 中进入
     * Path B（ModelFactory + AnimatedMeshCache）生成主模型网格，消除“No mesh path”告警。
     * 羊驼主模型的动画设置由 _createModelForEntity 的 llama 分支统一处理。
     * 层渲染（背上的地毯等）暂未实现，保持 supportsLayers() 为基类默认 false。
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

private:
    model::animal::LlamaModel m_model;
    model::animal::LlamaModel m_modelBaby;
};

/**
 * @brief 注册羊驼渲染器
 */
void registerLlamaRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
