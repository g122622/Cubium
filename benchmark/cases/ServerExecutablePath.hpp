#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace mc::benchmark::detail {

/**
 * @brief 以当前基准可执行文件所在目录定位同配置的服务端，允许整体移动 artifact。
 * @param fileName CMake 注入的服务端文件名。
 * @param error 查询可执行文件路径失败时的系统错误；成功时清空。
 * @return 同目录服务端路径；查询失败时返回空路径。
 */
inline std::filesystem::path resolveServerExecutable(std::string_view fileName, std::error_code& error)
{
    error.clear();
    std::filesystem::path executable;
#ifdef _WIN32
    // 动态扩大缓冲区，避免 MAX_PATH 截断重定位后的较长路径。
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
            return {};
        }
        if (length < buffer.size()) {
            executable = std::wstring(buffer.data(), length);
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        error = std::make_error_code(std::errc::filename_too_long);
        return {};
    }
    executable = std::filesystem::weakly_canonical(buffer.data(), error);
    if (error) {
        return {};
    }
#else
    executable = std::filesystem::read_symlink("/proc/self/exe", error);
    if (error) {
        return {};
    }
#endif
    return executable.parent_path() / std::filesystem::path(fileName);
}

} // namespace mc::benchmark::detail
