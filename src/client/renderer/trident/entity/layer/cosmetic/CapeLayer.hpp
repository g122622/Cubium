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
class Player;
struct TextureRegion;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::cosmetic {

/**
 * @brief 斗篷层渲染器
 *
 * 渲染玩家的斗篷。支持动态摆动动画。
 */
class CapeLayer : public core::LayerRenderer<::mc::Player> {
public:
    CapeLayer() = default;
    ~CapeLayer() override = default;

    /**
     * @brief 渲染斗篷层（GPU管线路径）
     */
    void renderPipeline(::mc::Player& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染斗篷
     */
    [[nodiscard]] bool shouldRender(const ::mc::Player& entity) const override;

    /**
     * @brief 设置自定义斗篷纹理
     * @param region 纹理区域（可为 nullptr）
     */
    void setCapeTexture(const TextureRegion* region) { m_customCapeRegion = region; }

    /**
     * @brief 获取当前斗篷纹理
     */
    [[nodiscard]] const TextureRegion* getCapeTexture() const { return m_customCapeRegion; }

private:
    /**
     * @brief 计算斗篷摆动角度
     * @param entity 玩家实体
     * @param context 动画上下文
     * @param partialTicks 部分 tick
     * @return 斗篷旋转角度（度）
     */
    [[nodiscard]] f32 _calculateCapeSwing(::mc::Player& entity,
        const mc::client::renderer::entity::core::AnimationContext& context,
        f32 partialTicks) const;

    /**
     * @brief 构建斗篷网格
     */
    void _buildCapeMesh(f32 swingAngle, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建斗篷网格
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateCapeMesh(f32 swingAngle, pipeline::EntityPipeline& pipeline);

    const TextureRegion* m_customCapeRegion = nullptr;

    // 斗篷网格缓存（按摆动角度离散化）
    // 使用有限的几个角度来避免每帧重建
    std::unordered_map<i32, pipeline::EntityMesh> m_capeMeshCache;
};

} // namespace mc::client::renderer::entity::layer::cosmetic
