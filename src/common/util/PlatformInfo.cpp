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

#include "PlatformInfo.hpp"
#include "common/core/Types.hpp"
#include <spdlog/spdlog.h>

// 平台特定头文件
#ifdef _WIN32
// clang-format off
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
// clang-format on
#include <intrin.h>
#elif defined(__linux__)
#include <fstream>
#include <sstream>
#include <sys/sysinfo.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task_info.h>
#include <sys/sysctl.h>
#endif

// Vulkan 头文件
#include <vulkan/vulkan.h>

namespace mc::util {

// ============================================================================
// Windows 实现
// ============================================================================
#ifdef _WIN32

MemoryInfo PlatformInfo::getMemoryInfo()
{
    return getMemoryInfoWindows();
}

CpuInfo PlatformInfo::getCpuInfo()
{
    return getCpuInfoWindows();
}

u64 PlatformInfo::getProcessMemoryMB()
{
    return getProcessMemoryMBWindows();
}

u64 PlatformInfo::getProcessPeakMemoryMB()
{
    return getProcessPeakMemoryMBWindows();
}

u64 PlatformInfo::getProcessCommitMB()
{
    return getProcessCommitMBWindows();
}

std::string PlatformInfo::getPlatformName()
{
    return getPlatformNameWindows();
}

MemoryInfo PlatformInfo::getMemoryInfoWindows()
{
    MemoryInfo info = {};

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);

    if (GlobalMemoryStatusEx(&status)) {
        info.totalPhysicalMB = status.ullTotalPhys / (1024 * 1024);
        info.availablePhysicalMB = status.ullAvailPhys / (1024 * 1024);
        info.usedPhysicalMB = info.totalPhysicalMB - info.availablePhysicalMB;
        info.usagePercent = status.dwMemoryLoad;
        info.processUsedMB = getProcessMemoryMBWindows();
        info.processPeakMB = getProcessPeakMemoryMBWindows();
    } else {
        spdlog::warn("Failed to get memory info: {}", GetLastError());
    }

    return info;
}

CpuInfo PlatformInfo::getCpuInfoWindows()
{
    CpuInfo info = {};
    info.is64Bit = true; // 假设64位

    // 获取核心/线程数
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    info.coreCount = sysInfo.dwNumberOfProcessors;
    info.threadCount = sysInfo.dwNumberOfProcessors;

    // 使用 __cpuid 获取CPU信息
    int cpuinfo[4] = {};

    // 获取厂商 (EAX=0)
    __cpuid(cpuinfo, 0);
    char vendor[13] = {};
    *reinterpret_cast<int*>(vendor) = cpuinfo[1];
    *reinterpret_cast<int*>(vendor + 4) = cpuinfo[3];
    *reinterpret_cast<int*>(vendor + 8) = cpuinfo[2];
    vendor[12] = '\0';
    info.vendor = vendor;

    // 获取品牌字符串 (EAX=0x80000000 检查是否支持, 然后 EAX=0x80000002-0x80000004)
    __cpuid(cpuinfo, 0x80000000);
    if (static_cast<u32>(cpuinfo[0]) >= 0x80000004U) {
        char brand[49] = {};
        __cpuid(reinterpret_cast<int*>(brand), 0x80000002);
        __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
        __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
        brand[48] = '\0';

        // 去除前后空格
        info.brand = brand;
        size_t start = info.brand.find_first_not_of(" \t");
        size_t end = info.brand.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            info.brand = info.brand.substr(start, end - start + 1);
        }
    }

    // 获取时钟速度 (从注册表读取更准确, 这里简化处理)
    DWORD freq = 0;
    DWORD size = sizeof(freq);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) ==
        ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "~MHz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&freq), &size) == ERROR_SUCCESS) {
            info.clockSpeedMHz = freq;
        }
        RegCloseKey(hKey);
    }

    return info;
}

u64 PlatformInfo::getProcessMemoryMBWindows()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024 * 1024);
    }
    return 0;
}

u64 PlatformInfo::getProcessPeakMemoryMBWindows()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return pmc.PeakWorkingSetSize / (1024 * 1024);
    }
    return 0;
}

u64 PlatformInfo::getProcessCommitMBWindows()
{
    // PagefileUsage 是进程的提交量（commit charge），单位字节。
    // 与 WorkingSetSize（驻留物理 RAM）互补：释放堆后提交量更及时回落，
    // 用于评估结构优化是否真实降低占用（工作集受页面复用影响常纹丝不动）。
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return pmc.PagefileUsage / (1024 * 1024);
    }
    return 0;
}

