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
 * @brief 雪球渲染器
 *
 * 渲染投掷的雪球实体。雪球使用物品纹理渲染为 billboard 四边形。
 */
class SnowballRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    SnowballRenderer();
    ~SnowballRenderer() override = default;

    // 禁止拷贝
    SnowballRenderer(const SnowballRenderer&) = delete;
    SnowballRenderer& operator=(const SnowballRenderer&) = delete;

    /**
     * @brief 渲染雪球（空实现，由 Vulkan 管线路径处理）
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
 * @brief 鸡蛋渲染器
 *
 * 渲染投掷的鸡蛋实体。鸡蛋使用物品纹理渲染为 billboard 四边形。
 */
class EggRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    EggRenderer();
    ~EggRenderer() override = default;

    EggRenderer(const EggRenderer&) = delete;
    EggRenderer& operator=(const EggRenderer&) = delete;

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

/**
 * @brief 末影珍珠渲染器
 *
 * 渲染投掷的末影珍珠实体。末影珍珠使用物品纹理渲染为 billboard 四边形。
 */
class EnderPearlRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    EnderPearlRenderer();
    ~EnderPearlRenderer() override = default;

    EnderPearlRenderer(const EnderPearlRenderer&) = delete;
    EnderPearlRenderer& operator=(const EnderPearlRenderer&) = delete;

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

/**
 * @brief 药水渲染器
 *
 * 渲染投掷的药水实体。药水使用物品纹理渲染为 billboard 四边形。
 */
class PotionRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    PotionRenderer();
    ~PotionRenderer() override = default;

    PotionRenderer(const PotionRenderer&) = delete;
    PotionRenderer& operator=(const PotionRenderer&) = delete;

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

/**
 * @brief 附魔之瓶渲染器
 *
 * 渲染投掷的附魔之瓶实体。附魔之瓶使用物品纹理渲染为 billboard 四边形。
 */
class ExperienceBottleRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    ExperienceBottleRenderer();
    ~ExperienceBottleRenderer() override = default;

    ExperienceBottleRenderer(const ExperienceBottleRenderer&) = delete;
    ExperienceBottleRenderer& operator=(const ExperienceBottleRenderer&) = delete;

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

/**
 * @brief 末影之眼渲染器
 *
 * 渲染投掷的末影之眼实体。末影之眼使用物品纹理渲染为 billboard 四边形。
 * 末影之眼使用全亮光照（fullbright），使其在黑暗中也清晰可见。
 */
class EyeOfEnderRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    EyeOfEnderRenderer();
    ~EyeOfEnderRenderer() override = default;

    EyeOfEnderRenderer(const EyeOfEnderRenderer&) = delete;
    EyeOfEnderRenderer& operator=(const EyeOfEnderRenderer&) = delete;

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
