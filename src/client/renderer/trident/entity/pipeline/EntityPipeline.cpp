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
#include "client/renderer/api/buffer/OffsetAllocatorHeader.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

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

Result<void> EntityPipeline::initialize(trident::TridentContext* context,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkDescriptorSetLayout cameraDescriptorLayout,
    VkDescriptorPool descriptorPool,
    VkCommandPool commandPool,
    VkSampleCountFlagBits sampleCount,
    u32 maxFramesInFlight)
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    m_context = context;
    m_device = device;
    m_physicalDevice = physicalDevice;

    destroy();
    // destroy() 会把 m_maxFramesInFlight 复位为 1，且会把 m_context 置空，
    // 必须在 destroy 之后重新设置二者，否则 _createDescriptorSets 只分配 1 个
    // 纹理描述符集，且 _uploadViaStagingPool 取不到 staging pool。
    m_context = context;
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_graphicsQueue = graphicsQueue;
    m_descriptorPool = descriptorPool;
    // commandPool 保留于签名兼容；mega-buffer 子分配走统一暂存缓冲池，不再需要单次命令池。

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
        m_additiveBlendPipeline != VK_NULL_HANDLE || m_multiplyBlendPipeline != VK_NULL_HANDLE ||
        m_noneBlendPipeline != VK_NULL_HANDLE || m_linePipeline != VK_NULL_HANDLE ||
        m_pipelineLayout != VK_NULL_HANDLE || m_textureSampler != VK_NULL_HANDLE ||
        m_textureDescriptorLayout != VK_NULL_HANDLE || !m_vertexSegments.empty() || !m_indexSegments.empty();

    // 关闭阶段：强制归还所有仍在延迟队列中的子分配区间（绕过帧延迟窗口）。
    // 此刻设备即将销毁，延迟窗口已无意义，但守恒断言要求段内 localUsedBytes 归零，
    // 故先归还再销毁 VkBuffer。注意：仍存在"所有者从未调用 destroyMesh 的 live mesh"
    // 的情况——其 localUsedBytes 无法在此归还，会触发下方守恒断言暴露泄漏。
    for (const auto& pending : m_pendingDestroys) {
        if (pending.hasVertex && pending.vertexSegmentIndex < m_vertexSegments.size()) {
            _freeAllocation(
                m_vertexSegments[pending.vertexSegmentIndex], pending.vertexAllocation, pending.vertexAlignedSize);
        }
        if (pending.hasIndex && pending.indexSegmentIndex < m_indexSegments.size()) {
            _freeAllocation(
                m_indexSegments[pending.indexSegmentIndex], pending.indexAllocation, pending.indexAlignedSize);
        }
    }
    m_pendingDestroys.clear();

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

    // 销毁乘法混合管线
    if (m_multiplyBlendPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_multiplyBlendPipeline, nullptr);
        m_multiplyBlendPipeline = VK_NULL_HANDLE;
    }

    // 销毁无混合管线
    if (m_noneBlendPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_noneBlendPipeline, nullptr);
        m_noneBlendPipeline = VK_NULL_HANDLE;
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

    // 关闭阶段清理：所有子分配区间随段 VkBuffer 一并销毁，无需再归还给 OffsetAllocator。
    // 若某段仍有 localUsedBytes != 0，说明存在所有者在关闭时未调用 destroyMesh 的 live mesh
    // （如 SpecialEntityRenderers 的 blockMeshCache 缓存）。设备已 idle 且整段 VkBuffer 即将
    // 销毁，此为关闭期可接受的丢弃；记录泄漏量但不视为致命错误（运行期的真实泄漏由
    // _freeAllocation 的守恒断言在渲染期间捕获）。
    for (auto& seg : m_vertexSegments) {
        if (seg.localUsedBytes != 0) {
            spdlog::warn("EntityPipeline: vertex mega-buffer segment leaked {} bytes at shutdown "
                         "(mesh owner did not destroyMesh before pipeline teardown)",
                seg.localUsedBytes);
        }
        if (seg.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, seg.buffer, nullptr);
        }
        if (seg.memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, seg.memory, nullptr);
        }
        seg.allocator.reset();
    }
    m_vertexSegments.clear();

    for (auto& seg : m_indexSegments) {
        if (seg.localUsedBytes != 0) {
            spdlog::warn("EntityPipeline: index mega-buffer segment leaked {} bytes at shutdown "
                         "(mesh owner did not destroyMesh before pipeline teardown)",
                seg.localUsedBytes);
        }
        if (seg.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, seg.buffer, nullptr);
        }
        if (seg.memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, seg.memory, nullptr);
        }
        seg.allocator.reset();
    }
    m_indexSegments.clear();

    // 描述符集随 descriptorPool 由所有者统一回收，这里只清空引用。
    m_textureDescriptorSets.clear();

    m_currentFrameIndex = 0;
    m_frameCounter = 0;
    m_maxFramesInFlight = 1;
    m_context = nullptr;

    m_initialized = false;

    if (hadResources) {
        spdlog::info("EntityPipeline destroyed");
    }
}

