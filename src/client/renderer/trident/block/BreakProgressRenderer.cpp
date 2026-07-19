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

#include "BreakProgressRenderer.hpp"
#include "BreakProgressManager.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

// ============================================================================
// 常量
// ============================================================================

namespace {
/// 破坏覆盖层立方体顶点数（6面 * 4顶点）
constexpr size_t VERTICES_PER_CUBE = 24;

/// 破坏覆盖层立方体索引数（6面 * 2三角形 * 3索引）
constexpr size_t INDICES_PER_CUBE = 36;

/// 默认缓冲区容量
constexpr size_t DEFAULT_MAX_CUBES = 64;
} // namespace

// ============================================================================
// 构造/析构
// ============================================================================

BreakProgressRenderer::~BreakProgressRenderer()
{
    cleanup();
}

// ============================================================================
// 初始化
// ============================================================================

bool BreakProgressRenderer::initialize(const Config& config, VkSampleCountFlagBits sampleCount)
{
    if (m_initialized) {
        return true;
    }

    spdlog::info("BreakProgressRenderer: Initializing...");

    m_config = config;

    if (m_config.device == VK_NULL_HANDLE) {
        spdlog::error("BreakProgressRenderer: Device is null");
        return false;
    }

    // 创建管线
    if (!_createPipeline(sampleCount)) {
        spdlog::error("BreakProgressRenderer: Failed to create pipeline");
        return false;
    }

    // 创建缓冲区
    if (!_createBuffers()) {
        spdlog::error("BreakProgressRenderer: Failed to create buffers");
        return false;
    }

    // 创建描述符集
    if (!_createDescriptorSets()) {
        spdlog::error("BreakProgressRenderer: Failed to create descriptor sets");
        return false;
    }

    // 纹理由 setBlockAtlas / setStageRegionLookup 在 AtlasManager 加载 blocks atlas 后注入。
    // 若此时已注入 blocks atlas 句柄则立即写入描述符。
    if (m_blockImageView != VK_NULL_HANDLE && m_blockSampler != VK_NULL_HANDLE) {
        _writeBlockAtlasDescriptor(m_blockImageView, m_blockSampler);
    }

    m_initialized = true;
    spdlog::info("BreakProgressRenderer: Initialized successfully");
    return true;
}

