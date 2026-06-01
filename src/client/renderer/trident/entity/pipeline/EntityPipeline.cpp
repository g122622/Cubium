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

#include "EntityPipeline.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include "common/util/math/Vector4.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::pipeline {

// 导入 ModelVertex 类型
using model::ModelVertex;

namespace {

// 从文件读取着色器
std::vector<u8> readShaderFile(const std::filesystem::path& path)
{
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

// 创建着色器模块
Result<VkShaderModule> createShaderModule(VkDevice device, const std::vector<u8>& code)
{
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

u32 growCapacity(u32 currentCapacity, u32 requiredCapacity)
{
    if (currentCapacity >= requiredCapacity) {
        return currentCapacity;
    }

    if (currentCapacity == 0) {
        return requiredCapacity;
    }

    const u32 grownCapacity = currentCapacity + currentCapacity / 2;
    return std::max(grownCapacity, requiredCapacity);
}

} // namespace

// ============================================================================
// EntityPipeline
// ============================================================================

EntityPipeline::EntityPipeline() = default;

EntityPipeline::~EntityPipeline()
{
    destroy();
}

VkVertexInputBindingDescription EntityPipeline::getVertexBindingDescription()
{
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(ModelVertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

std::vector<VkVertexInputAttributeDescription> EntityPipeline::getVertexAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> descs(3);

    // 位置
    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[0].offset = offsetof(ModelVertex, position);

    // 纹理坐标
    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    descs[1].offset = offsetof(ModelVertex, texCoord);

    // 法线
    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs[2].offset = offsetof(ModelVertex, normal);

    return descs;
}

Result<void> EntityPipeline::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkDescriptorSetLayout cameraDescriptorLayout,
    VkDescriptorPool descriptorPool,
    VkCommandPool commandPool,
    VkSampleCountFlagBits sampleCount)
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    m_device = device;
    m_physicalDevice = physicalDevice;

    destroy();
    m_graphicsQueue = graphicsQueue;
    m_descriptorPool = descriptorPool;
    m_commandPool = commandPool;

    // 创建描述符布局
    auto result = _createDescriptorLayouts();
    if (!result.success()) {
        destroy();
        return result.error();
    }

    // 创建纹理采样器
    result = _createTextureSampler();
    if (!result.success()) {
        destroy();
        return result.error();
    }

    // 创建描述符集
    result = _createDescriptorSets();
    if (!result.success()) {
        destroy();
        return result.error();
    }

    // 创建图形管线
    result = _createGraphicsPipeline(renderPass, cameraDescriptorLayout, sampleCount);
    if (!result.success()) {
        destroy();
        return result.error();
    }

    m_initialized = true;
    spdlog::info("EntityPipeline initialized");
    return Result<void>::ok();
}

void EntityPipeline::destroy()
{
    const bool hadResources = m_initialized || m_pipeline != VK_NULL_HANDLE ||
        m_additiveBlendPipeline != VK_NULL_HANDLE || m_linePipeline != VK_NULL_HANDLE ||
        m_pipelineLayout != VK_NULL_HANDLE ||
        m_textureSampler != VK_NULL_HANDLE || m_textureDescriptorLayout != VK_NULL_HANDLE ||
        m_vertexStagingBuffer != VK_NULL_HANDLE || m_indexStagingBuffer != VK_NULL_HANDLE;

    // 销毁管线
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    // 销毁叠加混合管线
    if (m_additiveBlendPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_additiveBlendPipeline, nullptr);
        m_additiveBlendPipeline = VK_NULL_HANDLE;
    }

    // 销毁线段渲染管线
    if (m_linePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_linePipeline, nullptr);
        m_linePipeline = VK_NULL_HANDLE;
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

    _destroyReusableStagingBuffers();

    m_initialized = false;

    if (hadResources) {
        spdlog::info("EntityPipeline destroyed");
    }
}

