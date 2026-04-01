#include "EntityPipeline.hpp"
#include "../util/VulkanUtils.hpp"
#include "../../util/ShaderPath.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <array>
#include <fstream>

namespace mc::client {

// 辅助函数：从文件读取着色器
static std::vector<u8> readShaderFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    const std::streamsize fileSize = file.tellg();
    std::vector<u8> data(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    return data;
}

// 辅助函数：创建着色器模块
static Result<VkShaderModule> createShaderModule(VkDevice device, const std::vector<u8>& code) {
    if (code.empty() || code.size() % 4 != 0) {
        return Error(ErrorCode::InvalidData, "Invalid shader code");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create shader module");
    }

    return shaderModule;
}

// ============================================================================
// EntityPipeline
// ============================================================================

EntityPipeline::EntityPipeline() = default;

EntityPipeline::~EntityPipeline() {
    destroy();
}

VkVertexInputBindingDescription EntityPipeline::getVertexBindingDescription() {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(renderer::ModelVertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

std::vector<VkVertexInputAttributeDescription> EntityPipeline::getVertexAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> descs(3);

    // 位置
    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[0].offset = offsetof(renderer::ModelVertex, position);

    // 纹理坐标
    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    descs[1].offset = offsetof(renderer::ModelVertex, texCoord);

    // 法线
    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[2].offset = offsetof(renderer::ModelVertex, normal);

    return descs;
}

Result<void> EntityPipeline::initialize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkDescriptorSetLayout cameraDescriptorLayout,
    VkDescriptorPool descriptorPool,
    VkCommandPool commandPool) {
    if (m_initialized) {
        return Result<void>::ok();
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_graphicsQueue = graphicsQueue;
    m_descriptorPool = descriptorPool;
    m_commandPool = commandPool;

    // 创建描述符布局
    auto result = createDescriptorLayouts();
    if (!result.success()) {
        return result.error();
    }

    // 创建纹理采样器
    result = createTextureSampler();
    if (!result.success()) {
        return result.error();
    }

    // 创建描述符集
    result = createDescriptorSets();
    if (!result.success()) {
        return result.error();
    }

    // 创建图形管线
    result = createGraphicsPipeline(renderPass, cameraDescriptorLayout);
    if (!result.success()) {
        return result.error();
    }

    m_initialized = true;
    spdlog::info("EntityPipeline initialized");
    return Result<void>::ok();
}

void EntityPipeline::destroy() {
    if (!m_initialized) {
        return;
    }

    // 销毁管线
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    // 销毁管线布局
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // 销毁采样器
    if (m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }

    // 销毁描述符布局
    if (m_textureDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_textureDescriptorLayout, nullptr);
        m_textureDescriptorLayout = VK_NULL_HANDLE;
    }

    m_initialized = false;
    spdlog::info("EntityPipeline destroyed");
}

void EntityPipeline::bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

Result<EntityMesh> EntityPipeline::createMesh(const std::vector<renderer::ModelVertex>& vertices,
                                               const std::vector<u32>& indices) {
    EntityMesh mesh;
    mesh.vertexCount = static_cast<u32>(vertices.size());
    mesh.indexCount = static_cast<u32>(indices.size());

    if (vertices.empty() || indices.empty()) {
        return mesh;  // 隐式转换为Result<EntityMesh>
    }

    VkDevice device = m_device;

    // 创建顶点缓冲区
    VkDeviceSize vertexBufferSize = sizeof(renderer::ModelVertex) * vertices.size();
    VkBuffer vertexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexStagingMemory = VK_NULL_HANDLE;

    // 创建暂存缓冲区
    auto result = createBuffer(vertexBufferSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               vertexStagingBuffer, vertexStagingMemory);
    if (!result.success()) {
        return result.error();
    }

    // 填充暂存缓冲区
    void* data;
    vkMapMemory(device, vertexStagingMemory, 0, vertexBufferSize, 0, &data);
    std::memcpy(data, vertices.data(), vertexBufferSize);
    vkUnmapMemory(device, vertexStagingMemory);

    // 创建设备本地缓冲区
    result = createBuffer(vertexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          mesh.vertexBuffer, mesh.vertexMemory);
    if (!result.success()) {
        vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
        vkFreeMemory(device, vertexStagingMemory, nullptr);
        return result.error();
    }

    // 复制到设备本地缓冲区
    copyBuffer(vertexStagingBuffer, mesh.vertexBuffer, vertexBufferSize);

    // 清理暂存缓冲区
    vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
    vkFreeMemory(device, vertexStagingMemory, nullptr);

    // 创建索引缓冲区
    VkDeviceSize indexBufferSize = sizeof(u32) * indices.size();
    VkBuffer indexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexStagingMemory = VK_NULL_HANDLE;

    result = createBuffer(indexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          indexStagingBuffer, indexStagingMemory);
    if (!result.success()) {
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(device, mesh.vertexMemory, nullptr);
        return result.error();
    }

    // 填充暂存缓冲区
    vkMapMemory(device, indexStagingMemory, 0, indexBufferSize, 0, &data);
    std::memcpy(data, indices.data(), indexBufferSize);
    vkUnmapMemory(device, indexStagingMemory);

    // 创建设备本地缓冲区
    result = createBuffer(indexBufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          mesh.indexBuffer, mesh.indexMemory);
    if (!result.success()) {
        vkDestroyBuffer(device, indexStagingBuffer, nullptr);
        vkFreeMemory(device, indexStagingMemory, nullptr);
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(device, mesh.vertexMemory, nullptr);
        return result.error();
    }

    // 复制到设备本地缓冲区
    copyBuffer(indexStagingBuffer, mesh.indexBuffer, indexBufferSize);

    // 清理暂存缓冲区
    vkDestroyBuffer(device, indexStagingBuffer, nullptr);
    vkFreeMemory(device, indexStagingMemory, nullptr);

    return mesh;  // 隐式转换为Result<EntityMesh>
}

Result<void> EntityPipeline::updateMesh(EntityMesh& mesh,
                                        const std::vector<renderer::ModelVertex>& vertices,
                                        const std::vector<u32>& indices) {
    // 销毁旧的缓冲区
    destroyMesh(mesh);

    // 创建新的缓冲区
    auto result = createMesh(vertices, indices);
    if (!result.success()) {
        return result.error();
    }

    // 复制新数据
    EntityMesh newMesh = std::move(result.value());
    mesh.vertexBuffer = newMesh.vertexBuffer;
    mesh.vertexMemory = newMesh.vertexMemory;
    mesh.indexBuffer = newMesh.indexBuffer;
    mesh.indexMemory = newMesh.indexMemory;
    mesh.vertexCount = newMesh.vertexCount;
    mesh.indexCount = newMesh.indexCount;

    return Result<void>::ok();
}

void EntityPipeline::destroyMesh(EntityMesh& mesh) {
    VkDevice device = m_device;

    if (mesh.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        mesh.vertexBuffer = VK_NULL_HANDLE;
    }

    if (mesh.vertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh.vertexMemory, nullptr);
        mesh.vertexMemory = VK_NULL_HANDLE;
    }