void BreakProgressRenderer::cleanup()
{
    if (!m_initialized) {
        return;
    }

    VkDevice device = m_config.device;

    // 等待设备空闲
    vkDeviceWaitIdle(device);

    // 顶点缓冲区
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
    vkFreeMemory(device, m_vertexBufferMemory, nullptr);
    m_vertexBufferMemory = VK_NULL_HANDLE;

    // 索引缓冲区
    vkDestroyBuffer(device, m_indexBuffer, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
    vkFreeMemory(device, m_indexBufferMemory, nullptr);
    m_indexBufferMemory = VK_NULL_HANDLE;

    // 描述符（纹理图集 view/sampler 由 AtlasManager 拥有，不在此释放）
    m_descriptorSet = VK_NULL_HANDLE;
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;
    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
    m_descriptorSetLayout = VK_NULL_HANDLE;

    // 管线
    vkDestroyPipeline(device, m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;

    // 清理注入的引用（不释放 GPU 资源，所有权归 AtlasManager）
    m_blockImageView = VK_NULL_HANDLE;
    m_blockSampler = VK_NULL_HANDLE;
    m_stageRegionLookup = nullptr;

    m_initialized = false;
    spdlog::info("BreakProgressRenderer: Cleaned up");
}

// ============================================================================
// 更新
// ============================================================================

void BreakProgressRenderer::updateMesh(const Vector3& cameraPos)
{
    // 获取可见的破坏进度（使用预分配缓冲区避免内存分配）
    auto& manager = BreakProgressManager::instance();
    m_progressEntries.clear();

    // 预分配缓冲区已准备好
    manager.getVisibleProgress(cameraPos, m_progressBuffer);

    // 转换 pair 格式到 ProgressEntry 格式，并计算偏移量
    m_progressEntries.reserve(m_progressBuffer.size());
    for (size_t i = 0; i < m_progressBuffer.size(); ++i) {
        const auto& [pos, stage] = m_progressBuffer[i];
        m_progressEntries.push_back({
            pos,
            stage,
            static_cast<u32>(i * VERTICES_PER_CUBE), // vertexOffset
            static_cast<u32>(i * INDICES_PER_CUBE)   // indexOffset
        });
    }

    // 如果没有进度，直接返回
    if (m_progressEntries.empty()) {
        m_vertexCount = 0;
        m_indexCount = 0;
        return;
    }

    // 计算所需缓冲区大小
    size_t requiredVertices = m_progressEntries.size() * VERTICES_PER_CUBE;
    size_t requiredIndices = m_progressEntries.size() * INDICES_PER_CUBE;

    // 确保缓冲区容量足够
    if (!_ensureBufferCapacity(requiredVertices, requiredIndices)) {
        spdlog::error("BreakProgressRenderer: Failed to ensure buffer capacity");
        return;
    }

    // 生成顶点数据（局部坐标 + 按破坏阶段烘焙的 UV）
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    vertices.reserve(requiredVertices);
    indices.reserve(requiredIndices);

    // UV 兜底区域：regionLookup 未注入或查不到时用全区域（采样结果会是某个实际 sprite 或
    // blocks atlas 左上角像素，仍可渲染，不会越界）。
    const TextureRegion fallbackRegion(0.0, 0.0, 1.0, 1.0);

    for (size_t i = 0; i < m_progressEntries.size(); ++i) {
        const u8 stage = m_progressEntries[i].stage;
        const TextureRegion* stageRegion = nullptr;
        if (m_stageRegionLookup) {
            stageRegion = m_stageRegionLookup(stage);
        }
        const TextureRegion& region = (stageRegion != nullptr) ? *stageRegion : fallbackRegion;
        _generateCubeMesh(i, region, vertices, indices);
    }

    // 更新缓冲区
    _updateVertexBuffer(vertices);
    _updateIndexBuffer(indices);

    m_vertexCount = vertices.size();
    m_indexCount = indices.size();
}

// ============================================================================
// 渲染
// ============================================================================

void BreakProgressRenderer::render(
    VkCommandBuffer commandBuffer, VkDescriptorSet cameraDescriptorSet, VkDescriptorSet fogDescriptorSet)
{
    if (!m_initialized || m_progressEntries.empty()) {
        return;
    }

    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // 绑定描述符集
    // Set 0: Camera UBO
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

    // Set 1: 纹理图集
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &m_descriptorSet, 0, nullptr);

    // Set 2: Fog UBO
    if (fogDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1, &fogDescriptorSet, 0, nullptr);
    }

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // 绑定索引缓冲区
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 推送常量结构体（与着色器匹配）
    // vec3 blockPos + pad → 16 字节。破坏阶段 UV 已烘进顶点，不再传 damageStage。
    struct PushConstants {
        f32 blockPosX, blockPosY, blockPosZ;
        f32 pad;
    };

    // 为每个破坏进度设置推送常量并绘制
    for (const auto& entry : m_progressEntries) {
        PushConstants pc;
        pc.blockPosX = static_cast<f32>(entry.position.x);
        pc.blockPosY = static_cast<f32>(entry.position.y);
        pc.blockPosZ = static_cast<f32>(entry.position.z);
        pc.pad = 0.0f;

        vkCmdPushConstants(commandBuffer,
            m_pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &pc);

        // 绘制单个方块的破坏覆盖层
        vkCmdDrawIndexed(commandBuffer,
            static_cast<u32>(INDICES_PER_CUBE),
            1,
            entry.indexOffset,
            static_cast<i32>(entry.vertexOffset),
            0);
    }
}

// ============================================================================
// 私有方法 - 资源创建
// ============================================================================

