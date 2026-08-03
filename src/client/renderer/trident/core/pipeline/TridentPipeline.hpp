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

#include "client/renderer/api/pipeline/IPipeline.hpp"
#include "client/renderer/api/pipeline/RenderState.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前向声明
class TridentContext;

/**
 * @brief Vulkan 着色器模块
 */
struct TridentShaderModule {
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage;
    std::string entryPoint = "main";
};

/**
 * @brief 管线配置（Vulkan特有）
 */
struct TridentPipelineConfig {
    // 着色器路径
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    // 顶点输入
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

    // 输入装配
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkBool32 primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态设置）
    bool dynamicViewport = true;
    bool dynamicScissor = true;

    // 光栅化
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
    f64 lineWidth = 1.0f;

    // 多重采样
    VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkBool32 sampleShadingEnable = VK_FALSE;
    f64 minSampleShading = 1.0f;

    // 深度/模板
    VkBool32 depthTestEnable = VK_TRUE;
    VkBool32 depthWriteEnable = VK_TRUE;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkBool32 stencilTestEnable = VK_FALSE;

    // 颜色混合
    VkBool32 blendEnable = VK_FALSE;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // 渲染通道
    VkRenderPass renderPass = VK_NULL_HANDLE;
    u32 subpass = 0;

    // 管线布局
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

/**
 * @brief Vulkan 图形管线实现
 *
 * 实现 api::IPipeline 接口。
 */
class TridentPipeline : public api::IPipeline {
public:
    TridentPipeline();
    ~TridentPipeline() override;

    // 禁止拷贝
    TridentPipeline(const TridentPipeline&) = delete;
    TridentPipeline& operator=(const TridentPipeline&) = delete;

    // 允许移动
    TridentPipeline(TridentPipeline&& other) noexcept;
    TridentPipeline& operator=(TridentPipeline&& other) noexcept;

    /**
     * @brief 创建管线
     * @param context Trident 上下文
     * @param config 管线配置
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(TridentContext* context, const TridentPipelineConfig& config);

    /**
     * @brief 从 API 描述创建管线
     * @param context Trident 上下文
     * @param desc API 管线描述
     * @param renderPass 渲染通道
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> createFromDesc(
        TridentContext* context, const api::PipelineDesc& desc, VkRenderPass renderPass);

    // IPipeline 接口实现
    void destroy() override;
    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] const api::RenderState& renderState() const override { return m_renderState; }
    [[nodiscard]] bool isValid() const override { return m_pipeline != VK_NULL_HANDLE; }
    void bind(void* commandBuffer) override;

    // Vulkan 特有访问器
    [[nodiscard]] VkPipeline pipeline() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout layout() const { return m_layout; }

    /**
     * @brief 加载着色器模块
     */
    [[nodiscard]] static Result<VkShaderModule> createShaderModule(
        TridentContext* context, const std::vector<u8>& code);

private:
    TridentContext* m_context = nullptr;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    std::string m_name;
    api::RenderState m_renderState;
    std::vector<TridentShaderModule> m_shaders;

    /**
     * @brief 加载着色器模块
     */
    [[nodiscard]] Result<TridentShaderModule> _loadShader(const std::string& path, api::ShaderStage stage);
};

/**
 * @brief Vulkan 管线缓存
 *
 * 用于加速管线创建。
 */
class TridentPipelineCache {
public:
    TridentPipelineCache();
    ~TridentPipelineCache();

    // 禁止拷贝
    TridentPipelineCache(const TridentPipelineCache&) = delete;
    TridentPipelineCache& operator=(const TridentPipelineCache&) = delete;

    // 允许移动
    TridentPipelineCache(TridentPipelineCache&& other) noexcept;
    TridentPipelineCache& operator=(TridentPipelineCache&& other) noexcept;

    /**
     * @brief 初始化管线缓存
     * @param context Trident 上下文
     * @param cachePath 缓存文件路径（可选）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(TridentContext* context, const std::string& cachePath);

    void destroy();

    [[nodiscard]] VkPipelineCache cache() const { return m_cache; }
    [[nodiscard]] bool isValid() const { return m_cache != VK_NULL_HANDLE; }

private:
    TridentContext* m_context = nullptr;
    VkPipelineCache m_cache = VK_NULL_HANDLE;
    std::string m_cachePath;
};

} // namespace mc::client::renderer::trident
