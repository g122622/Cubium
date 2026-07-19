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

#include "../core/Types.hpp"
#include <string>

namespace mc::util {

/**
 * @brief 内存信息
 */
struct MemoryInfo {
    u64 totalPhysicalMB;     // 总物理内存 (MB)
    u64 availablePhysicalMB; // 可用物理内存 (MB)
    u64 usedPhysicalMB;      // 已用物理内存 (MB)
    u64 processUsedMB;       // 进程已用内存 (MB)
    u64 processPeakMB;       // 进程峰值内存 (MB)
    u32 usagePercent;        // 使用百分比
};

/**
 * @brief CPU信息
 */
struct CpuInfo {
    std::string vendor; // CPU厂商
    std::string brand;  // CPU品牌/型号
    u32 coreCount;      // 核心数量
    u32 threadCount;    // 线程数量
    u32 clockSpeedMHz;  // 时钟速度 (MHz)
    bool is64Bit;       // 是否64位
};

/**
 * @brief GPU信息
 */
struct GpuInfo {
    std::string vendor;        // GPU厂商 (NVIDIA, AMD, Intel等)
    std::string name;          // GPU型号
    std::string driverVersion; // 驱动版本
    u64 dedicatedVideoMB;      // 专用显存 (MB)
    u64 sharedSystemMB;        // 共享系统内存 (MB)
    u32 apiMajorVersion;       // 图形API主版本号
    u32 apiMinorVersion;       // 图形API次版本号
};

/**
 * @brief 平台信息工具类
 *
 * 提供跨平台的系统信息获取功能：
 * - 内存信息 (总内存、已用内存)
 * - CPU信息 (厂商、型号、核心数)
 * - GPU信息 (厂商、型号、显存)
 * - 显示器信息 (分辨率、刷新率)
 *
 * 参考 MC 1.16.5 PlatformDescriptors
 */
class PlatformInfo {
public:
    /**
     * @brief 获取内存信息
     *
     * Windows: 使用 GlobalMemoryStatusEx
     * Linux: 读取 /proc/meminfo
     * macOS: 使用 sysctl
     */
    static MemoryInfo getMemoryInfo();

    /**
     * @brief 获取CPU信息
     *
     * Windows: 使用 GetSystemInfo + __cpuid
     * Linux: 读取 /proc/cpuinfo
     * macOS: 使用 sysctl
     */
    static CpuInfo getCpuInfo();

    /**
     * @brief 获取当前进程内存使用量 (MB)
     *
     * Windows: 使用 GetProcessMemoryInfo
     * Linux: 读取 /proc/self/status
     * macOS: 使用 task_info
     */
    static u64 getProcessMemoryMB();

    /**
     * @brief 获取当前进程峰值内存使用量 (MB)
     *
     * Windows: 使用 GetProcessMemoryInfo (PeakWorkingSetSize)
     * Linux: 读取 /proc/self/status (VmHWM)
     * macOS: 使用 task_info (resident_size_max)
     */
    static u64 getProcessPeakMemoryMB();

    /**
     * @brief 获取当前进程已提交内存（commit charge）使用量 (MB)
     *
     * 与 getProcessMemoryMB()（工作集）互补：工作集是当前驻留物理 RAM 的页，
     * 提交量是进程向 OS 申请保留的总虚拟内存（含换出页、未驻留页）。释放堆内存后
     * 提交量通常比工作集更及时回落，故更适合用于评估结构优化是否真实降低占用。
     *
     * Windows: 使用 GetProcessMemoryInfo (PagefileUsage)
     * Linux: 读取 /proc/self/status (VmSize)
     * macOS: 使用 task_info (virtual_size)
     */
    static u64 getProcessCommitMB();

    /**
     * @brief 从Vulkan设备属性提取GPU信息
     *
     * @param properties Vulkan物理设备属性
     * @param memoryProperties Vulkan内存属性
     */
    static GpuInfo getGpuInfoFromVulkan(const struct VkPhysicalDeviceProperties_T* properties,
        const struct VkPhysicalDeviceMemoryProperties_T* memoryProperties);

    /**
     * @brief 获取平台名称
     *
     * 返回操作系统名称，如 "Windows 10", "Linux", "macOS"
     */
    static std::string getPlatformName();

    /**
     * @brief 检查是否为64位系统
     */
    static bool is64BitSystem();

private:
// 平台特定实现
#ifdef _WIN32
    static MemoryInfo getMemoryInfoWindows();
    static CpuInfo getCpuInfoWindows();
    static u64 getProcessMemoryMBWindows();
    static u64 getProcessPeakMemoryMBWindows();
    static u64 getProcessCommitMBWindows();
    static std::string getPlatformNameWindows();
#elif defined(__linux__)
    static MemoryInfo getMemoryInfoLinux();
    static CpuInfo getCpuInfoLinux();
    static u64 getProcessMemoryMBLinux();
    static u64 getProcessPeakMemoryMBLinux();
    static u64 getProcessCommitMBLinux();
    static std::string getPlatformNameLinux();
#elif defined(__APPLE__)
    static MemoryInfo getMemoryInfoMacOS();
    static CpuInfo getCpuInfoMacOS();
    static u64 getProcessMemoryMBMacOS();
    static u64 getProcessPeakMemoryMBMacOS();
    static u64 getProcessCommitMBMacOS();
    static std::string getPlatformNameMacOS();
#endif
};

} // namespace mc::util