std::string PlatformInfo::getPlatformNameWindows()
{
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    // 使用 RtlGetVersion 获取真实版本 (绕过兼容性垫片)
    typedef LONG(WINAPI * RtlGetVersionFunc)(PRTL_OSVERSIONINFOW);
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        auto pRtlGetVersion =
            reinterpret_cast<RtlGetVersionFunc>(reinterpret_cast<void*>(GetProcAddress(hNtDll, "RtlGetVersion")));
        if (pRtlGetVersion) {
            pRtlGetVersion(reinterpret_cast<PRTL_OSVERSIONINFOW>(&osvi));
        }
    }

    // 简化版本名称
    if (osvi.dwMajorVersion == 10) {
        if (osvi.dwBuildNumber >= 22000) {
            return "Windows 11";
        }
        return "Windows 10";
    } else if (osvi.dwMajorVersion == 6) {
        if (osvi.dwMinorVersion == 3) return "Windows 8.1";
        if (osvi.dwMinorVersion == 2) return "Windows 8";
        if (osvi.dwMinorVersion == 1) return "Windows 7";
        if (osvi.dwMinorVersion == 0) return "Windows Vista";
    }

    return "Windows";
}

// ============================================================================
// Linux 实现
// ============================================================================
#elif defined(__linux__)

MemoryInfo PlatformInfo::getMemoryInfo()
{
    return getMemoryInfoLinux();
}

CpuInfo PlatformInfo::getCpuInfo()
{
    return getCpuInfoLinux();
}

u64 PlatformInfo::getProcessMemoryMB()
{
    return getProcessMemoryMBLinux();
}

u64 PlatformInfo::getProcessPeakMemoryMB()
{
    return getProcessPeakMemoryMBLinux();
}

u64 PlatformInfo::getProcessCommitMB()
{
    return getProcessCommitMBLinux();
}

std::string PlatformInfo::getPlatformName()
{
    return getPlatformNameLinux();
}

MemoryInfo PlatformInfo::getMemoryInfoLinux()
{
    MemoryInfo info = {};

    struct sysinfo sysInfo;
    if (sysinfo(&sysInfo) == 0) {
        info.totalPhysicalMB = sysInfo.totalram * sysInfo.mem_unit / (1024 * 1024);
        info.availablePhysicalMB = sysInfo.freeram * sysInfo.mem_unit / (1024 * 1024);
        info.usedPhysicalMB = info.totalPhysicalMB - info.availablePhysicalMB;
        info.usagePercent = static_cast<u32>((info.usedPhysicalMB * 100) / info.totalPhysicalMB);
        info.processUsedMB = getProcessMemoryMBLinux();
        info.processPeakMB = getProcessPeakMemoryMBLinux();
    }

    return info;
}

CpuInfo PlatformInfo::getCpuInfoLinux()
{
    CpuInfo info = {};
    info.is64Bit = sizeof(void*) == 8;

    // 读取 /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        u32 processorCount = 0;
        bool foundModel = false;

        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") == 0) {
                if (!foundModel) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        info.brand = line.substr(pos + 2);
                        foundModel = true;
                    }
                }
            } else if (line.find("vendor_id") == 0) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    info.vendor = line.substr(pos + 2);
                }
            } else if (line.find("cpu MHz") == 0) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    info.clockSpeedMHz = static_cast<u32>(std::stoul(line.substr(pos + 2)));
                }
            } else if (line.find("processor") == 0) {
                processorCount++;
            }
        }

        info.threadCount = processorCount;
        info.coreCount = processorCount; // 简化处理，实际可能需要从其他地方读取
    }

    // 如果没有获取到型号，使用 sysconf
    if (info.brand.empty()) {
        info.brand = "Unknown CPU";
    }
    if (info.threadCount == 0) {
        info.threadCount = sysconf(_SC_NPROCESSORS_ONLN);
        info.coreCount = info.threadCount;
    }

    return info;
}

u64 PlatformInfo::getProcessMemoryMBLinux()
{
    std::ifstream status("/proc/self/status");
    if (status.is_open()) {
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("VmRSS:") == 0) {
                // VmRSS: 12345 kB
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    // 去除空格和单位
                    size_t numStart = value.find_first_not_of(" \t");
                    size_t numEnd = value.find_first_of(" \tk", numStart);
                    if (numStart != std::string::npos) {
                        u64 kb = std::stoull(value.substr(numStart, numEnd - numStart));
                        return kb / 1024;
                    }
                }
            }
        }
    }
    return 0;
}

