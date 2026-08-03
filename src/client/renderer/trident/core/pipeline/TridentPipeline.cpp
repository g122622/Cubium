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

#include "TridentPipeline.hpp"
#include "client/renderer/api/BlendMode.hpp"
#include "client/renderer/api/CompareOp.hpp"
#include "client/renderer/api/CullMode.hpp"
#include "client/renderer/api/pipeline/IPipeline.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <fstream>
#include <ios>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

namespace {

// ============================================================================
// 辅助转换函数
// ============================================================================

/**
 * @brief 将 API BlendFactor 转换为 Vulkan VkBlendFactor
 */
VkBlendFactor toVulkanBlendFactor(api::BlendFactor factor)
{
    switch (factor) {
        case api::BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case api::BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case api::BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case api::BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case api::BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case api::BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case api::BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case api::BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case api::BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case api::BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case api::BlendFactor::ConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case api::BlendFactor::OneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case api::BlendFactor::ConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case api::BlendFactor::OneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case api::BlendFactor::SrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        default:
            return VK_BLEND_FACTOR_ZERO;
    }
}

/**
 * @brief 将 API BlendOp 转换为 Vulkan VkBlendOp
 */
VkBlendOp toVulkanBlendOp(api::BlendOp op)
{
    switch (op) {
        case api::BlendOp::Add:
            return VK_BLEND_OP_ADD;
        case api::BlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case api::BlendOp::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case api::BlendOp::Min:
            return VK_BLEND_OP_MIN;
        case api::BlendOp::Max:
            return VK_BLEND_OP_MAX;
        default:
            return VK_BLEND_OP_ADD;
    }
}

/**
 * @brief 将 API CompareOp 转换为 Vulkan VkCompareOp
 */
VkCompareOp toVulkanCompareOp(api::CompareOp op)
{
    switch (op) {
        case api::CompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case api::CompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case api::CompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case api::CompareOp::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case api::CompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case api::CompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case api::CompareOp::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case api::CompareOp::Always:
            return VK_COMPARE_OP_ALWAYS;
        default:
            return VK_COMPARE_OP_LESS;
    }
}

/**
 * @brief 将 API CullMode 转换为 Vulkan VkCullModeFlags
 */
VkCullModeFlags toVulkanCullMode(api::CullMode mode)
{
    switch (mode) {
        case api::CullMode::None:
            return VK_CULL_MODE_NONE;
        case api::CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
        case api::CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        case api::CullMode::FrontAndBack:
            return VK_CULL_MODE_FRONT_AND_BACK;
        default:
            return VK_CULL_MODE_BACK_BIT;
    }
}

/**
 * @brief 将 API FrontFace 转换为 Vulkan VkFrontFace
 */
VkFrontFace toVulkanFrontFace(api::FrontFace face)
{
    switch (face) {
        case api::FrontFace::CounterClockwise:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case api::FrontFace::Clockwise:
            return VK_FRONT_FACE_CLOCKWISE;
        default:
            return VK_FRONT_FACE_CLOCKWISE;
    }
}

/**
 * @brief 将 API PolygonMode 转换为 Vulkan VkPolygonMode
 */
VkPolygonMode toVulkanPolygonMode(api::PolygonMode mode)
{
    switch (mode) {
        case api::PolygonMode::Fill:
            return VK_POLYGON_MODE_FILL;
        case api::PolygonMode::Line:
            return VK_POLYGON_MODE_LINE;
        case api::PolygonMode::Point:
            return VK_POLYGON_MODE_POINT;
        default:
            return VK_POLYGON_MODE_FILL;
    }
}

/**
 * @brief 从文件加载着色器
 */
Result<TridentShaderModule> loadShaderFromFile(TridentContext* context, const std::string& path, api::ShaderStage stage)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, "Failed to open shader file: " + path);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<u8> code(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(fileSize));
    file.close();

    auto moduleResult = TridentPipeline::createShaderModule(context, code);
    if (moduleResult.failed()) {
        return moduleResult.error();
    }

    TridentShaderModule shader;
    shader.module = moduleResult.value();
    shader.stage = stage == api::ShaderStage::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
    shader.entryPoint = "main";

    spdlog::info("Loaded shader: {} ({} bytes)", path, fileSize);
    return shader;
}

} // namespace

