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
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc {
class Entity;
}

namespace mc::client {
class ClientEntity;
}

namespace mc::client::renderer::entity::renderer::projectile {

/**
 * @brief 火球渲染器
 *
 * 渲染恶魂发射的火球实体。火球使用实体纹理渲染为 billboard 四边形，
 * 使用全亮光照（fullbright）使其在黑暗中清晰可见。
 * 缩放比例为 3.0。
 */
class FireballRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    FireballRenderer();
    ~FireballRenderer() override = default;

    // 禁止拷贝
    FireballRenderer(const FireballRenderer&) = delete;
    FireballRenderer& operator=(const FireballRenderer&) = delete;

    /**
     * @brief 渲染火球（空实现，由 Vulkan 管线路径处理）
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 获取管线网格提供者
     */
    [[nodiscard]] core::PipelineMeshProvider* getPipelineMeshProvider() override { return this; }

    // ========== PipelineMeshProvider 接口 ==========

    /**
     * @brief 生成 billboard 网格
     */
    [[nodiscard]] bool generateMesh(::mc::client::ClientEntity& entity,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices) override;

    /**
     * @brief 网格不需要每帧更新
     */
    [[nodiscard]] bool needsMeshUpdate(::mc::client::ClientEntity& entity) const override;

private:
    /// 是否使用全亮光照
    bool m_fullbright;
    /// 缩放比例
    f64 m_scale;
};

/**
 * @brief 小火球渲染器
 *
 * 渲染烈焰人发射的小火球实体。小火球使用实体纹理渲染为 billboard 四边形，
 * 使用全亮光照（fullbright）使其在黑暗中清晰可见。
 * 缩放比例为 0.75。
 */
class SmallFireballRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    SmallFireballRenderer();
    ~SmallFireballRenderer() override = default;

    SmallFireballRenderer(const SmallFireballRenderer&) = delete;
    SmallFireballRenderer& operator=(const SmallFireballRenderer&) = delete;

    void render(Entity& entity, f64 partialTicks) override;
    [[nodiscard]] core::PipelineMeshProvider* getPipelineMeshProvider() override { return this; }

    [[nodiscard]] bool generateMesh(::mc::client::ClientEntity& entity,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices) override;
    [[nodiscard]] bool needsMeshUpdate(::mc::client::ClientEntity& entity) const override;

private:
    bool m_fullbright;
    f64 m_scale;
};

} // namespace mc::client::renderer::entity::renderer::projectile
