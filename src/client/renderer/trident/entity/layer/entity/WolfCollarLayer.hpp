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

#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/animal/WolfModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class WolfEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 狼项圈层渲染器
 *
 * 渲染驯服狼的项圈。支持不同颜色的项圈。
 */
class WolfCollarLayer : public core::LayerRenderer<::mc::WolfEntity> {
public:
    WolfCollarLayer() = default;
    ~WolfCollarLayer() override = default;

    /**
     * @brief 渲染项圈层（GPU管线路径）
     */
    void renderPipeline(::mc::WolfEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染项圈
     */
    [[nodiscard]] bool shouldRender(const ::mc::WolfEntity& entity) const override;

private:
    /**
     * @brief 获取项圈颜色
     * @param entity 狼实体
     * @return RGB颜色值
     */
    [[nodiscard]] static Vector3f _getCollarColor(const ::mc::WolfEntity& entity);

    /**
     * @brief 构建项圈网格
     */
    void _buildCollarMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建项圈网格
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateCollarMesh(pipeline::EntityPipeline& pipeline);

    // 项圈网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_collarMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
