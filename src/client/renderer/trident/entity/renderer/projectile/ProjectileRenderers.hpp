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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

// Forward declaration
namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::projectile {

/**
 * @brief 箭矢渲染器
 *
 * 渲染箭矢实体的专用渲染器。
 *
 * 参考 MC 1.16.5 ArrowRenderer
 */
class ArrowRenderer : public core::EntityRenderer {
public:
    ArrowRenderer();
    ~ArrowRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取箭矢纹理位置
     */
    [[nodiscard]] static ResourceLocation getArrowTexture();

private:
    void generateArrowMesh();

    // 箭矢网格缓存
    bool m_meshGenerated = false;
};

/**
 * @brief 光灵箭渲染器
 *
 * 参考 MC 1.16.5 SpectralArrowRenderer
 */
class SpectralArrowRenderer : public core::EntityRenderer {
public:
    SpectralArrowRenderer();
    ~SpectralArrowRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] static ResourceLocation getSpectralArrowTexture();
};

/**
 * @brief 三叉戟渲染器
 *
 * 参考 MC 1.16.5 TridentRenderer
 */
class TridentRenderer : public core::EntityRenderer {
public:
    TridentRenderer();
    ~TridentRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] static ResourceLocation getTridentTexture();

private:
    void generateTridentMesh();

    bool m_meshGenerated = false;
};

/**
 * @brief 注册投掷物渲染器
 */
void registerProjectileRenderers(::mc::client::renderer::entity::EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::projectile