void EntityPipeline::bind(VkCommandBuffer cmd, BlendMode blendMode)
{
    // 根据混合模式选择管线
    VkPipeline pipelineToBind = m_pipeline;
    switch (blendMode) {
        case BlendMode::Additive:
            pipelineToBind = m_additiveBlendPipeline;
            break;
        case BlendMode::Lines:
            pipelineToBind = m_linePipeline;
            break;
        case BlendMode::Alpha:
        case BlendMode::None:
        case BlendMode::Multiply:
        default:
            // TODO: BlendMode::Multiply 和 BlendMode::None 尚未实现专用管线，当前回退到Alpha混合
            pipelineToBind = m_pipeline;
            break;
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToBind);
}

Result<EntityMesh> EntityPipeline::createMesh(const std::vector<ModelVertex>& vertices, const std::vector<u32>& indices)
{
    EntityMesh mesh;
    mesh.vertexCount = static_cast<u32>(vertices.size());
    mesh.indexCount = static_cast<u32>(indices.size());
    mesh.vertexCapacity = mesh.vertexCount;
    mesh.indexCapacity = mesh.indexCount;

    if (vertices.empty() || indices.empty()) {
        return mesh; // 隐式转换为Result<EntityMesh>
    }

    for (u32 index : indices) {
        if (index >= vertices.size()) {
            return Error(ErrorCode::InvalidData,
                "Entity mesh index out of range: index=" + std::to_string(index) +
                    ", vertexCount=" + std::to_string(vertices.size()));
        }
    }

    // 创建设备本地顶点缓冲区
    const VkDeviceSize vertexBufferSize = sizeof(ModelVertex) * vertices.size();
    auto result = _createBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mesh.vertexBuffer,
        mesh.vertexMemory);
    if (!result.success()) {
        return result.error();
    }

    result = _uploadToDeviceBuffer(vertices.data(), vertexBufferSize, mesh.vertexBuffer, true);
    if (!result.success()) {
        destroyMesh(mesh);
        return result.error();
    }

    // 创建设备本地索引缓冲区
    const VkDeviceSize indexBufferSize = sizeof(u32) * indices.size();
    result = _createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mesh.indexBuffer,
        mesh.indexMemory);
    if (!result.success()) {
        destroyMesh(mesh);
        return result.error();
    }

    result = _uploadToDeviceBuffer(indices.data(), indexBufferSize, mesh.indexBuffer, false);
    if (!result.success()) {
        destroyMesh(mesh);
        return result.error();
    }

    return mesh; // 隐式转换为Result<EntityMesh>
}

