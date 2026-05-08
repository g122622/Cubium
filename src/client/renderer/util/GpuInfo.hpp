#pragma once

#include "common/core/Types.hpp"
#include <vulkan/vulkan.h>
#include <string>

namespace mc::client {

/**
 * @brief GPU信息结构
 *
 * 用于存储从Vulkan获取的GPU信息
 */
struct DebugGpuInfo {
    std::string vendor;              // GPU厂商 (NVIDIA, AMD, Intel等)
    std::string name;                // GPU型号
    std::string driverVersion;       // 驱动版本
    u64 dedicatedVideoMB = 0;   // 专用显存 (MB)
    u64 sharedSystemMB = 0;     // 共享系统内存 (MB)
    u32 apiMajorVersion = 0;    // Vulkan API主版本
    u32 apiMinorVersion = 0;    // Vulkan API次版本
};

/**
 * @brief 从 Vulkan 设备属性提取 GPU 信息
 *
 * @param deviceProperties Vulkan 物理设备属性
 * @param memoryProperties Vulkan 物理设备内存属性
 * @return 提取的 GPU 信息
 */
inline DebugGpuInfo getGpuInfo(
    const VkPhysicalDeviceProperties& deviceProperties,
    const VkPhysicalDeviceMemoryProperties& memoryProperties)
{
    DebugGpuInfo gpuInfo;

    // 设备名称
    gpuInfo.name = deviceProperties.deviceName;

    // API 版本
    gpuInfo.apiMajorVersion = VK_API_VERSION_MAJOR(deviceProperties.apiVersion);
    gpuInfo.apiMinorVersion = VK_API_VERSION_MINOR(deviceProperties.apiVersion);

    // 驱动版本
    gpuInfo.driverVersion =
        std::to_string(VK_API_VERSION_MAJOR(deviceProperties.driverVersion)) + "." +
        std::to_string(VK_API_VERSION_MINOR(deviceProperties.driverVersion)) + "." +
        std::to_string(VK_API_VERSION_PATCH(deviceProperties.driverVersion));

    // 内存统计
    u64 dedicatedVideoMemory = 0;
    u64 sharedSystemMemory = 0;
    for (u32 i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        const auto& heap = memoryProperties.memoryHeaps[i];
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            dedicatedVideoMemory += heap.size;
        } else {
            sharedSystemMemory += heap.size;
        }
    }
    gpuInfo.dedicatedVideoMB = dedicatedVideoMemory / (1024 * 1024);
    gpuInfo.sharedSystemMB = sharedSystemMemory / (1024 * 1024);

    // 厂商识别
    switch (deviceProperties.vendorID) {
        case 0x10DE: gpuInfo.vendor = "NVIDIA"; break;
        case 0x1002:
        case 0x1022: gpuInfo.vendor = "AMD"; break;
        case 0x8086:
        case 0x8087: gpuInfo.vendor = "Intel"; break;
        case 0x13B5: gpuInfo.vendor = "ARM"; break;
        case 0x1010: gpuInfo.vendor = "Apple"; break;
        case 0x5143: gpuInfo.vendor = "Qualcomm"; break;
        default: gpuInfo.vendor = "Unknown"; break;
    }

    return gpuInfo;
}

} // namespace mc::client