// ============================================================================
// TridentPipeline 实现
// ============================================================================

TridentPipeline::TridentPipeline() = default;

TridentPipeline::~TridentPipeline()
{
    destroy();
}

TridentPipeline::TridentPipeline(TridentPipeline&& other) noexcept
    : m_context(other.m_context)
    , m_pipeline(other.m_pipeline)
    , m_layout(other.m_layout)
    , m_name(std::move(other.m_name))
    , m_renderState(other.m_renderState)
    , m_shaders(std::move(other.m_shaders))
{
    other.m_context = nullptr;
    other.m_pipeline = VK_NULL_HANDLE;
    other.m_layout = VK_NULL_HANDLE;
}

TridentPipeline& TridentPipeline::operator=(TridentPipeline&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_pipeline = other.m_pipeline;
        m_layout = other.m_layout;
        m_name = std::move(other.m_name);
        m_renderState = other.m_renderState;
        m_shaders = std::move(other.m_shaders);

        other.m_context = nullptr;
        other.m_pipeline = VK_NULL_HANDLE;
        other.m_layout = VK_NULL_HANDLE;
    }
    return *this;
}

Result<void> TridentPipeline::create(TridentContext* context, const TridentPipelineConfig& config)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        destroy();
    }

    m_context = context;
    m_name = "TridentPipeline";

    // 创建管线布局
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<u32>(config.descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = config.descriptorSetLayouts.empty() ? nullptr : config.descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<u32>(config.pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = config.pushConstantRanges.empty() ? nullptr : config.pushConstantRanges.data();

    VkResult result = vkCreatePipelineLayout(m_context->device(), &layoutInfo, nullptr, &m_layout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to create pipeline layout: " + std::to_string(static_cast<i32>(result)));
    }

    // 加载着色器（如果尚未通过 createFromDesc 加载）
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    // 仅当 m_shaders 为空时才从文件加载
    if (m_shaders.empty()) {
        if (!config.vertexShaderPath.empty()) {
            auto shaderResult = _loadShader(config.vertexShaderPath, api::ShaderStage::Vertex);
            if (shaderResult.failed()) {
                destroy();
                return shaderResult.error();
            }
            m_shaders.push_back(shaderResult.value());
        }

        if (!config.fragmentShaderPath.empty()) {
            auto shaderResult = _loadShader(config.fragmentShaderPath, api::ShaderStage::Fragment);
            if (shaderResult.failed()) {
                destroy();
                return shaderResult.error();
            }
            m_shaders.push_back(shaderResult.value());
        }
    }

    // 着色器阶段
    for (const auto& shader : m_shaders) {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = shader.stage;
        stageInfo.module = shader.module;
        stageInfo.pName = shader.entryPoint.c_str();
        shaderStages.push_back(stageInfo);
    }

    // 顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(config.vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = config.vertexBindings.empty() ? nullptr : config.vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(config.vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions =
        config.vertexAttributes.empty() ? nullptr : config.vertexAttributes.data();

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.topology;
    inputAssembly.primitiveRestartEnable = config.primitiveRestartEnable;

    // 视口和裁剪（动态）
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // 动态
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // 动态

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = config.polygonMode;
    rasterizer.lineWidth = config.lineWidth;
    rasterizer.cullMode = config.cullMode;
    rasterizer.frontFace = config.frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = config.sampleShadingEnable;
    multisampling.rasterizationSamples = config.rasterizationSamples;
    multisampling.minSampleShading = config.minSampleShading;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // 深度/模板
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.depthTestEnable;
    depthStencil.depthWriteEnable = config.depthWriteEnable;
    depthStencil.depthCompareOp = config.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = config.stencilTestEnable;
    depthStencil.front = {};
    depthStencil.back = {};

    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = config.colorWriteMask;
    colorBlendAttachment.blendEnable = config.blendEnable;
    colorBlendAttachment.srcColorBlendFactor = config.srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = config.dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = config.colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = config.srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = config.dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = config.alphaBlendOp;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // 动态状态
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 管线创建
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_layout;
    pipelineInfo.renderPass = config.renderPass;
    pipelineInfo.subpass = config.subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(m_context->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        destroy();
        return Error(ErrorCode::InitializationFailed,
            "Failed to create graphics pipeline: " + std::to_string(static_cast<i32>(result)));
    }

    // 销毁着色器模块（管线已创建，不再需要）
    for (auto& shader : m_shaders) {
        if (shader.module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_context->device(), shader.module, nullptr);
            shader.module = VK_NULL_HANDLE;
        }
    }
    m_shaders.clear();

    spdlog::info("TridentPipeline created successfully");
    return {};
}

