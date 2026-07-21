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

#include "client/renderer/api/TridentApi.hpp"
#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

// 前置声明 GLFW
struct GLFWwindow;

namespace mc::client::renderer::trident {

/**
 * @brief Trident 渲染引擎配置
 */
struct TridentConfig {
    std::string appName = "Trident";
    std::string engineName = "Trident Engine";
    bool enableValidation = true;
    bool enableVSync = true;
    u32 maxFramesInFlight = 2;

    // 扩展和层配置
    std::vector<std::string> requiredDeviceExtensions;
    std::vector<std::string> optionalDeviceExtensions;
    std::vector<std::string> requiredLayers;

    TridentConfig()
    {
        // 默认必需的设备扩展
        requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // 默认可选的设备扩展
        optionalDeviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

#ifdef _DEBUG
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    }
};

/**
 * @brief Vulkan 版本信息
 */
struct VulkanVersion {
    u32 major;
    u32 minor;
    u32 patch;

    std::string toString() const
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

/**
 * @brief 队列族索引
 */
struct QueueFamilyIndices {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    std::optional<u32> transferFamily;
    std::optional<u32> computeFamily;

    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }

    bool hasTransfer() const { return transferFamily.has_value(); }
    bool hasCompute() const { return computeFamily.has_value(); }
};

/**
 * @brief 交换链支持详情
 */
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @brief Trident Vulkan 上下文
 *
 * 管理 Vulkan 实例、设备和队列。
 * 是从 VulkanContext 重命名并移动到 trident 命名空间。
 */
class TridentContext {
public:
    TridentContext();
    ~TridentContext();

    // 禁止拷贝
    TridentContext(const TridentContext&) = delete;
    TridentContext& operator=(const TridentContext&) = delete;

    // 允许移动
    TridentContext(TridentContext&& other) noexcept;
    TridentContext& operator=(TridentContext&& other) noexcept;

    // ========================================================================
    // 初始化（完整流程）
    // ========================================================================

    /**
     * @brief 完整初始化
     * @param window GLFW 窗口
     * @param config 配置
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(GLFWwindow* window, const TridentConfig& config);

    /**
     * @brief 分阶段初始化 - 第一步：创建 instance
     */
    [[nodiscard]] Result<void> createInstanceOnly(const TridentConfig& config);

    /**
     * @brief 分阶段初始化 - 第二步：设置 surface
     */
    void setSurface(VkSurfaceKHR surface);

    /**
     * @brief 分阶段初始化 - 第三步：创建设备
     */
    [[nodiscard]] Result<void> createDevice();

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    // ========================================================================
    // 访问器
    // ========================================================================

    [[nodiscard]] VkInstance instance() const { return m_instance; }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    [[nodiscard]] VkDevice device() const { return m_device; }
    [[nodiscard]] VkSurfaceKHR surface() const { return m_surface; }

    [[nodiscard]] VkQueue graphicsQueue() const { return m_graphicsQueue; }
    [[nodiscard]] VkQueue presentQueue() const { return m_presentQueue; }
    [[nodiscard]] VkQueue transferQueue() const { return m_transferQueue; }
    [[nodiscard]] VkQueue computeQueue() const { return m_computeQueue; }

    [[nodiscard]] const QueueFamilyIndices& queueFamilies() const { return m_queueFamilies; }

    [[nodiscard]] const VkPhysicalDeviceProperties& deviceProperties() const { return m_deviceProperties; }
    [[nodiscard]] const VkPhysicalDeviceFeatures& deviceFeatures() const { return m_deviceFeatures; }
    [[nodiscard]] const VkPhysicalDeviceMemoryProperties& memoryProperties() const { return m_memoryProperties; }

    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport() const;

    // ========================================================================
    // 工具函数
    // ========================================================================

    /**
     * @brief 查找合适的内存类型
     */
    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;

    /**
     * @brief 查找支持的格式
     */
    [[nodiscard]] Result<VkFormat> findSupportedFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    /**
     * @brief 查找深度格式
     */
    [[nodiscard]] Result<VkFormat> findDepthFormat() const;

    /**
     * @brief 获取物理设备支持的最高多重采样等级
     */
    [[nodiscard]] VkSampleCountFlagBits maxUsableSampleCount() const;

    /**
     * @brief 等待设备空闲
     */
    void waitIdle() const;

    /**
     * @brief 开始单次命令缓冲区
     * @return 命令缓冲区
     */
    [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;

    /**
     * @brief 结束单次命令缓冲区并提交
     * @param commandBuffer 命令缓冲区
     */
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    /**
     * @brief 获取统一暂存缓冲池
     *
     * 由 TridentEngine 在初始化时注入，供所有上传路径（图集子区域、顶点/索引缓冲、
     * mega-buffer 段等）复用，消除散落的"每次上传新建 staging buffer"反模式。
     * 初始化完成前返回 nullptr。
     */
    [[nodiscard]] api::IStagingBufferPool* stagingPool() const { return m_stagingPool; }

    /**
     * @brief 注入统一暂存缓冲池（由 TridentEngine 调用）
     */
    void setStagingPool(api::IStagingBufferPool* pool) { m_stagingPool = pool; }

    /**
     * @brief 检查是否启用验证层
     */
    [[nodiscard]] bool isValidationEnabled() const { return m_validationEnabled; }

    /**
     * @brief 获取调试消息处理器
     */
    [[nodiscard]] VkDebugUtilsMessengerEXT debugMessenger() const { return m_debugMessenger; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const TridentConfig& config() const { return m_config; }

private:
    // 创建方法
    [[nodiscard]] Result<void> _createInstance();
    [[nodiscard]] Result<void> _setupDebugMessenger();
    [[nodiscard]] Result<void> _pickPhysicalDevice();
    [[nodiscard]] Result<void> _createLogicalDevice();

    // 辅助方法
    bool _checkValidationLayerSupport();
    bool _checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool _isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails _querySwapChainSupport(VkPhysicalDevice device) const;
    std::vector<const char*> _getRequiredInstanceExtensions();
    std::vector<const char*> _getRequiredDeviceExtensions();

    // Vulkan 对象
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE; // 单次命令池

    // 统一暂存缓冲池（由 TridentEngine 拥有并注入，Context 不负责其生命周期）
    api::IStagingBufferPool* m_stagingPool = nullptr;

    // 队列
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;

    // 队列族索引
    QueueFamilyIndices m_queueFamilies;

    // 设备属性
    VkPhysicalDeviceProperties m_deviceProperties{};
    VkPhysicalDeviceFeatures m_deviceFeatures{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};

    // 配置
    TridentConfig m_config;
    bool m_validationEnabled = false;
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