bool BreakProgressRenderer::_createPipeline(VkSampleCountFlagBits sampleCount)
{
    // 加载着色器
    auto vertPath = resolveShaderPath("break_overlay.vert.spv");
    auto fragPath = resolveShaderPath("break_overlay.frag.spv");

    if (vertPath.empty() || fragPath.empty()) {
        spdlog::error("BreakProgressRenderer: Failed to resolve shader paths");
        return false;
    }

    // 读取着色器代码
    std::vector<char> vertCode, fragCode;

    std::ifstream vertFile(vertPath, std::ios::binary | std::ios::ate);
    if (!vertFile.is_open()) {
        spdlog::error("BreakProgressRenderer: Failed to open vertex shader: {}", vertPath.string());
        return false;
    }
    size_t vertSize = vertFile.tellg();
    vertFile.seekg(0);
    vertCode.resize(vertSize);
    vertFile.read(vertCode.data(), vertSize);
    vertFile.close();

    std::ifstream fragFile(fragPath, std::ios::binary | std::ios::ate);
    if (!fragFile.is_open()) {
        spdlog::error("BreakProgressRenderer: Failed to open fragment shader: {}", fragPath.string());
        return false;
    }
    size_t fragSize = fragFile.tellg();
    fragFile.seekg(0);
    fragCode.resize(fragSize);
    fragFile.read(fragCode.data(), fragSize);
    fragFile.close();

    // 创建着色器模块
    VkShaderModule vertModule, fragModule;

    VkShaderModuleCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertCreateInfo.codeSize = vertCode.size();
    vertCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());

    if (vkCreateShaderModule(m_config.device, &vertCreateInfo, nullptr, &vertModule) != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to create vertex shader module");
        return false;
    }

    VkShaderModuleCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragCreateInfo.codeSize = fragCode.size();
    fragCreateInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());

    if (vkCreateShaderModule(m_config.device, &fragCreateInfo, nullptr, &fragModule) != VK_SUCCESS) {
        vkDestroyShaderModule(m_config.device, vertModule, nullptr);
        spdlog::error("BreakProgressRenderer: Failed to create fragment shader module");
        return false;
    }

    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    // 顶点输入描述
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // 位置属性
    VkVertexInputAttributeDescription positionAttr{};
    positionAttr.binding = 0;
    positionAttr.location = 0;
    positionAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttr.offset = offsetof(Vertex, x);

    // UV属性
    VkVertexInputAttributeDescription texCoordAttr{};
    texCoordAttr.binding = 0;
    texCoordAttr.location = 1;
    texCoordAttr.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordAttr.offset = offsetof(Vertex, u);

    VkVertexInputAttributeDescription attributeDescriptions[] = {positionAttr, texCoordAttr};

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions;

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态设置）
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    // 禁用面剔除 - 破坏覆盖层需要渲染所有面
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // 深度偏移：让破坏覆盖层渲染在方块表面之前
    // OpenGL factor -> Vulkan slopeFactor, OpenGL units -> Vulkan constantFactor
    // 注意：Vulkan 中 constantFactor 单位是 r（最小深度变化），需要乘以适当值
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = -0.01f; // 恒定偏移，单位是深度缓冲精度
    rasterizer.depthBiasSlopeFactor = -1.0f;     // 斜率相关偏移
    rasterizer.depthBiasClamp = 0.0f;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;

    // 深度模板
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 混合 - 叠加混合模式：DST_COLOR * SRC_COLOR
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 推送常量
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(f32) * 4; // vec3 blockPos + pad（破坏阶段 UV 已烘进顶点）

    // 创建纹理描述符布局
    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureBinding;

    if (vkCreateDescriptorSetLayout(m_config.device, &textureLayoutInfo, nullptr, &m_descriptorSetLayout) !=
        VK_SUCCESS) {
        vkDestroyShaderModule(m_config.device, vertModule, nullptr);
        vkDestroyShaderModule(m_config.device, fragModule, nullptr);
        spdlog::error("BreakProgressRenderer: Failed to create descriptor set layout");
        return false;
    }

    // 管线布局描述符集
    VkDescriptorSetLayout layouts[3] = {m_config.cameraLayout, m_descriptorSetLayout, m_config.fogLayout};

    // 管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_config.device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(m_config.device, m_descriptorSetLayout, nullptr);
        vkDestroyShaderModule(m_config.device, vertModule, nullptr);
        vkDestroyShaderModule(m_config.device, fragModule, nullptr);
        spdlog::error("BreakProgressRenderer: Failed to create pipeline layout");
        return false;
    }

    // 动态状态
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 创建图形管线
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_config.renderPass;
    pipelineInfo.subpass = 0;

    VkResult result =
        vkCreateGraphicsPipelines(m_config.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // 清理着色器模块
    vkDestroyShaderModule(m_config.device, vertModule, nullptr);
    vkDestroyShaderModule(m_config.device, fragModule, nullptr);

    if (result != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to create graphics pipeline");
        return false;
    }

    spdlog::info("BreakProgressRenderer: Pipeline created successfully");
    return true;
}