Result<void> TridentPipeline::createFromDesc(
    TridentContext* context, const api::PipelineDesc& desc, VkRenderPass renderPass)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    // 将 API 描述转换为 TridentPipelineConfig
    TridentPipelineConfig config;
    config.renderPass = renderPass;
    m_name = desc.name;
    m_renderState = desc.renderState;

    // 转换 BlendState
    const auto& blend = desc.renderState.blend;
    config.blendEnable = blend.enabled ? VK_TRUE : VK_FALSE;
    config.srcColorBlendFactor = toVulkanBlendFactor(blend.srcColor);
    config.dstColorBlendFactor = toVulkanBlendFactor(blend.dstColor);
    config.colorBlendOp = toVulkanBlendOp(blend.colorOp);
    config.srcAlphaBlendFactor = toVulkanBlendFactor(blend.srcAlpha);
    config.dstAlphaBlendFactor = toVulkanBlendFactor(blend.dstAlpha);
    config.alphaBlendOp = toVulkanBlendOp(blend.alphaOp);
    config.colorWriteMask = blend.colorWriteMask;

    // 转换 DepthState
    const auto& depth = desc.renderState.depth;
    config.depthTestEnable = depth.testEnabled ? VK_TRUE : VK_FALSE;
    config.depthWriteEnable = depth.writeEnabled ? VK_TRUE : VK_FALSE;
    config.depthCompareOp = toVulkanCompareOp(depth.compareOp);

    // 转换 RasterizerState
    const auto& rasterizer = desc.renderState.rasterizer;
    config.cullMode = toVulkanCullMode(rasterizer.cullMode);
    config.frontFace = toVulkanFrontFace(rasterizer.frontFace);
    config.polygonMode = toVulkanPolygonMode(rasterizer.polygonMode);

    // 顶点步长 - 设置默认顶点绑定
    if (desc.vertexStride > 0 && config.vertexBindings.empty()) {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = desc.vertexStride;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        config.vertexBindings.push_back(binding);
    }

    // 加载着色器
    for (const auto& shader : desc.shaders) {
        if (shader.bytecode.empty()) {
            continue;
        }

        auto moduleResult = createShaderModule(context, shader.bytecode);
        if (moduleResult.failed()) {
            destroy();
            return moduleResult.error();
        }

        TridentShaderModule shaderModule;
        shaderModule.module = moduleResult.value();
        shaderModule.stage = shader.stage == api::ShaderStage::Vertex
            ? VK_SHADER_STAGE_VERTEX_BIT
            : (shader.stage == api::ShaderStage::Fragment
                      ? VK_SHADER_STAGE_FRAGMENT_BIT
                      : (shader.stage == api::ShaderStage::Geometry ? VK_SHADER_STAGE_GEOMETRY_BIT
                                                                    : VK_SHADER_STAGE_VERTEX_BIT));
        shaderModule.entryPoint = shader.entryPoint;
        m_shaders.push_back(shaderModule);
    }

    return create(context, config);
}

