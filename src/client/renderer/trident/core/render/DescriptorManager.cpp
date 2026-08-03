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

#include "DescriptorManager.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <string>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

DescriptorManager::DescriptorManager() = default;

DescriptorManager::~DescriptorManager()
{
    destroy();
}

DescriptorManager::DescriptorManager(DescriptorManager&& other) noexcept
    : m_context(other.m_context)
    , m_cameraLayout(other.m_cameraLayout)
    , m_textureLayout(other.m_textureLayout)
    , m_fogLayout(other.m_fogLayout)
    , m_pipelineLayout(other.m_pipelineLayout)
    , m_pool(other.m_pool)
    , m_maxFramesInFlight(other.m_maxFramesInFlight)
    , m_initialized(other.m_initialized)
{
    other.m_context = nullptr;
    other.m_cameraLayout = VK_NULL_HANDLE;
    other.m_textureLayout = VK_NULL_HANDLE;
    other.m_fogLayout = VK_NULL_HANDLE;
    other.m_pipelineLayout = VK_NULL_HANDLE;
    other.m_pool = VK_NULL_HANDLE;
    other.m_initialized = false;
}

DescriptorManager& DescriptorManager::operator=(DescriptorManager&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_cameraLayout = other.m_cameraLayout;
        m_textureLayout = other.m_textureLayout;
        m_fogLayout = other.m_fogLayout;
        m_pipelineLayout = other.m_pipelineLayout;
        m_pool = other.m_pool;
        m_maxFramesInFlight = other.m_maxFramesInFlight;
        m_initialized = other.m_initialized;

        other.m_context = nullptr;
        other.m_cameraLayout = VK_NULL_HANDLE;
        other.m_textureLayout = VK_NULL_HANDLE;
        other.m_fogLayout = VK_NULL_HANDLE;
        other.m_pipelineLayout = VK_NULL_HANDLE;
        other.m_pool = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> DescriptorManager::initialize(TridentContext* context, u32 maxFramesInFlight)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "DescriptorManager already initialized");
    }

    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_maxFramesInFlight = maxFramesInFlight;

    // 创建描述符集布局
    auto layoutResult = _createDescriptorSetLayouts();
    if (layoutResult.failed()) {
        return layoutResult.error();
    }

    // 创建管线布局
    auto pipelineResult = _createPipelineLayout();
    if (pipelineResult.failed()) {
        _destroyDescriptorSetLayouts();
        return pipelineResult.error();
    }

    // 创建描述符池
    auto poolResult = _createDescriptorPool();
    if (poolResult.failed()) {
        _destroyPipelineLayout();
        _destroyDescriptorSetLayouts();
        return poolResult.error();
    }

    m_initialized = true;
    spdlog::info("DescriptorManager initialized");
    return {};
}

void DescriptorManager::destroy()
{
    if (!m_initialized) return;

    _destroyDescriptorPool();
    _destroyPipelineLayout();
    _destroyDescriptorSetLayouts();

    m_context = nullptr;
    m_initialized = false;
    spdlog::info("DescriptorManager destroyed");
}

Result<VkDescriptorSet> DescriptorManager::allocateCameraSet(u32 frameIndex)
{
    (void)frameIndex; // TODO: 利用帧索引优化描述符集分配
    return _allocateSet(m_cameraLayout, "Failed to allocate camera descriptor set");
}

Result<VkDescriptorSet> DescriptorManager::allocateTextureSet()
{
    return _allocateSet(m_textureLayout, "Failed to allocate texture descriptor set");
}

Result<VkDescriptorSet> DescriptorManager::allocateFogSet()
{
    return _allocateSet(m_fogLayout, "Failed to allocate fog descriptor set");
}

// ============================================================================
// 私有方法 - 分配辅助
// ============================================================================