void EntityPipeline::beginFrame(u32 frameIndex)
{
    m_currentFrameIndex = (m_maxFramesInFlight > 0) ? (frameIndex % m_maxFramesInFlight) : 0;
    // 推进帧计数器，使先前入队的延迟销毁条目逐步到期。
    ++m_frameCounter;
}

void EntityPipeline::processPendingDestroys()
{
    if (m_pendingDestroys.empty()) {
        return;
    }

    // 保留窗口：maxFramesInFlight + 1 帧。在飞帧最多为 maxFramesInFlight，
    // 当前正在录制的帧再占 1 个槽位，故需等待 maxFramesInFlight + 1 帧后才可安全归还。
    const u64 safeRetention = static_cast<u64>(m_maxFramesInFlight) + 1;

    for (auto it = m_pendingDestroys.begin(); it != m_pendingDestroys.end();) {
        const u64 age = (m_frameCounter >= it->enqueueFrame) ? (m_frameCounter - it->enqueueFrame) : 0;
        if (age >= safeRetention) {
            // 归还子分配区间给对应段的 OffsetAllocator，VkBuffer 段本身不销毁。
            if (it->hasVertex && it->vertexSegmentIndex < m_vertexSegments.size()) {
                _freeAllocation(m_vertexSegments[it->vertexSegmentIndex], it->vertexAllocation, it->vertexAlignedSize);
            }
            if (it->hasIndex && it->indexSegmentIndex < m_indexSegments.size()) {
                _freeAllocation(m_indexSegments[it->indexSegmentIndex], it->indexAllocation, it->indexAlignedSize);
            }
            it = m_pendingDestroys.erase(it);
        } else {
            ++it;
        }
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
        case BlendMode::Multiply:
            pipelineToBind = m_multiplyBlendPipeline;
            break;
        case BlendMode::None:
            pipelineToBind = m_noneBlendPipeline;
            break;
        case BlendMode::Lines:
            pipelineToBind = m_linePipeline;
            break;
        case BlendMode::Alpha:
        default:
            // 默认回退到 Alpha 混合管线，涵盖所有未匹配的枚举值
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

    // vertex mega-buffer 段子分配
    const u64 vertexUploadSize = sizeof(ModelVertex) * vertices.size();
    const u64 vertexAlignedSize = _alignUp(vertexUploadSize);
    auto result = _subAllocate(
        m_vertexSegments, kVertexSegmentCapacity, vertexAlignedSize, mesh.vertexSegmentIndex, mesh.vertexAllocation);
    if (!result.success()) {
        return result.error();
    }
    mesh.vertexBuffer = m_vertexSegments[mesh.vertexSegmentIndex].buffer;
    mesh.vertexOffset = mesh.vertexAllocation.offset;
    mesh.vertexAlignedSize = vertexAlignedSize;

    result = _uploadViaStagingPool(vertices.data(), vertexUploadSize, mesh.vertexBuffer, mesh.vertexOffset);
    if (!result.success()) {
        // 刚分配、尚未提交到任何命令缓冲区，立即归还区间。
        _freeAllocation(m_vertexSegments[mesh.vertexSegmentIndex], mesh.vertexAllocation, vertexAlignedSize);
        mesh = {};
        return result.error();
    }

    // index mega-buffer 段子分配
    const u64 indexUploadSize = sizeof(u32) * indices.size();
    const u64 indexAlignedSize = _alignUp(indexUploadSize);
    result = _subAllocate(
        m_indexSegments, kIndexSegmentCapacity, indexAlignedSize, mesh.indexSegmentIndex, mesh.indexAllocation);
    if (!result.success()) {
        _freeAllocation(m_vertexSegments[mesh.vertexSegmentIndex], mesh.vertexAllocation, vertexAlignedSize);
        mesh = {};
        return result.error();
    }
    mesh.indexBuffer = m_indexSegments[mesh.indexSegmentIndex].buffer;
    mesh.indexOffset = mesh.indexAllocation.offset;
    mesh.indexAlignedSize = indexAlignedSize;

    result = _uploadViaStagingPool(indices.data(), indexUploadSize, mesh.indexBuffer, mesh.indexOffset);
    if (!result.success()) {
        _freeAllocation(m_vertexSegments[mesh.vertexSegmentIndex], mesh.vertexAllocation, vertexAlignedSize);
        _freeAllocation(m_indexSegments[mesh.indexSegmentIndex], mesh.indexAllocation, indexAlignedSize);
        mesh = {};
        return result.error();
    }

    return mesh; // 隐式转换为Result<EntityMesh>
}

Result<void> EntityPipeline::updateMesh(
    EntityMesh& mesh, const std::vector<model::ModelVertex>& vertices, const std::vector<u32>& indices)
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

    // mega-buffer 子分配大小在分配时固定。若新数据量超出当前区间容量，
    // 释放旧区间（延迟）、子分配新区间；否则就地覆盖同一段区间。
    const u64 vertexUploadSize = sizeof(ModelVertex) * vertices.size();
    const u64 vertexAlignedSize = _alignUp(vertexUploadSize);
    const u64 indexUploadSize = sizeof(u32) * indices.size();
    const u64 indexAlignedSize = _alignUp(indexUploadSize);

    const bool needsVertexReallocate =
        mesh.vertexBuffer == VK_NULL_HANDLE || vertexAlignedSize > mesh.vertexAlignedSize;
    const bool needsIndexReallocate = mesh.indexBuffer == VK_NULL_HANDLE || indexAlignedSize > mesh.indexAlignedSize;

    // 旧区间延迟归还（仍可能被在飞命令缓冲区引用）
    if (needsVertexReallocate && mesh.vertexBuffer != VK_NULL_HANDLE) {
        PendingDestroy pending;
        pending.vertexAllocation = mesh.vertexAllocation;
        pending.vertexAlignedSize = mesh.vertexAlignedSize;
        pending.vertexSegmentIndex = mesh.vertexSegmentIndex;
        pending.hasVertex = true;
        pending.enqueueFrame = m_frameCounter;
        m_pendingDestroys.push_back(std::move(pending));
    }
    if (needsIndexReallocate && mesh.indexBuffer != VK_NULL_HANDLE) {
        PendingDestroy pending;
        pending.indexAllocation = mesh.indexAllocation;
        pending.indexAlignedSize = mesh.indexAlignedSize;
        pending.indexSegmentIndex = mesh.indexSegmentIndex;
        pending.hasIndex = true;
        pending.enqueueFrame = m_frameCounter;
        m_pendingDestroys.push_back(std::move(pending));
    }

    if (needsVertexReallocate) {
        auto result = _subAllocate(m_vertexSegments,
            kVertexSegmentCapacity,
            vertexAlignedSize,
            mesh.vertexSegmentIndex,
            mesh.vertexAllocation);
        if (!result.success()) {
            // 子分配失败：mesh 的旧区间已延迟归还，将 mesh 置空避免悬垂。
            mesh = {};
            return result.error();
        }
        mesh.vertexBuffer = m_vertexSegments[mesh.vertexSegmentIndex].buffer;
        mesh.vertexOffset = mesh.vertexAllocation.offset;
        mesh.vertexAlignedSize = vertexAlignedSize;
    }

    if (needsIndexReallocate) {
        auto result = _subAllocate(
            m_indexSegments, kIndexSegmentCapacity, indexAlignedSize, mesh.indexSegmentIndex, mesh.indexAllocation);
        if (!result.success()) {
            // index 子分配失败：vertex 新区间刚分配且尚未提交，立即归还。
            if (needsVertexReallocate) {
                _freeAllocation(m_vertexSegments[mesh.vertexSegmentIndex], mesh.vertexAllocation, vertexAlignedSize);
            }
            mesh = {};
            return result.error();
        }
        mesh.indexBuffer = m_indexSegments[mesh.indexSegmentIndex].buffer;
        mesh.indexOffset = mesh.indexAllocation.offset;
        mesh.indexAlignedSize = indexAlignedSize;
    }

    auto result = _uploadViaStagingPool(vertices.data(), vertexUploadSize, mesh.vertexBuffer, mesh.vertexOffset);
    if (!result.success()) {
        destroyMesh(mesh);
        return result.error();
    }

    result = _uploadViaStagingPool(indices.data(), indexUploadSize, mesh.indexBuffer, mesh.indexOffset);
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
    // 公共销毁入口：mesh 可能已被某帧命令缓冲区引用，必须延迟归还子分配区间。
    _enqueueDestroyMesh(mesh);
}