bool BreakProgressRenderer::_createBuffers()
{
    m_maxVertices = DEFAULT_MAX_CUBES * VERTICES_PER_CUBE;
    m_maxIndices = DEFAULT_MAX_CUBES * INDICES_PER_CUBE;

    // 使用 VulkanUtils 创建顶点缓冲区
    auto vertexResult = ::mc::client::renderer::VulkanUtils::createBuffer(m_config.device,
        m_config.physicalDevice,
        m_maxVertices * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory);

    if (!vertexResult.success()) {
        spdlog::error("BreakProgressRenderer: Failed to create vertex buffer: {}", vertexResult.error().message());
        return false;
    }

    // 使用 VulkanUtils 创建索引缓冲区
    auto indexResult = ::mc::client::renderer::VulkanUtils::createBuffer(m_config.device,
        m_config.physicalDevice,
        m_maxIndices * sizeof(u32),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_indexBuffer,
        m_indexBufferMemory);

    if (!indexResult.success()) {
        spdlog::error("BreakProgressRenderer: Failed to create index buffer: {}", indexResult.error().message());
        // 清理已创建的顶点缓冲区
        vkDestroyBuffer(m_config.device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_config.device, m_vertexBufferMemory, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
        m_vertexBufferMemory = VK_NULL_HANDLE;
        return false;
    }

    spdlog::info("BreakProgressRenderer: Buffers created (vertices: {}, indices: {})", m_maxVertices, m_maxIndices);
    return true;
}

bool BreakProgressRenderer::_createDescriptorSets()
{
    // 创建描述符池
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_config.device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to create descriptor pool");
        return false;
    }

    // 分配描述符集
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkAllocateDescriptorSets(m_config.device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        spdlog::error("BreakProgressRenderer: Failed to allocate descriptor set");
        return false;
    }

    spdlog::info("BreakProgressRenderer: Descriptor sets created");
    return true;
}

// ============================================================================
// blocks atlas 注入
// ============================================================================

void BreakProgressRenderer::setBlockAtlas(VkImageView imageView, VkSampler sampler)
{
    m_blockImageView = imageView;
    m_blockSampler = sampler;

    // 若已初始化则立即（重）写描述符；否则在 initialize 末尾按已注入句柄写入。
    if (m_initialized && m_descriptorSet != VK_NULL_HANDLE && imageView != VK_NULL_HANDLE &&
        sampler != VK_NULL_HANDLE) {
        _writeBlockAtlasDescriptor(imageView, sampler);
    }
}