Result<VkDescriptorSet> DescriptorManager::_allocateSet(VkDescriptorSetLayout layout, const char* errorMsg)
{
    if (!m_initialized || m_pool == VK_NULL_HANDLE || layout == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotInitialized, "DescriptorManager not initialized");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(m_context->device(), &allocInfo, &descriptorSet);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, std::string(errorMsg) + ": " + std::to_string(result));
    }

    return descriptorSet;
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> DescriptorManager::_createDescriptorSetLayouts()
{
    VkDevice device = m_context->device();

    // Camera UBO 布局 (set = 0, binding = 0)
    VkDescriptorSetLayoutBinding cameraLayoutBinding{};
    cameraLayoutBinding.binding = 0;
    cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraLayoutBinding.descriptorCount = 1;
    cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    cameraLayoutBinding.pImmutableSamplers = nullptr;

    // Lighting UBO 布局 (set = 0, binding = 1)
    VkDescriptorSetLayoutBinding lightingLayoutBinding{};
    lightingLayoutBinding.binding = 1;
    lightingLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingLayoutBinding.descriptorCount = 1;
    lightingLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lightingLayoutBinding.pImmutableSamplers = nullptr;

    // 创建包含两个绑定的相机布局
    std::array<VkDescriptorSetLayoutBinding, 2> cameraBindings = {cameraLayoutBinding, lightingLayoutBinding};

    VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
    cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraLayoutInfo.bindingCount = static_cast<u32>(cameraBindings.size());
    cameraLayoutInfo.pBindings = cameraBindings.data();

    VkResult result = vkCreateDescriptorSetLayout(device, &cameraLayoutInfo, nullptr, &m_cameraLayout);
    if (result != VK_SUCCESS) {
        return Error(
            ErrorCode::OperationFailed, "Failed to create camera descriptor set layout: " + std::to_string(result));
    }

    // Texture sampler 布局 (set = 1, binding = 0)
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &samplerLayoutBinding;

    result = vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &m_textureLayout);
    if (result != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, m_cameraLayout, nullptr);
        m_cameraLayout = VK_NULL_HANDLE;
        return Error(
            ErrorCode::OperationFailed, "Failed to create texture descriptor set layout: " + std::to_string(result));
    }

    // Fog UBO 布局 (set = 2, binding = 0)
    VkDescriptorSetLayoutBinding fogLayoutBinding{};
    fogLayoutBinding.binding = 0;
    fogLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fogLayoutBinding.descriptorCount = 1;
    fogLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fogLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo fogLayoutInfo{};
    fogLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    fogLayoutInfo.bindingCount = 1;
    fogLayoutInfo.pBindings = &fogLayoutBinding;

    result = vkCreateDescriptorSetLayout(device, &fogLayoutInfo, nullptr, &m_fogLayout);
    if (result != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, m_cameraLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, m_textureLayout, nullptr);
        m_cameraLayout = VK_NULL_HANDLE;
        m_textureLayout = VK_NULL_HANDLE;
        return Error(
            ErrorCode::OperationFailed, "Failed to create fog descriptor set layout: " + std::to_string(result));
    }

    return {};
}

Result<void> DescriptorManager::_createPipelineLayout()
{
    VkDevice device = m_context->device();

    std::array<VkDescriptorSetLayout, 3> setLayouts = {m_cameraLayout, m_textureLayout, m_fogLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<u32>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create pipeline layout: " + std::to_string(result));
    }

    return {};
}

Result<void> DescriptorManager::_createDescriptorPool()
{
    VkDevice device = m_context->device();

    // 描述符池大小
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<u32>(m_maxFramesInFlight * 3 + 20); // 相机 + 光照 + 雾
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 100; // 纹理采样器

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = static_cast<u32>(m_maxFramesInFlight * 3 + 100);
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_pool);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create descriptor pool: " + std::to_string(result));
    }

    return {};
}

// ============================================================================
// 私有方法 - 销毁
// ============================================================================

void DescriptorManager::_destroyDescriptorSetLayouts()
{
    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;

    if (m_cameraLayout != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_cameraLayout, nullptr);
        m_cameraLayout = VK_NULL_HANDLE;
    }

    if (m_textureLayout != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_textureLayout, nullptr);
        m_textureLayout = VK_NULL_HANDLE;
    }

    if (m_fogLayout != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_fogLayout, nullptr);
        m_fogLayout = VK_NULL_HANDLE;
    }
}

void DescriptorManager::_destroyPipelineLayout()
{
    if (m_pipelineLayout != VK_NULL_HANDLE && m_context) {
        vkDestroyPipelineLayout(m_context->device(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}

void DescriptorManager::_destroyDescriptorPool()
{
    if (m_pool != VK_NULL_HANDLE && m_context) {
        vkDestroyDescriptorPool(m_context->device(), m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

} // namespace mc::client::renderer::trident