    if (mesh.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
        mesh.indexBuffer = VK_NULL_HANDLE;
    }

    if (mesh.indexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mesh.indexMemory, nullptr);
        mesh.indexMemory = VK_NULL_HANDLE;
    }

    mesh.vertexCount = 0;
    mesh.indexCount = 0;
}

void EntityPipeline::drawMesh(VkCommandBuffer cmd,
                               const EntityMesh& mesh,
                               const std::array<f64, 16>& modelMatrix,
                               const Vector3f& position,
                               f64 scale) {
    if (mesh.vertexCount == 0 || mesh.indexCount == 0) {
        return;
    }

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 绑定索引缓冲区
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 推送常量
    struct PushConstants {
        std::array<f64, 16> model;
        f64 posX, posY, posZ;
        f64 scale;
    } pc;

    pc.model = modelMatrix;
    pc.posX = position.x;
    pc.posY = position.y;
    pc.posZ = position.z;
    pc.scale = scale;

    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(PushConstants), &pc);

    // 绘制
    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
}

void EntityPipeline::bindTextureDescriptor(VkCommandBuffer cmd) {
    if (m_textureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pipelineLayout, 1, 1, &m_textureDescriptorSet, 0, nullptr);
    }
}

void EntityPipeline::setTextureAtlas(VkImageView textureView, VkSampler sampler) {
    if (m_textureDescriptorSet == VK_NULL_HANDLE || textureView == VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_device;

    // 更新描述符集
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureView;
    imageInfo.sampler = sampler != VK_NULL_HANDLE ? sampler : m_textureSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_textureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

VkPipelineLayout EntityPipeline::pipelineLayout() const {
    return m_pipelineLayout;
}

Result<void> EntityPipeline::createDescriptorLayouts() {
    VkDevice device = m_device;

    // 纹理采样器描述符布局（绑定到 set 1）
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                   &m_textureDescriptorLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to create texture descriptor layout");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createTextureSampler() {
    VkDevice device = m_device;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // 实体使用最近邻过滤以保持像素风格
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_textureSampler);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to create texture sampler");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createDescriptorSets() {
    VkDevice device = m_device;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_textureDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &m_textureDescriptorSet);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to allocate texture descriptor set");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createGraphicsPipeline(VkRenderPass renderPass,
                                                      VkDescriptorSetLayout cameraDescriptorLayout) {
    // 着色器路径
    const auto vertPath = resolveShaderPath("entity.vert.spv");
    const auto fragPath = resolveShaderPath("entity.frag.spv");
    if (vertPath.empty() || fragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve entity shader binaries");
    }

    // 加载着色器
    auto vertCode = readShaderFile(vertPath);
    auto fragCode = readShaderFile(fragPath);
    if (vertCode.empty() || fragCode.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to load entity shaders");
    }

    auto vertModuleResult = createShaderModule(m_device, vertCode);
    if (!vertModuleResult.success()) {
        return vertModuleResult.error();
    }
    VkShaderModule vertShaderModule = vertModuleResult.value();

    auto fragModuleResult = createShaderModule(m_device, fragCode);
    if (!fragModuleResult.success()) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        return fragModuleResult.error();
    }
    VkShaderModule fragShaderModule = fragModuleResult.value();

    // 着色器阶段
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";

    // 顶点输入
    auto bindingDesc = getVertexBindingDescription();
    auto attrDescs = getVertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attrDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态）
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
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // 实体模型禁用剔除
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 深度/模板
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 颜色混合 - 启用alpha混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 动态状态
    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 管线布局
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
        cameraDescriptorLayout,
        m_textureDescriptorLayout
    };

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(f64) * 16 + sizeof(f64) * 4;  // mat4 + vec3 + float

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create pipeline layout");
    }

    // 创建图形管线
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
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // 清理着色器模块
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
        return Error(ErrorCode::InitializationFailed, "Failed to create graphics pipeline");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::createBuffer(VkDeviceSize size,
                                           VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags properties,
                                           VkBuffer& buffer,
                                           VkDeviceMemory& memory) {
    return renderer::VulkanUtils::createBuffer(m_device, m_physicalDevice, size, usage, properties, buffer, memory);
}

void EntityPipeline::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(cmd);
}

VkCommandBuffer EntityPipeline::beginSingleTimeCommands() {
    return renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void EntityPipeline::endSingleTimeCommands(VkCommandBuffer cmd) {
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);
}

} // namespace mc::client