u64 PlatformInfo::getProcessPeakMemoryMBLinux()
{
    std::ifstream status("/proc/self/status");
    if (status.is_open()) {
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("VmHWM:") == 0) {
                // VmHWM: 12345 kB (peak resident set size)
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    size_t numStart = value.find_first_not_of(" \t");
                    size_t numEnd = value.find_first_of(" \tk", numStart);
                    if (numStart != std::string::npos) {
                        u64 kb = std::stoull(value.substr(numStart, numEnd - numStart));
                        return kb / 1024;
                    }
                }
            }
        }
    }
    return 0;
}

u64 PlatformInfo::getProcessCommitMBLinux()
{
    // VmSize 是进程总虚拟内存（含换出、未驻留页），语义对应 Windows 的提交量。
    // 与 VmRSS（驻留物理 RAM）互补：释放堆后 VmSize 更及时回落。
    std::ifstream status("/proc/self/status");
    if (status.is_open()) {
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("VmSize:") == 0) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    size_t numStart = value.find_first_not_of(" \t");
                    size_t numEnd = value.find_first_of(" \tk", numStart);
                    if (numStart != std::string::npos) {
                        u64 kb = std::stoull(value.substr(numStart, numEnd - numStart));
                        return kb / 1024;
                    }
                }
            }
        }
    }
    return 0;
}

std::string PlatformInfo::getPlatformNameLinux()
{
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            if (line.find("PRETTY_NAME=") == 0) {
                std::string name = line.substr(12);
                // 去除引号
                if (!name.empty() && name.front() == '"') {
                    name = name.substr(1);
                }
                if (!name.empty() && name.back() == '"') {
                    name = name.substr(0, name.length() - 1);
                }
                return name;
            }
        }
    }
    return "Linux";
}

// ============================================================================
// macOS 实现
// ============================================================================
#elif defined(__APPLE__)

MemoryInfo PlatformInfo::getMemoryInfo()
{
    return getMemoryInfoMacOS();
}

CpuInfo PlatformInfo::getCpuInfo()
{
    return getCpuInfoMacOS();
}

u64 PlatformInfo::getProcessMemoryMB()
{
    return getProcessMemoryMBMacOS();
}

u64 PlatformInfo::getProcessPeakMemoryMB()
{
    return getProcessPeakMemoryMBMacOS();
}

u64 PlatformInfo::getProcessCommitMB()
{
    return getProcessCommitMBMacOS();
}

std::string PlatformInfo::getPlatformName()
{
    return getPlatformNameMacOS();
}

MemoryInfo PlatformInfo::getMemoryInfoMacOS()
{
    MemoryInfo info = {};

    // 获取物理内存
    u64 memSize = 0;
    size_t size = sizeof(memSize);
    if (sysctlbyname("hw.memsize", &memSize, &size, nullptr, 0) == 0) {
        info.totalPhysicalMB = memSize / (1024 * 1024);
    }

    // 获取可用内存 (使用 vm_statistics)
    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmStats), &count) ==
        KERN_SUCCESS) {
        info.availablePhysicalMB = (vmStats.free_count + vmStats.inactive_count) * vm_page_size / (1024 * 1024);
        info.usedPhysicalMB = info.totalPhysicalMB - info.availablePhysicalMB;
        if (info.totalPhysicalMB > 0) {
            info.usagePercent = static_cast<u32>((info.usedPhysicalMB * 100) / info.totalPhysicalMB);
        }
    }

    info.processUsedMB = getProcessMemoryMBMacOS();
    info.processPeakMB = getProcessPeakMemoryMBMacOS();

    return info;
}

CpuInfo PlatformInfo::getCpuInfoMacOS()
{
    CpuInfo info = {};
    info.is64Bit = true;

    // 获取CPU品牌
    char brand[256] = {};
    size_t brandSize = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", brand, &brandSize, nullptr, 0) == 0) {
        info.brand = brand;
    }

    // 获取厂商
    char vendor[64] = {};
    size_t vendorSize = sizeof(vendor);
    if (sysctlbyname("machdep.cpu.vendor", vendor, &vendorSize, nullptr, 0) == 0) {
        info.vendor = vendor;
    }

    // 获取核心数
    int coreCount = 0;
    size_t coreSize = sizeof(coreCount);
    if (sysctlbyname("hw.physicalcpu", &coreCount, &coreSize, nullptr, 0) == 0) {
        info.coreCount = static_cast<u32>(coreCount);
    }

    // 获取线程数
    int threadCount = 0;
    size_t threadSize = sizeof(threadCount);
    if (sysctlbyname("hw.logicalcpu", &threadCount, &threadSize, nullptr, 0) == 0) {
        info.threadCount = static_cast<u32>(threadCount);
    }

    // 获取时钟速度
    u64 freq = 0;
    size_t freqSize = sizeof(freq);
    if (sysctlbyname("hw.cpufrequency", &freq, &freqSize, nullptr, 0) == 0) {
        info.clockSpeedMHz = static_cast<u32>(freq / 1000000);
    }

    // Apple Silicon 可能没有 brand_string 或频率信息
    if (info.brand.empty()) {
        // 尝试检测 Apple Silicon
        char cpuType[64] = {};
        size_t cpuTypeSize = sizeof(cpuType);
        if (sysctlbyname("hw.cputype", cpuType, &cpuTypeSize, nullptr, 0) == 0) {
            int type = *reinterpret_cast<int*>(cpuType);
            if (type == 0x0100000C) { // CPU_TYPE_ARM64
                info.brand = "Apple Silicon";
                info.vendor = "Apple";
            }
        }
    }

    return info;
}

