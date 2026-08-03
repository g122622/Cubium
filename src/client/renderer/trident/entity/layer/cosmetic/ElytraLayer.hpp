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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class LivingEntity;
struct TextureRegion;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::cosmetic {

/**
 * @brief 鞘翅层渲染器
 *
 * 渲染玩家装备的鞘翅。支持滑翔时的展开动画。
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class ElytraLayer : public core::LayerRenderer<TEntity> {
public:
    ElytraLayer() = default;
    ~ElytraLayer() override = default;

    /**
     * @brief 渲染鞘翅层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染鞘翅
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

    /**
     * @brief 设置自定义鞘翅纹理
     * @param region 纹理区域（可为 nullptr）
     */
    void setElytraTexture(const TextureRegion* region);

    /**
     * @brief 设置披风纹理（当没有鞘翅纹理时作为备选）
     * @param region 纹理区域（可为 nullptr）
     */
    void setCapeTexture(const TextureRegion* region);

    /**
     * @brief 获取当前鞘翅纹理
     */
    [[nodiscard]] const TextureRegion* getElytraTexture() const { return m_customElytraRegion; }

    /**
     * @brief 获取披风纹理
     */
    [[nodiscard]] const TextureRegion* getCapeTexture() const { return m_capeRegion; }

private:
    /**
     * @brief 计算鞘翅展开角度
     */
    [[nodiscard]] f32 _calculateElytraAngle(
        TEntity& entity, const mc::client::renderer::entity::core::AnimationContext& context, f32 partialTicks) const;

    /**
     * @brief 构建鞘翅网格
     */
    void _buildElytraMesh(
        f32 spreadAngle, bool isLeftWing, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建鞘翅网格
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateElytraMesh(f32 spreadAngle, pipeline::EntityPipeline& pipeline);

    const TextureRegion* m_customElytraRegion = nullptr;
    const TextureRegion* m_capeRegion = nullptr;

    // 鞘翅网格缓存（按展开角度离散化）
    std::unordered_map<i32, pipeline::EntityMesh> m_elytraMeshCache;
};

} // namespace mc::client::renderer::entity::layer::cosmetic
