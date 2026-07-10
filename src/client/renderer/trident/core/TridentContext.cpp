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

#include "TridentContext.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <set>
#include <spdlog/spdlog.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace mc::trace;

namespace mc::client::renderer::trident {

// Vulkan 调试回调
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            // 详细和信息级别的验证层消息过于冗长，不输出
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn("[Vulkan] {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error("[Vulkan] {}", pCallbackData->pMessage);
            break;
        default:
            spdlog::info("[Vulkan] {}", pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

// ============================================================================
// 构造/析构
// ============================================================================

TridentContext::TridentContext() = default;

TridentContext::~TridentContext()
{
    destroy();
}

TridentContext::TridentContext(TridentContext&& other) noexcept
    : m_instance(other.m_instance)
    , m_debugMessenger(other.m_debugMessenger)
    , m_physicalDevice(other.m_physicalDevice)
    , m_device(other.m_device)
    , m_surface(other.m_surface)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_presentQueue(other.m_presentQueue)
    , m_transferQueue(other.m_transferQueue)
    , m_computeQueue(other.m_computeQueue)
    , m_queueFamilies(other.m_queueFamilies)
    , m_deviceProperties(other.m_deviceProperties)
    , m_deviceFeatures(other.m_deviceFeatures)
    , m_memoryProperties(other.m_memoryProperties)
    , m_config(other.m_config)
    , m_validationEnabled(other.m_validationEnabled)
    , m_initialized(other.m_initialized)
{
    other.m_instance = VK_NULL_HANDLE;
    other.m_debugMessenger = VK_NULL_HANDLE;
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_device = VK_NULL_HANDLE;
    other.m_surface = VK_NULL_HANDLE;
    other.m_initialized = false;
}

TridentContext& TridentContext::operator=(TridentContext&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_instance = other.m_instance;
        m_debugMessenger = other.m_debugMessenger;
        m_physicalDevice = other.m_physicalDevice;
        m_device = other.m_device;
        m_surface = other.m_surface;
        m_graphicsQueue = other.m_graphicsQueue;
        m_presentQueue = other.m_presentQueue;
        m_transferQueue = other.m_transferQueue;
        m_computeQueue = other.m_computeQueue;
        m_queueFamilies = other.m_queueFamilies;
        m_deviceProperties = other.m_deviceProperties;
        m_deviceFeatures = other.m_deviceFeatures;
        m_memoryProperties = other.m_memoryProperties;
        m_config = other.m_config;
        m_validationEnabled = other.m_validationEnabled;
        m_initialized = other.m_initialized;

        other.m_instance = VK_NULL_HANDLE;
        other.m_debugMessenger = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
        other.m_surface = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> TridentContext::initialize(GLFWwindow* window, const TridentConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::initialize");

    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "TridentContext already initialized");
    }

    m_config = config;

    // 创建 Instance
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::initialize::CreateInstance");
        auto instanceResult = createInstanceOnly(config);
        if (instanceResult.failed()) {
            return instanceResult.error();
        }
    }

    // 创建 Surface
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::initialize::CreateSurface");
        VkSurfaceKHR surface;
        VkResult result = glfwCreateWindowSurface(m_instance, window, nullptr, &surface);
        if (result != VK_SUCCESS) {
            destroy();
            return Error(ErrorCode::OperationFailed, "Failed to create window surface: " + std::to_string(result));
        }
        setSurface(surface);
    }

    // 创建设备
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::initialize::CreateDevice");
        auto deviceResult = createDevice();
        if (deviceResult.failed()) {
            destroy();
            return deviceResult.error();
        }
    }

    m_initialized = true;
    spdlog::info("TridentContext initialized successfully");
    spdlog::info("Device: {}", m_deviceProperties.deviceName);
    return {};
}

Result<void> TridentContext::createInstanceOnly(const TridentConfig& config)
{
    m_config = config;

    // 检查验证层支持
    m_validationEnabled = config.enableValidation && _checkValidationLayerSupport();

    // 获取所需扩展
    auto extensions = _getRequiredInstanceExtensions();

    // 应用信息
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = config.appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = config.engineName.c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // 创建 Instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef __APPLE__
    // macOS 使用 MoltenVK，需要启用 portability enumeration
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // 验证层
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_validationEnabled && !config.requiredLayers.empty()) {
        // 需要转换 std::string vector 到 const char* 数组
        std::vector<const char*> layers;
        layers.reserve(config.requiredLayers.size());
        for (const auto& layer : config.requiredLayers) {
            layers.push_back(layer.c_str());
        }
        createInfo.enabledLayerCount = static_cast<u32>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();

        // 设置调试消息处理器
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        createInfo.pNext = &debugCreateInfo;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create Vulkan instance: " + std::to_string(result));
    }

    // 设置调试消息处理器
    if (m_validationEnabled) {
        auto messengerResult = _setupDebugMessenger();
        if (messengerResult.failed()) {
            spdlog::warn("Failed to set up debug messenger: {}", messengerResult.error().message());
        }
    }

    spdlog::info("Vulkan instance created");
    return {};
}

void TridentContext::setSurface(VkSurfaceKHR surface)
{
    m_surface = surface;
}