void TridentPipeline::destroy()
{
    if (m_context == nullptr) return;

    for (auto& shader : m_shaders) {
        if (shader.module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_context->device(), shader.module, nullptr);
            shader.module = VK_NULL_HANDLE;
        }
    }
    m_shaders.clear();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_context->device(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_context->device(), m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }

    m_context = nullptr;
}

void TridentPipeline::bind(void* commandBuffer)
{
    vkCmdBindPipeline(static_cast<VkCommandBuffer>(commandBuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

Result<VkShaderModule> TridentPipeline::createShaderModule(TridentContext* context, const std::vector<u8>& code)
{
    if (code.size() % 4 != 0) {
        return Error(ErrorCode::InvalidData, "Shader code size must be a multiple of 4");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(context->device(), &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to create shader module: " + std::to_string(static_cast<i32>(result)));
    }

    return shaderModule;
}

Result<TridentShaderModule> TridentPipeline::_loadShader(const std::string& path, api::ShaderStage stage)
{
    return loadShaderFromFile(m_context, path, stage);
}

// ============================================================================
// TridentPipelineCache 实现
// ============================================================================

TridentPipelineCache::TridentPipelineCache() = default;

TridentPipelineCache::~TridentPipelineCache()
{
    destroy();
}

TridentPipelineCache::TridentPipelineCache(TridentPipelineCache&& other) noexcept
    : m_context(other.m_context)
    , m_cache(other.m_cache)
    , m_cachePath(std::move(other.m_cachePath))
{
    other.m_context = nullptr;
    other.m_cache = VK_NULL_HANDLE;
}

TridentPipelineCache& TridentPipelineCache::operator=(TridentPipelineCache&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_cache = other.m_cache;
        m_cachePath = std::move(other.m_cachePath);

        other.m_context = nullptr;
        other.m_cache = VK_NULL_HANDLE;
    }
    return *this;
}

Result<void> TridentPipelineCache::create(TridentContext* context, const std::string& cachePath)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    if (m_cache != VK_NULL_HANDLE) {
        destroy();
    }

    m_context = context;
    m_cachePath = cachePath;

    // 尝试从文件加载缓存
    std::vector<u8> cacheData;
    if (!cachePath.empty()) {
        std::ifstream file(cachePath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            size_t fileSize = static_cast<size_t>(file.tellg());
            cacheData.resize(fileSize);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(cacheData.data()), static_cast<std::streamsize>(fileSize));
            spdlog::info("Loaded pipeline cache from: {} ({} bytes)", cachePath, fileSize);
        }
    }

    VkPipelineCacheCreateInfo cacheInfo{};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cacheInfo.initialDataSize = cacheData.size();
    cacheInfo.pInitialData = cacheData.empty() ? nullptr : cacheData.data();

    VkResult result = vkCreatePipelineCache(context->device(), &cacheInfo, nullptr, &m_cache);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to create pipeline cache: " + std::to_string(static_cast<i32>(result)));
    }

    return {};
}

void TridentPipelineCache::destroy()
{
    if (m_context == nullptr) return;

    // 保存缓存到文件
    if (!m_cachePath.empty() && m_cache != VK_NULL_HANDLE) {
        size_t dataSize = 0;
        vkGetPipelineCacheData(m_context->device(), m_cache, &dataSize, nullptr);

        if (dataSize > 0) {
            std::vector<u8> cacheData(dataSize);
            vkGetPipelineCacheData(m_context->device(), m_cache, &dataSize, cacheData.data());

            std::ofstream file(m_cachePath, std::ios::binary);
            if (file.is_open()) {
                file.write(
                    reinterpret_cast<const char*>(cacheData.data()), static_cast<std::streamsize>(cacheData.size()));
                spdlog::info("Saved pipeline cache to: {} ({} bytes)", m_cachePath, cacheData.size());
            }
        }
    }

    if (m_cache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(m_context->device(), m_cache, nullptr);
        m_cache = VK_NULL_HANDLE;
    }

    m_context = nullptr;
}

} // namespace mc::client::renderer::trident
