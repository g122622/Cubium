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
#include "client/renderer/trident/entity/model/animal/CatModel.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <array>
#include <memory>

namespace mc {
class CatEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 猫渲染器
 *
 * 支持 11 种猫皮肤（类型）。
 */
class CatRenderer : public core::EntityRenderer {
public:
    CatRenderer();
    ~CatRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取猫纹理
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::CatEntity& entity);
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::CatEntity& entity) const;

    // ========== GPU 管线支持 ==========

    /**
     * @brief 猫支持动画
     *
     * 重写返回 true，使 EntityRendererManager 在 renderWithPipeline 中进入
     * Path B（ModelFactory + AnimatedMeshCache）生成主模型网格，消除“No mesh path”告警。
     * 猫主模型的动画设置（setCatAnimState/setSitting/setLivingAnimations）由
     * _createModelForEntity 的 cat 分支统一处理。
     * 层渲染（颈圈等）暂未实现，保持 supportsLayers() 为基类默认 false。
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

private:
    model::animal::CatModel m_model;
    model::animal::CatModel m_modelBaby;

    /**
     * @brief 获取猫类型对应的纹理
     * @param catType 猫类型 (0-10)
     * @return 纹理位置
     */
    [[nodiscard]] static ResourceLocation _getCatTexture(u32 catType);
};

/**
 * @brief 注册猫渲染器
 */
void registerCatRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
