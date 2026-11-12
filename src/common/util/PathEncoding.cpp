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

#include "common/util/PathEncoding.hpp"
#include "common/util/assert/AssertAll.hpp"

#ifdef _WIN32
// clang-format off
// windows.h 必须在 shellapi.h 之前：后者依赖前者引入的 EXTERN_C / DECLARE_HANDLE。
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
// clang-format on
#endif

namespace mc::util {

#ifdef _WIN32

std::wstring utf8ToWide(std::string_view text)
{
    if (text.empty()) {
        return {};
    }

    const i32 wcharCount =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<i32>(text.size()), nullptr, 0);
    if (wcharCount <= 0) {
        return {};
    }

    std::wstring wide(static_cast<usize>(wcharCount), L'\0');
    MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<i32>(text.size()), wide.data(), wcharCount);
    return wide;
}

std::string wideToUtf8(std::wstring_view text)
{
    if (text.empty()) {
        return {};
    }

    const i32 byteCount = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<i32>(text.size()), nullptr, 0, nullptr, nullptr);
    if (byteCount <= 0) {
        return {};
    }

    std::string utf8(static_cast<usize>(byteCount), '\0');
    WideCharToMultiByte(CP_UTF8,
        WC_ERR_INVALID_CHARS,
        text.data(),
        static_cast<i32>(text.size()),
        utf8.data(),
        byteCount,
        nullptr,
        nullptr);
    return utf8;
}

std::filesystem::path pathFromUtf8(std::string_view text)
{
    if (text.empty()) {
        return {};
    }
    // std::filesystem::path 的宽字符构造在 Windows 上直接以 UTF-16 保存，
    // 是唯一不失真的入口。
    return std::filesystem::path(utf8ToWide(text));
}

std::string pathToUtf8(const std::filesystem::path& path)
{
    return wideToUtf8(path.wstring());
}

std::vector<std::string> commandLineArgumentsUtf8()
{
    LPWSTR rawCommandLine = GetCommandLineW();
    if (rawCommandLine == nullptr) {
        return {};
    }

    // CommandLineToArgvW 采用与 CRT 相同的分词规则（引号、反斜杠转义），
    // 因此拆出的参数与 main 的 argv 一一对应，只是保住了宽字符原始值。
    int argumentCount = 0;
    LPWSTR* wideArguments = CommandLineToArgvW(rawCommandLine, &argumentCount);
    if (wideArguments == nullptr || argumentCount <= 0) {
        return {};
    }

    std::vector<std::string> arguments;
    arguments.reserve(static_cast<usize>(argumentCount));
    for (int index = 0; index < argumentCount; ++index) {
        arguments.push_back(wideToUtf8(wideArguments[index]));
    }

    LocalFree(wideArguments);
    return arguments;
}

#else

std::wstring utf8ToWide(std::string_view text)
{
    // 非 Windows 平台不该调用（main 的 argv 已是本地编码），返回空串让调用方回退。
    MC_UNUSED(text);
    return {};
}

std::string wideToUtf8(std::wstring_view text)
{
    MC_UNUSED(text);
    return {};
}

std::filesystem::path pathFromUtf8(std::string_view text)
{
    // POSIX 下窄字符串即路径字节序列，直接构造即可。
    return std::filesystem::path(std::string(text));
}

std::string pathToUtf8(const std::filesystem::path& path)
{
    return path.string();
}

std::vector<std::string> commandLineArgumentsUtf8()
{
    return {};
}

#endif

} // namespace mc::util
