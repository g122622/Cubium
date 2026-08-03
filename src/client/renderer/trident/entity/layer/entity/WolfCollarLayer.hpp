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
#include "client/renderer/trident/entity/layer/entity/WolfCollarColors.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 狼项圈层渲染器
 *
 * 渲染驯服狼的项圈。支持不同颜色的项圈。
 *
 * 数据来源：通过 ClientEntity 的元数据镜像字段读取驯服状态和颈圈颜色。
 * - wolfTamed(): 通过 TameableEntity::DATA_TAMED_PARAM 同步，仅驯服的狼渲染项圈
 * - wolfCollarColor(): 通过 WolfEntity::DATA_COLLAR_COLOR_PARAM 同步，决定项圈色调
 *
 * 颜色映射逻辑（DyeColor → RGB）定义在 WolfCollarColors.hpp 中，
 * 可独立于 Vulkan 渲染管线进行单元测试。
 *
 * 对应 MC 1.21.11 WolfCollarLayer：
 * - shouldRender 检查 collarColor != null（MC 中 extractRenderState 在未驯服时设为 null）
 * - render 使用 DyeColor.getTextureDiffuseColor() 作为项圈色调
 */
class WolfCollarLayer : public core::LayerRenderer<::mc::client::ClientEntity> {
public:
    WolfCollarLayer() = default;
    ~WolfCollarLayer() override = default;

    /**
     * @brief 渲染项圈层（GPU管线路径）
     */
    void renderPipeline(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染项圈
     *
     * 只有驯服的狼才显示项圈。
     * 对应 MC 1.21.11 WolfCollarLayer.submit() 中的 collarColor != null 检查。
     */
    [[nodiscard]] bool shouldRender(const ::mc::client::ClientEntity& entity) const override;

private:
    /**
     * @brief 获取项圈颜色
     * @param entity 客户端实体（狼）
     * @return RGB颜色值
     */
    [[nodiscard]] static Vector3f _getCollarColor(const ::mc::client::ClientEntity& entity);

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