Result<void> EntityPipeline::updateMesh(
    EntityMesh& mesh, const std::vector<ModelVertex>& vertices, const std::vector<u32>& indices)
{
    if (vertices.empty() || indices.empty()) {
        destroyMesh(mesh);
        return Result<void>::ok();
    }

    for (u32 index : indices) {
        if (index >= vertices.size()) {
            return Error(ErrorCode::InvalidData,
                "Entity mesh index out of range during update: index=" + std::to_string(index) +
                    ", vertexCount=" + std::to_string(vertices.size()));
        }
    }

    const u32 requiredVertexCount = static_cast<u32>(vertices.size());
    const u32 requiredIndexCount = static_cast<u32>(indices.size());

    const bool needsVertexRecreate = mesh.vertexBuffer == VK_NULL_HANDLE || mesh.vertexMemory == VK_NULL_HANDLE ||
        mesh.vertexCapacity < requiredVertexCount;
    const bool needsIndexRecreate = mesh.indexBuffer == VK_NULL_HANDLE || mesh.indexMemory == VK_NULL_HANDLE ||
        mesh.indexCapacity < requiredIndexCount;

    VkBuffer replacementVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory replacementVertexMemory = VK_NULL_HANDLE;
    u32 replacementVertexCapacity = mesh.vertexCapacity;

    VkBuffer replacementIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory replacementIndexMemory = VK_NULL_HANDLE;
    u32 replacementIndexCapacity = mesh.indexCapacity;

    if (needsVertexRecreate) {
        replacementVertexCapacity = growCapacity(mesh.vertexCapacity, requiredVertexCount);
        const VkDeviceSize replacementVertexSize =
            sizeof(ModelVertex) * static_cast<VkDeviceSize>(replacementVertexCapacity);
        auto result = _createBuffer(replacementVertexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            replacementVertexBuffer,
            replacementVertexMemory);
        if (!result.success()) {
            return result.error();
        }
    }

    if (needsIndexRecreate) {
        replacementIndexCapacity = growCapacity(mesh.indexCapacity, requiredIndexCount);
        const VkDeviceSize replacementIndexSize = sizeof(u32) * static_cast<VkDeviceSize>(replacementIndexCapacity);
        auto result = _createBuffer(replacementIndexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            replacementIndexBuffer,
            replacementIndexMemory);
        if (!result.success()) {
            if (replacementVertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_device, replacementVertexBuffer, nullptr);
            }
            if (replacementVertexMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device, replacementVertexMemory, nullptr);
            }
            return result.error();
        }
    }

    if (needsVertexRecreate) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, mesh.vertexBuffer, nullptr);
        }
        if (mesh.vertexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, mesh.vertexMemory, nullptr);
        }
        mesh.vertexBuffer = replacementVertexBuffer;
        mesh.vertexMemory = replacementVertexMemory;
        mesh.vertexCapacity = replacementVertexCapacity;
    }

    if (needsIndexRecreate) {
        if (mesh.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, mesh.indexBuffer, nullptr);
        }
        if (mesh.indexMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, mesh.indexMemory, nullptr);
        }
        mesh.indexBuffer = replacementIndexBuffer;
        mesh.indexMemory = replacementIndexMemory;
        mesh.indexCapacity = replacementIndexCapacity;
    }

    const VkDeviceSize vertexUploadSize = sizeof(ModelVertex) * vertices.size();
    auto result = _uploadToDeviceBuffer(vertices.data(), vertexUploadSize, mesh.vertexBuffer, true);
    if (!result.success()) {
        destroyMesh(mesh);
        return result.error();
    }

    const VkDeviceSize indexUploadSize = sizeof(u32) * indices.size();
    result = _uploadToDeviceBuffer(indices.data(), indexUploadSize, mesh.indexBuffer, false);
    if (!result.success()) {
        destroyMesh(mesh);
        return result.error();
    }

    mesh.vertexCount = requiredVertexCount;
    mesh.indexCount = requiredIndexCount;

    return Result<void>::ok();
}

void EntityPipeline::destroyMesh(EntityMesh& mesh)
{
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
    mesh.vertexCapacity = 0;
    mesh.indexCapacity = 0;
}