void EntityPipeline::_enqueueDestroyMesh(EntityMesh& mesh)
{
    if (mesh.vertexBuffer != VK_NULL_HANDLE) {
        PendingDestroy pending;
        pending.vertexAllocation = mesh.vertexAllocation;
        pending.vertexAlignedSize = mesh.vertexAlignedSize;
        pending.vertexSegmentIndex = mesh.vertexSegmentIndex;
        pending.hasVertex = true;
        pending.enqueueFrame = m_frameCounter;
        m_pendingDestroys.push_back(std::move(pending));
    }
    if (mesh.indexBuffer != VK_NULL_HANDLE) {
        PendingDestroy pending;
        pending.indexAllocation = mesh.indexAllocation;
        pending.indexAlignedSize = mesh.indexAlignedSize;
        pending.indexSegmentIndex = mesh.indexSegmentIndex;
        pending.hasIndex = true;
        pending.enqueueFrame = m_frameCounter;
        m_pendingDestroys.push_back(std::move(pending));
    }

    mesh = {};
}

void EntityPipeline::drawMesh(VkCommandBuffer cmd,
    const EntityMesh& mesh,
    const std::array<f64, 16>& modelMatrix,
    const Vector3f& position,
    f64 scale,
    const Vector4f& overlayColor,
    f32 hurtTime,
    f32 deathTime,
    f32 fullbright)
{
    if (mesh.vertexCount == 0 || mesh.indexCount == 0) {
        return;
    }

    // 绑定顶点缓冲区（mega-buffer 段内偏移）
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {mesh.vertexOffset};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 绑定索引缓冲区（mega-buffer 段内偏移）
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, mesh.indexOffset, VK_INDEX_TYPE_UINT32);

    // 推送常量 - 与着色器保持一致
    // struct PushConstants {
    //     mat4 model;           // 64 bytes (16 floats)
    //     vec3 entityPos;       // 12 bytes (3 floats)
    //     float scale;          // 4 bytes (1 float)
    //     vec4 overlayColor;    // 16 bytes (4 floats)
    //     float hurtTime;       // 4 bytes (1 float)
    //     float deathTime;      // 4 bytes (1 float)
    //     float fullbright;     // 4 bytes (1 float) - 全亮光照因子
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
        f32 fullbright;
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
    pc.fullbright = fullbright;
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
    VkDescriptorSet set = (m_currentFrameIndex < m_textureDescriptorSets.size())
        ? m_textureDescriptorSets[m_currentFrameIndex]
        : VK_NULL_HANDLE;
    if (set != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &set, 0, nullptr);
    }
}

