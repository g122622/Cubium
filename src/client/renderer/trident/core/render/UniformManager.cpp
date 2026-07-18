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

#include "UniformManager.hpp"
#include "DescriptorManager.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "common/core/Constants.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

UniformManager::UniformManager() = default;

UniformManager::~UniformManager()
{
    destroy();
}

UniformManager::UniformManager(UniformManager&& other) noexcept
    : m_context(other.m_context)
    , m_descriptor(other.m_descriptor)
    , m_cameraBuffers(std::move(other.m_cameraBuffers))
    , m_cameraBufferMemory(std::move(other.m_cameraBufferMemory))
    , m_cameraBufferMapped(std::move(other.m_cameraBufferMapped))
    , m_lightingBuffer(other.m_lightingBuffer)
    , m_lightingBufferMemory(other.m_lightingBufferMemory)
    , m_lightingBufferMapped(other.m_lightingBufferMapped)
    , m_cameraDescriptorSets(std::move(other.m_cameraDescriptorSets))
    , m_maxFramesInFlight(other.m_maxFramesInFlight)
    , m_initialized(other.m_initialized)
{
    other.m_context = nullptr;
    other.m_descriptor = nullptr;
    other.m_lightingBuffer = VK_NULL_HANDLE;
    other.m_lightingBufferMemory = VK_NULL_HANDLE;
    other.m_lightingBufferMapped = nullptr;
    other.m_initialized = false;
}

UniformManager& UniformManager::operator=(UniformManager&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_descriptor = other.m_descriptor;
        m_cameraBuffers = std::move(other.m_cameraBuffers);
        m_cameraBufferMemory = std::move(other.m_cameraBufferMemory);
        m_cameraBufferMapped = std::move(other.m_cameraBufferMapped);
        m_lightingBuffer = other.m_lightingBuffer;
        m_lightingBufferMemory = other.m_lightingBufferMemory;
        m_lightingBufferMapped = other.m_lightingBufferMapped;
        m_cameraDescriptorSets = std::move(other.m_cameraDescriptorSets);
        m_maxFramesInFlight = other.m_maxFramesInFlight;
        m_initialized = other.m_initialized;

        other.m_context = nullptr;
        other.m_descriptor = nullptr;
        other.m_lightingBuffer = VK_NULL_HANDLE;
        other.m_lightingBufferMemory = VK_NULL_HANDLE;
        other.m_lightingBufferMapped = nullptr;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> UniformManager::initialize(TridentContext* context, DescriptorManager* descriptor, u32 maxFramesInFlight)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "UniformManager already initialized");
    }

    if (!context || !descriptor) {
        return Error(ErrorCode::NullPointer, "Context or descriptor is null");
    }

    m_context = context;
    m_descriptor = descriptor;
    m_maxFramesInFlight = maxFramesInFlight;

    // 创建 Uniform 缓冲区
    auto bufferResult = _createUniformBuffers();
    if (bufferResult.failed()) {
        return bufferResult.error();
    }

    // 创建相机描述符集
    auto descriptorResult = _createCameraDescriptorSets();
    if (descriptorResult.failed()) {
        _destroyUniformBuffers();
        return descriptorResult.error();
    }

    m_initialized = true;
    spdlog::info("UniformManager initialized");
    return {};
}

void UniformManager::destroy()
{
    if (!m_initialized) return;

    _destroyUniformBuffers();

    m_context = nullptr;
    m_descriptor = nullptr;
    m_initialized = false;
    spdlog::info("UniformManager destroyed");
}

// ============================================================================
// 更新方法
// ============================================================================

void UniformManager::updateCamera(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, u32 frameIndex)
{
    MC_ASSERT_RELEASE(m_initialized && frameIndex < m_maxFramesInFlight);

    CameraUBO ubo{};
    ubo.view = viewMatrix;
    ubo.projection = projectionMatrix;
    ubo.viewProjection = projectionMatrix * viewMatrix;

    std::memcpy(m_cameraBufferMapped[frameIndex], &ubo, sizeof(CameraUBO));
}

void UniformManager::updateLighting(i64 dayTime, i64 gameTime, f64 partialTick)
{
    MC_ASSERT_RELEASE(m_initialized && m_lightingBufferMapped);

    // 计算太阳方向
    // dayTime: 0=日出, 6000=正午, 12000=日落, 18000=午夜
    // 因此太阳高度应采用 sin 映射：
    // sin(0)=0, sin(pi/2)=1, sin(pi)≈0, sin(3pi/2)=-1
    const f64 sunAngle = static_cast<f64>(dayTime % static_cast<i64>(mc::game::DAY_LENGTH_TICKS)) /
        static_cast<f64>(mc::game::DAY_LENGTH_TICKS) * 2.0 * mc::math::PI_DOUBLE;
    const f64 sunHeight = std::sin(sunAngle);
    const f64 sunHoriz = std::cos(sunAngle);

    LightingUBO ubo{};
    ubo.sunDirection = glm::normalize(glm::vec3(static_cast<f32>(sunHoriz), static_cast<f32>(sunHeight), 0.0f));
    ubo.sunIntensity = static_cast<f32>(std::max(sunHeight, 0.0));
    ubo.moonDirection = -ubo.sunDirection;
    ubo.moonIntensity = static_cast<f32>(std::max(-sunHeight, 0.0));
    ubo.dayTime = static_cast<f32>(dayTime);
    ubo.gameTime = static_cast<f32>(static_cast<f64>(gameTime) + partialTick);

    std::memcpy(m_lightingBufferMapped, &ubo, sizeof(LightingUBO));
}

VkBuffer UniformManager::cameraBuffer(u32 frameIndex) const
{
    MC_ASSERT_RELEASE(frameIndex < m_cameraBuffers.size());
    return m_cameraBuffers[frameIndex];
}