void EntityPipeline::drawMesh(VkCommandBuffer cmd,
    const EntityMesh& mesh,
    const std::array<f64, 16>& modelMatrix,
    const Vector3f& position,
    f64 scale,
    const Vector4f& overlayColor,
    f32 hurtTime,
    f32 deathTime)
{
    if (mesh.vertexCount == 0 || mesh.indexCount == 0) {
        return;
    }

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 绑定索引缓冲区
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 推送常量 - 与着色器保持一致
    // struct PushConstants {
    //     mat4 model;           // 64 bytes (16 floats)
    //     vec3 entityPos;       // 12 bytes (3 floats)
    //     float scale;          // 4 bytes (1 float)
    //     vec4 overlayColor;    // 16 bytes (4 floats)
    //     float hurtTime;       // 4 bytes (1 float)
    //     float deathTime;      // 4 bytes (1 float)
    //     float _padding0;      // 4 bytes (1 float)
    //     float _padding1;      // 4 bytes (1 float)
    // };                        // Total: 112 bytes (28 floats)
    struct PushConstants {
        std::array<f32, 16> model;
        f32 posX;
        f32 posY;
        f32 posZ;
        f32 scale;
        f32 overlayR;
        f32 overlayG;
        f32 overlayB;
        f32 overlayA;
        f32 hurtTime;
        f32 deathTime;
        f32 _padding0;
        f32 _padding1;
    } pc{};

    // CPU 侧矩阵使用行主序存储，GLSL mat4 默认按列主序读取，
    // 这里在上传前做一次转置，避免第一人称模型出现异常拉伸/错位。
    for (i32 row = 0; row < 4; ++row) {
        for (i32 col = 0; col < 4; ++col) {
            pc.model[static_cast<size_t>(col * 4 + row)] =
                static_cast<f32>(modelMatrix[static_cast<size_t>(row * 4 + col)]);
        }
    }

    pc.posX = position.x;
    pc.posY = position.y;
    pc.posZ = position.z;
    pc.scale = static_cast<f32>(scale);
    pc.overlayR = overlayColor.x;
    pc.overlayG = overlayColor.y;
    pc.overlayB = overlayColor.z;
    pc.overlayA = overlayColor.w;
    pc.hurtTime = hurtTime;
    pc.deathTime = deathTime;
    pc._padding0 = 0.0f;
    pc._padding1 = 0.0f;

    vkCmdPushConstants(cmd,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &pc);

    // 绘制
    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
}

void EntityPipeline::bindTextureDescriptor(VkCommandBuffer cmd)
{
    if (m_textureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &m_textureDescriptorSet, 0, nullptr);
    }
}

void EntityPipeline::setTextureAtlas(VkImageView textureView, VkSampler sampler)
{
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

VkPipelineLayout EntityPipeline::pipelineLayout() const
{
    return m_pipelineLayout;
}

Result<void> EntityPipeline::_createDescriptorLayouts()
{
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

    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_textureDescriptorLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create texture descriptor layout");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::_createTextureSampler()
{
    VkDevice device = m_device;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST; // 实体使用最近邻过滤以保持像素风格
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
        return Error(ErrorCode::InitializationFailed, "Failed to create texture sampler");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::_createDescriptorSets()
{
    VkDevice device = m_device;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_textureDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &m_textureDescriptorSet);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate texture descriptor set");
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::_createGraphicsPipeline(
    VkRenderPass renderPass, VkDescriptorSetLayout cameraDescriptorLayout, VkSampleCountFlagBits sampleCount)
{
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
    rasterizer.cullMode = VK_CULL_MODE_NONE; // 实体模型禁用剔除
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;

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
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 管线布局
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {cameraDescriptorLayout, m_textureDescriptorLayout};

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size =
        sizeof(f32) * 28; // mat4(16) + vec3(3) + float(1) + vec4(4) + float(4) = 28 floats = 112 bytes

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

    // 创建叠加混合管线（用于眼睛发光等效果）
    VkPipelineColorBlendAttachmentState additiveBlendAttachment{};
    additiveBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    additiveBlendAttachment.blendEnable = VK_TRUE;
    // 叠加混合: src * srcAlpha + dst * 1
    additiveBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    additiveBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    additiveBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    additiveBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    additiveBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    additiveBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo additiveColorBlending{};
    additiveColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    additiveColorBlending.logicOpEnable = VK_FALSE;
    additiveColorBlending.attachmentCount = 1;
    additiveColorBlending.pAttachments = &additiveBlendAttachment;

    pipelineInfo.pColorBlendState = &additiveColorBlending;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_additiveBlendPipeline);
    if (result != VK_SUCCESS) {
        spdlog::warn("EntityPipeline: Failed to create additive blend pipeline, falling back to alpha blend only");
        m_additiveBlendPipeline = VK_NULL_HANDLE;
    }

    // ==================== 线段渲染管线 ====================
    // 使用 VK_PRIMITIVE_TOPOLOGY_LINE_LIST，Alpha 混合，用于钓鱼线等
    VkPipelineInputAssemblyStateCreateInfo lineInputAssembly{};
    lineInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    lineInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    lineInputAssembly.primitiveRestartEnable = VK_FALSE;

    // 使用 Alpha 混合（与主管线相同）
    pipelineInfo.pInputAssemblyState = &lineInputAssembly;
    pipelineInfo.pColorBlendState = &colorBlending;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_linePipeline);
    if (result != VK_SUCCESS) {
        spdlog::warn("EntityPipeline: Failed to create line pipeline, falling back to alpha blend only");
        m_linePipeline = VK_NULL_HANDLE;
    }

    return Result<void>::ok();
}

Result<void> EntityPipeline::_createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    return ::mc::client::renderer::VulkanUtils::createBuffer(
        m_device, m_physicalDevice, size, usage, properties, buffer, memory);
}