void EntityPipeline::setTextureAtlas(VkImageView textureView, VkSampler sampler)
{
    VkDescriptorSet set = (m_currentFrameIndex < m_textureDescriptorSets.size())
        ? m_textureDescriptorSets[m_currentFrameIndex]
        : VK_NULL_HANDLE;
    if (set == VK_NULL_HANDLE || textureView == VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_device;

    // 仅更新当前帧的描述符集，避免改写仍被在飞帧命令缓冲区引用的描述符
    // （device-lost 根因之一：帧内切图集时 vkUpdateDescriptorSet 撞上在飞读取）。
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureView;
    imageInfo.sampler = sampler != VK_NULL_HANDLE ? sampler : m_textureSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

void EntityPipeline::setTextureAtlasAllFrames(VkImageView textureView, VkSampler sampler)
{
    if (textureView == VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_device;
    VkSampler resolvedSampler = sampler != VK_NULL_HANDLE ? sampler : m_textureSampler;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureView;
    imageInfo.sampler = resolvedSampler;

    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(m_textureDescriptorSets.size());
    for (VkDescriptorSet set : m_textureDescriptorSets) {
        if (set == VK_NULL_HANDLE) {
            continue;
        }
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = set;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        writes.push_back(descriptorWrite);
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
    }
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

    // 每帧分配一个纹理描述符集。per-frame set 避免在飞帧读取被本帧 setTextureAtlas
    // 改写的描述符（device-lost 根因之一）。
    const u32 frameCount = std::max<u32>(1u, m_maxFramesInFlight);
    m_textureDescriptorSets.assign(frameCount, VK_NULL_HANDLE);

    // VkDescriptorSetAllocateInfo 要求 pSetLayouts 指向 descriptorSetCount 个 layout 句柄。
    std::vector<VkDescriptorSetLayout> layouts(frameCount, m_textureDescriptorLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = frameCount;
    allocInfo.pSetLayouts = layouts.data();

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, m_textureDescriptorSets.data());
    if (result != VK_SUCCESS) {
        m_textureDescriptorSets.clear();
        return Error(ErrorCode::InitializationFailed, "Failed to allocate texture descriptor sets");
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

    if (result != VK_SUCCESS) {
        // 主管线创建失败：销毁着色器模块与管线布局后返回。
        // 注意：着色器模块在此销毁，因为后续变体管线不会创建。
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
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

    // ==================== 乘法混合管线 ====================
    // 使用 DST_COLOR * SRC_COLOR 的对称乘法混合（out = 2 * src * dst），
    // 对应 MC 1.21.11 RenderPipelines.CRUMBLING 的 BlendFunction(DST_COLOR, SRC_COLOR, ONE, ZERO)，
    // 以及本项目 BreakProgressRenderer 的破坏进度叠加。纹理按 50% 亮度补偿 2x 系数。
    // 用于实体颜色调制/着色叠加（如受损红色闪烁覆盖、环境着色等）。
    VkPipelineColorBlendAttachmentState multiplyBlendAttachment{};
    multiplyBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    multiplyBlendAttachment.blendEnable = VK_TRUE;
    // 乘法混合: src * dst + dst * src = 2 * src * dst
    multiplyBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
    multiplyBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    multiplyBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // alpha 通道保持源 alpha 透传，避免叠加导致 alpha 失真
    multiplyBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    multiplyBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    multiplyBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo multiplyColorBlending{};
    multiplyColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    multiplyColorBlending.logicOpEnable = VK_FALSE;
    multiplyColorBlending.attachmentCount = 1;
    multiplyColorBlending.pAttachments = &multiplyBlendAttachment;

    // 还原输入装配状态为三角形列表（line 块会修改 pInputAssemblyState，此处先复位）
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pColorBlendState = &multiplyColorBlending;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_multiplyBlendPipeline);
    if (result != VK_SUCCESS) {
        spdlog::warn("EntityPipeline: Failed to create multiply blend pipeline, falling back to alpha blend only");
        m_multiplyBlendPipeline = VK_NULL_HANDLE;
    }

    // ==================== 无混合管线 ====================
    // blendEnable=VK_FALSE，不透明/剪切实体渲染，对应 MC Java 的 withoutBlend()
    // （ENTITY_SOLID / ENTITY_CUTOUT / ENTITY_CUTOUT_NO_CULL 等管线）。
    // 片元着色器输出直接写入帧缓冲，不与现有颜色混合。
    VkPipelineColorBlendAttachmentState noneBlendAttachment{};
    noneBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    noneBlendAttachment.blendEnable = VK_FALSE;
    // blendEnable=VK_FALSE 时以下字段被忽略，置零保持整洁
    noneBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    noneBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    noneBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    noneBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    noneBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    noneBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo noneColorBlending{};
    noneColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    noneColorBlending.logicOpEnable = VK_FALSE;
    noneColorBlending.attachmentCount = 1;
    noneColorBlending.pAttachments = &noneBlendAttachment;

    pipelineInfo.pColorBlendState = &noneColorBlending;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_noneBlendPipeline);
    if (result != VK_SUCCESS) {
        spdlog::warn("EntityPipeline: Failed to create no-blend pipeline, falling back to alpha blend only");
        m_noneBlendPipeline = VK_NULL_HANDLE;
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

    // 所有管线（主管线 + 4 个变体）均已创建完毕，现在销毁着色器模块。
    // 必须在变体管线创建之后再销毁：MoltenVK 在 vkCreateGraphicsPipelines 时
    // 通过 SPIRV-Cross 对 VkShaderModule 做反射与 SPIRV→MSL 转换，过早销毁
    // 会导致后续变体管线访问已释放的 SPIRV 数据，报 "SPIRV file too small"。
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return Result<void>::ok();
}

u64 EntityPipeline::_alignUp(u64 size)
{
    return (size + kSubAllocAlign - 1) & ~(kSubAllocAlign - 1);
}

Result<void> EntityPipeline::_createSegment(MegaBufferSegment& segment, VkDeviceSize capacity, VkBufferUsageFlags usage)
{
    segment.capacity = capacity;
    auto result = ::mc::client::renderer::VulkanUtils::createBuffer(m_device,
        m_physicalDevice,
        capacity,
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        segment.buffer,
        segment.memory);
    if (!result.success()) {
        return result.error();
    }

    // OffsetAllocator 容量上限为 u32；段容量 32MB/8MB 远在范围内。
    segment.allocator = std::make_unique<OffsetAllocator::Allocator>(static_cast<u32>(capacity));
    segment.localUsedBytes = 0;
    segment.localFreeBytes = capacity;
    return Result<void>::ok();
}

Result<void> EntityPipeline::_subAllocate(std::vector<MegaBufferSegment>& segments,
    VkDeviceSize segmentCapacity,
    u64 alignedSize,
    u32& outSegmentIndex,
    OffsetAllocator::Allocation& outAllocation)
{
    if (alignedSize == 0) {
        return Error(ErrorCode::InvalidArgument, "EntityPipeline::_subAllocate: zero size");
    }

    // 在已有段中寻找可容纳的空闲区间
    // 注：OffsetAllocator::allocate 仅接受 size，无 align 参数。因所有请求经 _alignUp
    // 向上取整到 kSubAllocAlign=16，且段从对齐的偏移 0 起按 16 倍数切分，
    // 返回 offset 保持 16 倍数对齐（满足顶点/索引绑定偏移要求）。
    for (u32 i = 0; i < segments.size(); ++i) {
        auto& seg = segments[i];
        OffsetAllocator::Allocation alloc = seg.allocator->allocate(static_cast<u32>(alignedSize));
        if (alloc.offset != OffsetAllocator::Allocation::NO_SPACE) {
            outSegmentIndex = i;
            outAllocation = alloc;
            seg.localUsedBytes += alignedSize;
            seg.localFreeBytes -= alignedSize;
            return Result<void>::ok();
        }
    }

    // 所有段均无足够连续空间，追加新段
    // 若单次请求超过段容量，按对齐大小向上取整作为新段容量（罕见，如巨型实体 mesh）
    const VkDeviceSize newCapacity = std::max<VkDeviceSize>(segmentCapacity, alignedSize);
    MegaBufferSegment newSeg;
    const VkBufferUsageFlags usage =
        (&segments == &m_vertexSegments) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    auto result = _createSegment(newSeg, newCapacity, usage);
    if (!result.success()) {
        return result.error();
    }
    segments.push_back(std::move(newSeg));
    auto& seg = segments.back();

    OffsetAllocator::Allocation alloc = seg.allocator->allocate(static_cast<u32>(alignedSize));
    if (alloc.offset == OffsetAllocator::Allocation::NO_SPACE) {
        return Error(ErrorCode::OutOfMemory, "EntityPipeline::_subAllocate: fresh segment rejected allocation");
    }
    outSegmentIndex = static_cast<u32>(segments.size() - 1);
    outAllocation = alloc;
    seg.localUsedBytes += alignedSize;
    seg.localFreeBytes -= alignedSize;
    return Result<void>::ok();
}

void EntityPipeline::_freeAllocation(
    MegaBufferSegment& segment, const OffsetAllocator::Allocation& alloc, u64 alignedSize)
{
    if (alloc.offset == OffsetAllocator::Allocation::NO_SPACE) {
        return;
    }
    segment.allocator->free(alloc);
    segment.localUsedBytes -= alignedSize;
    segment.localFreeBytes += alignedSize;

    // 守恒断言：OffsetAllocator 报告的空闲空间应与本地记账一致
    const u64 reported = segment.allocator->storageReport().totalFreeSpace;
    MC_ASSERT_RELEASE_MSG(reported == segment.localFreeBytes,
        "EntityPipeline mega-buffer segment conservation mismatch: OffsetAllocator free space disagrees with local "
        "bookkeeping");
}

Result<void> EntityPipeline::_uploadViaStagingPool(
    const void* sourceData, u64 size, VkBuffer dstBuffer, VkDeviceSize dstOffset)
{
    if (sourceData == nullptr || dstBuffer == VK_NULL_HANDLE || size == 0) {
        return Error(ErrorCode::InvalidArgument, "EntityPipeline::_uploadViaStagingPool: invalid arguments");
    }

    auto* pool = m_context ? m_context->stagingPool() : nullptr;
    if (pool == nullptr) {
        return Error(ErrorCode::NotInitialized, "EntityPipeline: staging pool not available");
    }

    auto handle = pool->stage(size);
    if (!handle.valid) {
        return Error(ErrorCode::OutOfMemory, "EntityPipeline: staging pool out of space");
    }
    std::memcpy(handle.mappedPtr, sourceData, static_cast<size_t>(size));
    auto result = pool->copyToBuffer(handle, dstBuffer, dstOffset);
    pool->release(handle);
    return result;
}

} // namespace mc::client::renderer::entity::pipeline