VkDescriptorSet UniformManager::cameraDescriptorSet(u32 frameIndex) const
{
    MC_ASSERT_RELEASE(frameIndex < m_cameraDescriptorSets.size());
    return m_cameraDescriptorSets[frameIndex];
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> UniformManager::_createUniformBuffers()
{
    VkDevice device = m_context->device();

    m_cameraBuffers.resize(m_maxFramesInFlight);
    m_cameraBufferMemory.resize(m_maxFramesInFlight);
    m_cameraBufferMapped.resize(m_maxFramesInFlight, nullptr);

    VkDeviceSize cameraBufferSize = sizeof(CameraUBO);
    VkDeviceSize lightingBufferSize = sizeof(LightingUBO);

    // 创建相机缓冲区
    for (u32 i = 0; i < m_maxFramesInFlight; i++) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = cameraBufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_cameraBuffers[i]);
        if (result != VK_SUCCESS) {
            _destroyUniformBuffers();
            return Error(ErrorCode::OutOfMemory, "Failed to create camera uniform buffer: " + std::to_string(result));
        }

        // 分配内存
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_cameraBuffers[i], &memRequirements);

        auto typeResult = m_context->findMemoryType(
            memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (typeResult.failed()) {
            _destroyUniformBuffers();
            return typeResult.error();
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = typeResult.value();

        result = vkAllocateMemory(device, &allocInfo, nullptr, &m_cameraBufferMemory[i]);
        if (result != VK_SUCCESS) {
            _destroyUniformBuffers();
            return Error(
                ErrorCode::OutOfMemory, "Failed to allocate camera uniform buffer memory: " + std::to_string(result));
        }

        vkBindBufferMemory(device, m_cameraBuffers[i], m_cameraBufferMemory[i], 0);

        // 映射内存
        result = vkMapMemory(device, m_cameraBufferMemory[i], 0, cameraBufferSize, 0, &m_cameraBufferMapped[i]);
        if (result != VK_SUCCESS || m_cameraBufferMapped[i] == nullptr) {
            _destroyUniformBuffers();
            return Error(ErrorCode::OperationFailed, "Failed to map camera uniform buffer: " + std::to_string(result));
        }
    }

    // 创建光照缓冲区
    VkBufferCreateInfo lightingBufferInfo{};
    lightingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    lightingBufferInfo.size = lightingBufferSize;
    lightingBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    lightingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &lightingBufferInfo, nullptr, &m_lightingBuffer);
    if (result != VK_SUCCESS) {
        _destroyUniformBuffers();
        return Error(ErrorCode::OutOfMemory, "Failed to create lighting uniform buffer: " + std::to_string(result));
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_lightingBuffer, &memRequirements);

    auto typeResult = m_context->findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (typeResult.failed()) {
        _destroyUniformBuffers();
        return typeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = typeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_lightingBufferMemory);
    if (result != VK_SUCCESS) {
        _destroyUniformBuffers();
        return Error(
            ErrorCode::OutOfMemory, "Failed to allocate lighting uniform buffer memory: " + std::to_string(result));
    }

    vkBindBufferMemory(device, m_lightingBuffer, m_lightingBufferMemory, 0);

    result = vkMapMemory(device, m_lightingBufferMemory, 0, lightingBufferSize, 0, &m_lightingBufferMapped);
    if (result != VK_SUCCESS || m_lightingBufferMapped == nullptr) {
        _destroyUniformBuffers();
        return Error(ErrorCode::OperationFailed, "Failed to map lighting uniform buffer: " + std::to_string(result));
    }

    return {};
}

Result<void> UniformManager::_createCameraDescriptorSets()
{
    m_cameraDescriptorSets.resize(m_maxFramesInFlight);

    for (u32 i = 0; i < m_maxFramesInFlight; i++) {
        auto setResult = m_descriptor->allocateCameraSet(i);
        if (setResult.failed()) {
            return setResult.error();
        }
        m_cameraDescriptorSets[i] = setResult.value();

        // 更新描述符集
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_cameraBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraUBO);

        VkDescriptorBufferInfo lightingInfo{};
        lightingInfo.buffer = m_lightingBuffer;
        lightingInfo.offset = 0;
        lightingInfo.range = sizeof(LightingUBO);

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_cameraDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_cameraDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &lightingInfo;

        vkUpdateDescriptorSets(
            m_context->device(), static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return {};
}

void UniformManager::_destroyUniformBuffers()
{
    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;

    for (u32 i = 0; i < m_cameraBuffers.size(); i++) {
        if (m_cameraBufferMapped[i] && device != VK_NULL_HANDLE) {
            vkUnmapMemory(device, m_cameraBufferMemory[i]);
            m_cameraBufferMapped[i] = nullptr;
        }
        if (m_cameraBuffers[i] != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_cameraBuffers[i], nullptr);
        }
        if (m_cameraBufferMemory[i] != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_cameraBufferMemory[i], nullptr);
        }
    }
    m_cameraBuffers.clear();
    m_cameraBufferMemory.clear();
    m_cameraBufferMapped.clear();

    if (m_lightingBufferMapped && device != VK_NULL_HANDLE) {
        vkUnmapMemory(device, m_lightingBufferMemory);
        m_lightingBufferMapped = nullptr;
    }
    if (m_lightingBuffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_lightingBuffer, nullptr);
        m_lightingBuffer = VK_NULL_HANDLE;
    }
    if (m_lightingBufferMemory != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_lightingBufferMemory, nullptr);
        m_lightingBufferMemory = VK_NULL_HANDLE;
    }
}

} // namespace mc::client::renderer::trident