void BreakProgressRenderer::_writeBlockAtlasDescriptor(VkImageView imageView, VkSampler sampler)
{
    if (m_descriptorSet == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = imageView;
    descriptorImageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descriptorImageInfo;

    vkUpdateDescriptorSets(m_config.device, 1, &descriptorWrite, 0, nullptr);
}

// ============================================================================
// 私有方法 - 网格生成
// ============================================================================

void BreakProgressRenderer::_generateCubeMesh(
    size_t cubeIndex, const TextureRegion& stageRegion, std::vector<Vertex>& vertices, std::vector<u32>& indices)
{
    // 生成立方体顶点（局部坐标 0-1 范围）
    // 方块位置通过 push constants 传入着色器
    // 立方体稍微放大以避免 z-fighting
    constexpr f32 EXPAND = 0.005f;
    constexpr f32 x0 = -EXPAND, y0 = -EXPAND, z0 = -EXPAND;
    constexpr f32 x1 = 1.0f + EXPAND, y1 = 1.0f + EXPAND, z1 = 1.0f + EXPAND;

    // 把该破坏阶段在 blocks atlas 中的 UV 区域映射到每面 0..1 局部 UV。
    const f32 u0 = static_cast<f32>(stageRegion.u0);
    const f32 u1 = static_cast<f32>(stageRegion.u1);
    const f32 v0 = static_cast<f32>(stageRegion.v0);
    const f32 v1 = static_cast<f32>(stageRegion.v1);

    // 局部 UV (lu, lv) ∈ {0,1} → 绝对 UV = u0 + lu*(u1-u0), v0 + lv*(v1-v0)
    // 6个面，每面4个顶点。UV 顺序与原 0..1 布局一致（保持朝向不变）。
    // 底面 (y = y0)
    vertices.push_back({x0, y0, z0, u0, v0});
    vertices.push_back({x1, y0, z0, u1, v0});
    vertices.push_back({x1, y0, z1, u1, v1});
    vertices.push_back({x0, y0, z1, u0, v1});

    // 顶面 (y = y1)
    vertices.push_back({x0, y1, z0, u0, v0});
    vertices.push_back({x0, y1, z1, u1, v0});
    vertices.push_back({x1, y1, z1, u1, v1});
    vertices.push_back({x1, y1, z0, u0, v1});

    // 前面 (z = z1)
    vertices.push_back({x0, y0, z1, u0, v0});
    vertices.push_back({x1, y0, z1, u1, v0});
    vertices.push_back({x1, y1, z1, u1, v1});
    vertices.push_back({x0, y1, z1, u0, v1});

    // 后面 (z = z0)
    vertices.push_back({x1, y0, z0, u0, v0});
    vertices.push_back({x0, y0, z0, u1, v0});
    vertices.push_back({x0, y1, z0, u1, v1});
    vertices.push_back({x1, y1, z0, u0, v1});

    // 右面 (x = x1)
    vertices.push_back({x1, y0, z1, u0, v0});
    vertices.push_back({x1, y0, z0, u1, v0});
    vertices.push_back({x1, y1, z0, u1, v1});
    vertices.push_back({x1, y1, z1, u0, v1});

    // 左面 (x = x0)
    vertices.push_back({x0, y0, z0, u0, v0});
    vertices.push_back({x0, y0, z1, u1, v0});
    vertices.push_back({x0, y1, z1, u1, v1});
    vertices.push_back({x0, y1, z0, u0, v1});

    // 每面6个索引（2个三角形）
    u32 baseVertex = static_cast<u32>(cubeIndex * VERTICES_PER_CUBE);
    for (u32 i = 0; i < 6; ++i) {
        u32 base = baseVertex + i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
}

void BreakProgressRenderer::_updateVertexBuffer(const std::vector<Vertex>& vertices)
{
    if (vertices.empty() || m_vertexBuffer == VK_NULL_HANDLE) {
        return;
    }

    void* data = nullptr;
    VkDeviceSize size = std::min(vertices.size(), m_maxVertices) * sizeof(Vertex);
    const VkResult mapResult = vkMapMemory(m_config.device, m_vertexBufferMemory, 0, size, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        spdlog::error("BreakProgressRenderer: Failed to map vertex buffer memory: {}", static_cast<i32>(mapResult));
        return;
    }
    std::memcpy(data, vertices.data(), size);
    vkUnmapMemory(m_config.device, m_vertexBufferMemory);
}

void BreakProgressRenderer::_updateIndexBuffer(const std::vector<u32>& indices)
{
    if (indices.empty() || m_indexBuffer == VK_NULL_HANDLE) {
        return;
    }

    void* data = nullptr;
    VkDeviceSize size = std::min(indices.size(), m_maxIndices) * sizeof(u32);
    const VkResult mapResult = vkMapMemory(m_config.device, m_indexBufferMemory, 0, size, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        spdlog::error("BreakProgressRenderer: Failed to map index buffer memory: {}", static_cast<i32>(mapResult));
        return;
    }
    std::memcpy(data, indices.data(), size);
    vkUnmapMemory(m_config.device, m_indexBufferMemory);
}

bool BreakProgressRenderer::_ensureBufferCapacity(size_t requiredVertices, size_t requiredIndices)
{
    // 如果容量足够，直接返回
    if (requiredVertices <= m_maxVertices && requiredIndices <= m_maxIndices) {
        return true;
    }

    // 计算新的容量（扩大1.5倍或至少满足需求）
    size_t newVertexCount = std::max(requiredVertices, m_maxVertices * 3 / 2);
    size_t newIndexCount = std::max(requiredIndices, m_maxIndices * 3 / 2);

    // 设置最小容量
    newVertexCount = std::max(newVertexCount, DEFAULT_MAX_CUBES * VERTICES_PER_CUBE);
    newIndexCount = std::max(newIndexCount, DEFAULT_MAX_CUBES * INDICES_PER_CUBE);

    return _recreateBuffers(newVertexCount, newIndexCount);
}

bool BreakProgressRenderer::_recreateBuffers(size_t vertexCount, size_t indexCount)
{
    VkDevice device = m_config.device;

    // 等待设备空闲
    vkDeviceWaitIdle(device);

    // 销毁旧缓冲区
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
    vkFreeMemory(device, m_vertexBufferMemory, nullptr);
    m_vertexBufferMemory = VK_NULL_HANDLE;
    vkDestroyBuffer(device, m_indexBuffer, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
    vkFreeMemory(device, m_indexBufferMemory, nullptr);
    m_indexBufferMemory = VK_NULL_HANDLE;

    // 使用 VulkanUtils 创建顶点缓冲区
    auto vertexResult = ::mc::client::renderer::VulkanUtils::createBuffer(device,
        m_config.physicalDevice,
        vertexCount * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory);

    if (!vertexResult.success()) {
        spdlog::error("BreakProgressRenderer: Failed to recreate vertex buffer: {}", vertexResult.error().message());
        return false;
    }

    // 使用 VulkanUtils 创建索引缓冲区
    auto indexResult = ::mc::client::renderer::VulkanUtils::createBuffer(device,
        m_config.physicalDevice,
        indexCount * sizeof(u32),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_indexBuffer,
        m_indexBufferMemory);

    if (!indexResult.success()) {
        spdlog::error("BreakProgressRenderer: Failed to recreate index buffer: {}", indexResult.error().message());
        return false;
    }

    m_maxVertices = vertexCount;
    m_maxIndices = indexCount;

    return true;
}

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
