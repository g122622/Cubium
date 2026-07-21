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

#include "FogManager.hpp"
#include "common/core/Constants.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::fog {

// 雾效果常量
static constexpr f64 WATER_FOG_DENSITY = 0.05;                              // 水中雾密度
static constexpr f64 LAVA_FOG_DENSITY = 0.25;                               // 岩浆雾密度
static constexpr f64 FOG_START_RATIO = 0.75;                                // 雾起始距离比例（相对于远平面）
static constexpr f64 FOG_END_RATIO = 1.0;                                   // 雾结束距离比例
static constexpr f64 CHUNK_SIZE = static_cast<f64>(mc::world::CHUNK_WIDTH); // 区块大小（方块）

FogManager::FogManager() = default;

FogManager::~FogManager()
{
    destroy();
}

FogManager::FogManager(FogManager&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_descriptorPool(other.m_descriptorPool)
    , m_layout(other.m_layout)
    , m_uniformBuffers(std::move(other.m_uniformBuffers))
    , m_uniformMemory(std::move(other.m_uniformMemory))
    , m_mappedMemory(std::move(other.m_mappedMemory))
    , m_descriptorSets(std::move(other.m_descriptorSets))
    , m_fogUBO(other.m_fogUBO)
    , m_maxFramesInFlight(other.m_maxFramesInFlight)
    , m_initialized(other.m_initialized)
    , m_currentFogMode(other.m_currentFogMode)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_descriptorPool = VK_NULL_HANDLE;
    other.m_layout = VK_NULL_HANDLE;
    other.m_initialized = false;
}