u64 PlatformInfo::getProcessMemoryMBMacOS()
{
    task_basic_info_64 info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_64_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO_64, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return info.resident_size / (1024 * 1024);
    }
    return 0;
}

u64 PlatformInfo::getProcessCommitMBMacOS()
{
    // virtual_size 是进程总虚拟内存，语义对应 Windows 的提交量。
    task_basic_info_64 info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_64_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO_64, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return info.virtual_size / (1024 * 1024);
    }
    return 0;
}

u64 PlatformInfo::getProcessPeakMemoryMBMacOS()
{
    // macOS 使用 TASK_VM_INFO 获取峰值内存
    task_vm_info_data_t vmInfo;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&vmInfo), &count) == KERN_SUCCESS) {
        return vmInfo.resident_size_peak / (1024 * 1024);
    }
    return 0;
}

std::string PlatformInfo::getPlatformNameMacOS()
{
    char version[256] = {};
    size_t size = sizeof(version);

    // 获取 macOS 版本
    if (sysctlbyname("kern.osproductversion", version, &size, nullptr, 0) == 0) {
        return std::string("macOS ") + version;
    }

    return "macOS";
}

#endif // 平台特定实现

// ============================================================================
// 跨平台实现
// ============================================================================

GpuInfo PlatformInfo::getGpuInfoFromVulkan(
    const VkPhysicalDeviceProperties_T* properties, const VkPhysicalDeviceMemoryProperties_T* memoryProperties)
{
    GpuInfo info = {};

    if (properties == nullptr) {
        return info;
    }

    const VkPhysicalDeviceProperties* props = reinterpret_cast<const VkPhysicalDeviceProperties*>(properties);
    const VkPhysicalDeviceMemoryProperties* memProps =
        reinterpret_cast<const VkPhysicalDeviceMemoryProperties*>(memoryProperties);

    // 设备名称
    info.name = props->deviceName;

    // API版本
    info.apiMajorVersion = VK_API_VERSION_MAJOR(props->apiVersion);
    info.apiMinorVersion = VK_API_VERSION_MINOR(props->apiVersion);

    // 厂商
    switch (props->vendorID) {
        case 0x10DE:
            info.vendor = "NVIDIA";
            break;
        case 0x1002:
        case 0x1022: // AMD
            info.vendor = "AMD";
            break;
        case 0x8086:
        case 0x8087: // Intel
            info.vendor = "Intel";
            break;
        case 0x13B5: // ARM
            info.vendor = "ARM";
            break;
        case 0x1010: // Apple
            info.vendor = "Apple";
            break;
        case 0x5143: // Qualcomm
            info.vendor = "Qualcomm";
            break;
        default:
            info.vendor = "Unknown";
            break;
    }

    // 驱动版本
    info.driverVersion = std::to_string(VK_API_VERSION_MAJOR(props->driverVersion)) + "." +
        std::to_string(VK_API_VERSION_MINOR(props->driverVersion)) + "." +
        std::to_string(VK_API_VERSION_PATCH(props->driverVersion));

    // 计算显存
    if (memProps) {
        u64 dedicatedVideoMemory = 0;
        u64 sharedSystemMemory = 0;

        for (u32 i = 0; i < memProps->memoryHeapCount; ++i) {
            const VkMemoryHeap& heap = memProps->memoryHeaps[i];
            if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                dedicatedVideoMemory += heap.size;
            } else {
                sharedSystemMemory += heap.size;
            }
        }

        info.dedicatedVideoMB = dedicatedVideoMemory / (1024 * 1024);
        info.sharedSystemMB = sharedSystemMemory / (1024 * 1024);
    }

    return info;
}

bool PlatformInfo::is64BitSystem()
{
    return sizeof(void*) == 8;
}

} // namespace mc::util
