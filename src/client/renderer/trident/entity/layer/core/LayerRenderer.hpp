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
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
}

namespace mc::client::renderer::entity::layer::core {

/**
 * @brief 层渲染器基类
 *
 * 用于在基础实体模型上添加额外渲染层（盔甲、鞍、发光效果等）。
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class LayerRenderer {
public:
    virtual ~LayerRenderer() = default;

    /**
     * @brief 渲染层（CPU路径 - 已废弃，保留为空实现以兼容旧代码路径）
     *
     * 所有 Layer 子类已迁移到 renderPipeline() GPU 管线路径。
     * 此方法保留空实现，仅用于 LivingRenderer::render() 旧路径的兼容。
     *
     * @param entity 实体
     * @param limbSwing 步态动画周期
     * @param limbSwingAmount 步态动画强度
     * @param partialTicks 部分tick
     * @param ageInTicks 年龄tick（用于空闲动画）
     * @param netHeadYaw 头部偏航角（相对身体）
     * @param headPitch 头部俯仰角
     * @param scale 缩放因子
     */
    virtual void render(TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale)
    {
        (void)entity;
        (void)limbSwing;
        (void)limbSwingAmount;
        (void)partialTicks;
        (void)ageInTicks;
        (void)netHeadYaw;
        (void)headPitch;
        (void)scale;
    }

    /**
     * @brief 渲染层（GPU管线路径）
     *
     * 使用 Vulkan 管线进行渲染。这是主要的渲染方法。
     *
     * @param entity 实体
     * @param cmd Vulkan 命令缓冲区
     * @param context 动画上下文
     * @param pipeline 实体渲染管线
     */
    virtual void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline)
    {
        // GPU 管线路径的默认空实现
        // 子类必须重写此方法以实现 GPU 渲染逻辑
        (void)entity;
        (void)cmd;
        (void)context;
        (void)pipeline;
    }

    /**
     * @brief 检查是否应该渲染此层
     * @param entity 实体
     * @return 是否应该渲染
     */
    [[nodiscard]] virtual bool shouldRender(const TEntity& entity) const
    {
        (void)entity;
        return true;
    }
};

} // namespace mc::client::renderer::entity::layer::core
