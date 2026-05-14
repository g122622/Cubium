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

#include "../../core/AnimationContext.hpp"
#include "../../model/core/EntityModel.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
}

namespace mc::client::renderer::entity::layer::core {

/**
 * @brief 层渲染器基类
 *
 * 用于在基础实体模型上添加额外渲染层（盔甲、鞍、发光效果等）。
 * 参考 MC 1.16.5 LayerRenderer
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class LayerRenderer {
public:
    virtual ~LayerRenderer() = default;

    /**
     * @brief 渲染层（CPU路径 - 已废弃）
     *
     * 此方法保留用于向后兼容，新代码应使用 renderPipeline()
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
        // 默认空实现，子类可选择实现此方法或 renderPipeline()
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
        // 默认实现：调用旧的 render 方法（如果子类实现了）
        // 子类应该重写此方法以使用 GPU 渲染
        (void)cmd;
        (void)pipeline;
        render(entity,
            static_cast<f32>(context.limbSwing),
            static_cast<f32>(context.limbSwingAmount),
            static_cast<f32>(context.partialTicks),
            static_cast<f32>(context.ageInTicks),
            static_cast<f32>(context.netHeadYaw),
            static_cast<f32>(context.headPitch),
            static_cast<f32>(context.scale));
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