FogManager& FogManager::operator=(FogManager&& other) noexcept
{
    if (this != &other) {
        destroy();

        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_descriptorPool = other.m_descriptorPool;
        m_layout = other.m_layout;
        m_uniformBuffers = std::move(other.m_uniformBuffers);
        m_uniformMemory = std::move(other.m_uniformMemory);
        m_mappedMemory = std::move(other.m_mappedMemory);
        m_descriptorSets = std::move(other.m_descriptorSets);
        m_fogUBO = other.m_fogUBO;
        m_maxFramesInFlight = other.m_maxFramesInFlight;
        m_initialized = other.m_initialized;
        m_currentFogMode = other.m_currentFogMode;

        other.m_device = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_descriptorPool = VK_NULL_HANDLE;
        other.m_layout = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

Result<void> FogManager::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout layout,
    u32 maxFramesInFlight)
{
    if (m_initialized) {
        spdlog::warn("FogManager already initialized");
        return Result<void>::ok();
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_descriptorPool = descriptorPool;
    m_layout = layout;
    m_maxFramesInFlight = maxFramesInFlight;

    // 创建 Uniform 缓冲区
    auto bufferResult = _createUniformBuffers();
    if (bufferResult.failed()) {
        return bufferResult;
    }

    // 创建描述符集
    auto descriptorResult = _createDescriptorSets();
    if (descriptorResult.failed()) {
        _destroyUniformBuffers();
        return descriptorResult;
    }

    // 初始化默认雾参数
    m_fogUBO.fogStart = 0.0f;
    m_fogUBO.fogEnd = 256.0f;
    m_fogUBO.fogDensity = 0.0f;
    m_fogUBO.fogMode = static_cast<i32>(FogMode::Linear);
    m_fogUBO.fogColor = glm::vec4(0.7f, 0.75f, 0.8f, 1.0f);

    m_initialized = true;
    spdlog::info("FogManager initialized successfully");
    return Result<void>::ok();
}

void FogManager::destroy()
{
    if (!m_initialized) {
        return;
    }

    _destroyUniformBuffers();

    // 描述符集会随描述符池一起销毁
    m_descriptorSets.clear();

    m_initialized = false;
}

void FogManager::update(i32 renderDistanceChunks,
    f64 rainStrength,
    f64 thunderStrength,
    f64 landFogDensity,
    const glm::vec4& skyFogColor,
    const glm::vec3& cameraPos,
    f32 cameraSkyLight,
    bool cameraBiomeHasPrecip)
{
    MC_UNUSED(thunderStrength);
    MC_UNUSED(cameraPos);

    if (!m_initialized) {
        return;
    }

    // 计算渲染距离（方块数）
    const f64 renderDistance = static_cast<f64>(renderDistanceChunks) * CHUNK_SIZE;

    // 根据雾模式计算参数
    if (m_currentFogMode == FogMode::Linear) {
        _calculateLinearFog(renderDistance);

        // 用户设置影响：video.fogDensity = 1.0 时保持当前效果
        // - 0.0：更清晰（雾更远）
        // - 2.0：更浓（雾更近）
        const f64 clampedLandFogDensity = std::clamp(landFogDensity, 0.0, 2.0);
        const f64 distanceScale = std::clamp(2.0 - clampedLandFogDensity, 0.25, 2.0);
        m_fogUBO.fogStart = static_cast<f32>(m_fogUBO.fogStart * distanceScale);
        m_fogUBO.fogEnd = static_cast<f32>(m_fogUBO.fogEnd * distanceScale);

        // 雨天雾距缩短（对齐原版 AtmosphericFogEnvironment.rainFogMultiplier）：
        // 相机 skyLight 越高（白天/空旷）雨天雾越浓；夜间 skyLight<=8 时不缩短。
        // 降水群系全额，非降水群系减半（沙漠等雨天不起雾）。
        const f32 skyFactor = std::clamp((cameraSkyLight - 8.0f) / 7.0f, 0.0f, 1.0f);
        const f32 target = static_cast<f32>(rainStrength) * skyFactor * (cameraBiomeHasPrecip ? 1.0f : 0.5f);
        m_rainFogMultiplier += (target - m_rainFogMultiplier) * 0.2f;

        const f32 rainStartOffset = -160.0f * m_rainFogMultiplier;
        const f32 rainEndOffset = -256.0f * m_rainFogMultiplier;
        m_fogUBO.fogStart = static_cast<f32>(std::max(0.0f, m_fogUBO.fogStart + rainStartOffset));
        m_fogUBO.fogEnd = static_cast<f32>(std::max(96.0f, m_fogUBO.fogEnd + rainEndOffset));
    }

    // 设置雾颜色（从天空颜色获取）
    m_fogUBO.fogColor = skyFogColor;

    // 更新所有帧的 Uniform 缓冲区
    for (u32 i = 0; i < m_maxFramesInFlight; ++i) {
        _updateUniformBuffer(i);
    }
}

void FogManager::setFogMode(FogMode mode)
{
    m_currentFogMode = mode;
    m_fogUBO.fogMode = static_cast<i32>(mode);
}

void FogManager::setUnderwater(u32 waterFogColor)
{
    setFogMode(FogMode::Exp2);
    m_fogUBO.fogDensity = static_cast<f32>(WATER_FOG_DENSITY);
    // 从 RGB 格式转换为 glm::vec4
    m_fogUBO.fogColor = glm::vec4(static_cast<f32>((waterFogColor >> 16) & 0xFF) / 255.0f, // R
        static_cast<f32>((waterFogColor >> 8) & 0xFF) / 255.0f,                            // G
        static_cast<f32>(waterFogColor & 0xFF) / 255.0f,                                   // B
        1.0f);
}

void FogManager::setInLava()
{
    setFogMode(FogMode::Exp2);
    m_fogUBO.fogDensity = static_cast<f32>(LAVA_FOG_DENSITY);
    // 岩浆雾颜色是深橙红色
    m_fogUBO.fogColor = glm::vec4(0.6f, 0.1f, 0.0f, 1.0f);
}

void FogManager::resetToLand()
{
    setFogMode(FogMode::Linear);
    m_fogUBO.fogDensity = 0.0f;
}

VkDescriptorSet FogManager::descriptorSet(u32 frameIndex) const
{
    if (frameIndex >= m_descriptorSets.size()) {
        return VK_NULL_HANDLE;
    }
    return m_descriptorSets[frameIndex];
}

void FogManager::_calculateLinearFog(f64 renderDistance)
{
    // 线性雾：fogStart 和 fogEnd 控制雾的范围
    m_fogUBO.fogStart = static_cast<f32>(renderDistance * FOG_START_RATIO);
    m_fogUBO.fogEnd = static_cast<f32>(renderDistance * FOG_END_RATIO);
}

void FogManager::_updateUniformBuffer(u32 frameIndex)
{
    if (frameIndex >= m_uniformBuffers.size() || !m_mappedMemory[frameIndex]) {
        return;
    }

    std::memcpy(m_mappedMemory[frameIndex], &m_fogUBO, sizeof(FogUBO));
}

Result<void> FogManager::_createUniformBuffers()
{
    m_uniformBuffers.resize(m_maxFramesInFlight);
    m_uniformMemory.resize(m_maxFramesInFlight);
    m_mappedMemory.resize(m_maxFramesInFlight, nullptr);

    const VkDeviceSize bufferSize = sizeof(FogUBO);

    for (u32 i = 0; i < m_maxFramesInFlight; ++i) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_uniformBuffers[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create fog uniform buffer {}", i);
            return Error(ErrorCode::OutOfMemory, "Failed to create fog uniform buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, m_uniformBuffers[i], &memRequirements);

        auto memoryTypeResult = _findMemoryType(
            memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (memoryTypeResult.failed()) {
            return memoryTypeResult.error();
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeResult.value();

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_uniformMemory[i]) != VK_SUCCESS) {
            spdlog::error("Failed to allocate fog uniform buffer memory {}", i);
            return Error(ErrorCode::OutOfMemory, "Failed to allocate fog uniform buffer memory");
        }

        vkBindBufferMemory(m_device, m_uniformBuffers[i], m_uniformMemory[i], 0);

        // 持久映射
        if (vkMapMemory(m_device, m_uniformMemory[i], 0, bufferSize, 0, &m_mappedMemory[i]) != VK_SUCCESS) {
            spdlog::error("Failed to map fog uniform buffer {}", i);
            return Error(ErrorCode::OperationFailed, "Failed to map fog uniform buffer");
        }
    }

    return Result<void>::ok();
}

Result<void> FogManager::_createDescriptorSets()
{
    m_descriptorSets.resize(m_maxFramesInFlight);

    std::vector<VkDescriptorSetLayout> layouts(m_maxFramesInFlight, m_layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = m_maxFramesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        spdlog::error("Failed to allocate fog descriptor sets");
        return Error(ErrorCode::OutOfMemory, "Failed to allocate fog descriptor sets");
    }

    // 更新描述符集
    for (u32 i = 0; i < m_maxFramesInFlight; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(FogUBO);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }

    return Result<void>::ok();
}

void FogManager::_destroyUniformBuffers()
{
    for (u32 i = 0; i < m_maxFramesInFlight; ++i) {
        if (m_mappedMemory[i]) {
            vkUnmapMemory(m_device, m_uniformMemory[i]);
            m_mappedMemory[i] = nullptr;
        }
        if (m_uniformMemory[i]) {
            vkFreeMemory(m_device, m_uniformMemory[i], nullptr);
            m_uniformMemory[i] = VK_NULL_HANDLE;
        }
        if (m_uniformBuffers[i]) {
            vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
            m_uniformBuffers[i] = VK_NULL_HANDLE;
        }
    }

    m_uniformBuffers.clear();
    m_uniformMemory.clear();
    m_mappedMemory.clear();
}

Result<u32> FogManager::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::NotFound, "Failed to find suitable memory type");
}

} // namespace mc::client::renderer::trident::fog