Result<void> EntityPipeline::_ensureReusableStagingBuffer(
    VkDeviceSize requiredSize, VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize& capacity)
{
    if (requiredSize == 0) {
        return Result<void>::ok();
    }

    if (buffer != VK_NULL_HANDLE && capacity >= requiredSize) {
        return Result<void>::ok();
    }

    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
    capacity = 0;

    auto result = _createBuffer(requiredSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        memory);
    if (!result.success()) {
        return result.error();
    }

    capacity = requiredSize;
    return Result<void>::ok();
}

Result<void> EntityPipeline::_uploadToDeviceBuffer(
    const void* sourceData, VkDeviceSize size, VkBuffer destinationBuffer, bool useVertexStagingBuffer)
{
    if (sourceData == nullptr || destinationBuffer == VK_NULL_HANDLE || size == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid upload arguments for EntityPipeline::_uploadToDeviceBuffer");
    }

    VkBuffer& stagingBuffer = useVertexStagingBuffer ? m_vertexStagingBuffer : m_indexStagingBuffer;
    VkDeviceMemory& stagingMemory = useVertexStagingBuffer ? m_vertexStagingMemory : m_indexStagingMemory;
    VkDeviceSize& stagingCapacity = useVertexStagingBuffer ? m_vertexStagingCapacity : m_indexStagingCapacity;

    auto result = _ensureReusableStagingBuffer(size, stagingBuffer, stagingMemory, stagingCapacity);
    if (!result.success()) {
        return result.error();
    }

    void* mappedData = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, stagingMemory, 0, size, 0, &mappedData);
    if (mapResult != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to map reusable staging buffer memory");
    }

    std::memcpy(mappedData, sourceData, static_cast<size_t>(size));
    vkUnmapMemory(m_device, stagingMemory);

    _copyBuffer(stagingBuffer, destinationBuffer, size);
    return Result<void>::ok();
}

void EntityPipeline::_destroyReusableStagingBuffers()
{
    if (m_vertexStagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_vertexStagingBuffer, nullptr);
        m_vertexStagingBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexStagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_vertexStagingMemory, nullptr);
        m_vertexStagingMemory = VK_NULL_HANDLE;
    }
    m_vertexStagingCapacity = 0;

    if (m_indexStagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_indexStagingBuffer, nullptr);
        m_indexStagingBuffer = VK_NULL_HANDLE;
    }
    if (m_indexStagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_indexStagingMemory, nullptr);
        m_indexStagingMemory = VK_NULL_HANDLE;
    }
    m_indexStagingCapacity = 0;
}

void EntityPipeline::_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    _endSingleTimeCommands(cmd);
}

VkCommandBuffer EntityPipeline::_beginSingleTimeCommands()
{
    return ::mc::client::renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void EntityPipeline::_endSingleTimeCommands(VkCommandBuffer cmd)
{
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    ::mc::client::renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);
}

} // namespace mc::client::renderer::entity::pipeline