Result<void> TridentContext::createDevice()
{
    if (m_instance == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotInitialized, "Instance not created");
    }
    if (m_surface == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotInitialized, "Surface not set");
    }

    // 选择物理设备
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::createDevice::PickPhysicalDevice");
        auto physicalResult = _pickPhysicalDevice();
        if (physicalResult.failed()) {
            return physicalResult.error();
        }
    }

    // 创建逻辑设备
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentContext::createDevice::CreateLogicalDevice");
        auto deviceResult = _createLogicalDevice();
        if (deviceResult.failed()) {
            return deviceResult.error();
        }
    }

    // 获取队列
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::createDevice::GetQueues");
        vkGetDeviceQueue(m_device, m_queueFamilies.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilies.presentFamily.value(), 0, &m_presentQueue);

        if (m_queueFamilies.hasTransfer()) {
            vkGetDeviceQueue(m_device, m_queueFamilies.transferFamily.value(), 0, &m_transferQueue);
        } else {
            m_transferQueue = m_graphicsQueue;
        }

        if (m_queueFamilies.hasCompute()) {
            vkGetDeviceQueue(m_device, m_queueFamilies.computeFamily.value(), 0, &m_computeQueue);
        } else {
            m_computeQueue = m_graphicsQueue;
        }
    }

    // 创建命令池（用于单次命令）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentContext::createDevice::CreateCommandPool");
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily.value();

        VkResult poolResult = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
        if (poolResult != VK_SUCCESS) {
            return Error(ErrorCode::OperationFailed, "Failed to create command pool");
        }
    }

    spdlog::info("Vulkan device created");
    return {};
}

void TridentContext::destroy()
{
    if (!m_initialized && m_instance == VK_NULL_HANDLE) return;

    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    m_initialized = false;
    spdlog::info("TridentContext destroyed");
}

// ============================================================================
// 访问器
// ============================================================================

SwapChainSupportDetails TridentContext::querySwapChainSupport() const
{
    return _querySwapChainSupport(m_physicalDevice);
}

Result<u32> TridentContext::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const
{
    for (u32 i = 0; i < m_memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return Error(ErrorCode::NotFound, "Failed to find suitable memory type");
}

Result<VkFormat> TridentContext::findSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return Error(ErrorCode::NotFound, "Failed to find supported format");
}

Result<VkFormat> TridentContext::findDepthFormat() const
{
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits TridentContext::maxUsableSampleCount() const
{
    const VkSampleCountFlags sampleCounts =
        m_deviceProperties.limits.framebufferColorSampleCounts & m_deviceProperties.limits.framebufferDepthSampleCounts;

    if (sampleCounts & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (sampleCounts & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (sampleCounts & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (sampleCounts & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (sampleCounts & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (sampleCounts & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

void TridentContext::waitIdle() const
{
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
}

VkCommandBuffer TridentContext::beginSingleTimeCommands() const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void TridentContext::endSingleTimeCommands(VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);

    // 使用 fence 替代 vkQueueWaitIdle，避免阻塞整个 GPU 队列
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence;
    vkCreateFence(m_device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence);
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device, fence, nullptr);
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> TridentContext::_createInstance()
{
    // 由 createInstanceOnly 处理
    return createInstanceOnly(m_config);
}

Result<void> TridentContext::_setupDebugMessenger()
{
    if (!m_validationEnabled) return {};

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");

    if (func == nullptr) {
        return Error(ErrorCode::NotFound, "Failed to find vkCreateDebugUtilsMessengerEXT");
    }

    VkResult result = func(m_instance, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to set up debug messenger: " + std::to_string(result));
    }

    return {};
}

Result<void> TridentContext::_pickPhysicalDevice()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return Error(ErrorCode::NotFound, "Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (_isDeviceSuitable(device)) {
            m_physicalDevice = device;
            vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
            vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures);
            vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
            m_queueFamilies = _findQueueFamilies(device);
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotFound, "Failed to find a suitable GPU");
    }

    return {};
}

Result<void> TridentContext::_createLogicalDevice()
{
    QueueFamilyIndices indices = _findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 获取设备扩展
    auto extensions = _getRequiredDeviceExtensions();

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
#ifndef __APPLE__
    // macOS 使用 MoltenVK/Metal，不支持 shaderFloat64
    deviceFeatures.shaderFloat64 = VK_TRUE;
#endif

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create logical device: " + std::to_string(result));
    }

    return {};
}

// ============================================================================
// 私有方法 - 辅助
// ============================================================================

bool TridentContext::_checkValidationLayerSupport()
{
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& requiredLayer : m_config.requiredLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(requiredLayer.c_str(), layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }

    return true;
}

bool TridentContext::_checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(
        m_config.requiredDeviceExtensions.begin(), m_config.requiredDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool TridentContext::_isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = _findQueueFamilies(device);

    bool extensionsSupported = _checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = _querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    bool suitable =
        indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;

#ifdef __APPLE__
    // macOS 使用 MoltenVK/Metal，不支持 shaderFloat64，跳过此检查
    (void)supportedFeatures.shaderFloat64;
#else
    suitable = suitable && supportedFeatures.shaderFloat64;
#endif

    return suitable;
}

QueueFamilyIndices TridentContext::_findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    u32 i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

SwapChainSupportDetails TridentContext::_querySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

std::vector<const char*> TridentContext::_getRequiredInstanceExtensions()
{
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

#ifdef __APPLE__
    // macOS 使用 MoltenVK，需要启用 portability enumeration 扩展
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    return extensions;
}

std::vector<const char*> TridentContext::_getRequiredDeviceExtensions()
{
    std::vector<const char*> extensions;
    extensions.reserve(m_config.requiredDeviceExtensions.size() + m_config.optionalDeviceExtensions.size());

    for (const auto& ext : m_config.requiredDeviceExtensions) {
        extensions.push_back(ext.c_str());
    }

    // 添加可选扩展
    for (const auto& ext : m_config.optionalDeviceExtensions) {
        // 检查是否支持
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        for (const auto& available : availableExtensions) {
            if (strcmp(ext.c_str(), available.extensionName) == 0) {
                extensions.push_back(ext.c_str());
                break;
            }
        }
    }

    return extensions;
}

} // namespace mc::client::renderer::trident
