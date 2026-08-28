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
 * The above copyright notice shall be included in all
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

#include "common/core/Types.hpp"
#include <string>
#include <string_view>

namespace mc::resource {

/**
 * @brief 资源包类型枚举
 *
 * 区分客户端资源包和服务端数据包：
 * - ClientResources 映射到 assets/ 目录（纹理、模型、音效、语言等）
 * - ServerData 映射到 data/ 目录（战利品表、配方、标签、函数等）
 *
 * 同一个物理资源包可以同时包含 assets/ 和 data/ 目录，
 * 通过 PackType 决定读取哪个子目录。
 */
enum class PackType : u8 {
    ClientResources, ///< 客户端资源包，映射到 assets/ 目录
    ServerData       ///< 服务端数据包，映射到 data/ 目录
};

/**
 * @brief 获取 PackType 对应的目录名
 * @param type 资源包类型
 * @return "assets" 或 "data"
 */
[[nodiscard]] constexpr std::string_view packTypeDirectoryName(PackType type) noexcept
{
    switch (type) {
        case PackType::ClientResources:
            return "assets";
        case PackType::ServerData:
            return "data";
    }
    return "assets"; // 不可达，但编译器可能需要
}

/**
 * @brief 词法归一化资源路径（折叠 . 与 .. 段）
 *
 * 输入是已拼好类型目录前缀的相对路径，如 "assets/../pack.mcmeta"，
 * 输出归一化后的路径 "pack.mcmeta"。处理规则：
 * - 反斜杠统一为正斜杠
 * - 移除前导斜杠
 * - "." 段跳过，".." 段弹出上一段（若已在根则跳过，不允许逃逸出包根）
 * - 空段（连续斜杠）跳过
 *
 * 必要性：资源路径可用 ".." 引用类型目录之外的包根文件（如 pack.mcmeta、
 * pack.png）。hasResource 基于 m_entries 内存索引查询，索引存储的是归一化
 * 路径（不含 .. 段），故查询前必须先词法归一化，否则 "assets/../pack.mcmeta"
 * 字面上与索引中的 "pack.mcmeta" 不匹配。readResource 交给文件系统解析 ..
 * 碰巧能工作，但为与 hasResource 行为一致、消除特判，同样先归一化。
 *
 * 纯词法、无系统调用，与 std::filesystem::path::lexically_normal 语义对齐
 * 但不构造 fs::path 对象（避免其内部小对象开销）。
 */
[[nodiscard]] inline std::string normalizeResourcePath(std::string_view path)
{
    std::string result(path);
    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }

    // 按 '/' 分段，用栈式折叠消除 . 与 .. 段
    std::string normalized;
    normalized.reserve(result.size());
    std::string_view remaining(result);

    while (!remaining.empty()) {
        size_t slash = remaining.find('/');
        std::string_view segment = (slash == std::string_view::npos) ? remaining : remaining.substr(0, slash);

        if (!segment.empty() && segment != "." && segment != "..") {
            // 普通段：追加（必要时补斜杠分隔）
            if (!normalized.empty() && normalized.back() != '/') {
                normalized += '/';
            }
            normalized.append(segment.data(), segment.size());
        } else if (segment == "..") {
            // 回退到上一段：找最后一个 '/' 并截断
            size_t lastSlash = normalized.rfind('/');
            if (lastSlash == std::string::npos) {
                normalized.clear(); // 已在根，丢弃 ..（不允许逃逸出包根）
            } else {
                normalized.resize(lastSlash);
            }
        }
        // "." 段与空段：跳过

        if (slash == std::string_view::npos) {
            break;
        }
        remaining.remove_prefix(slash + 1);
    }

    return normalized;
}

} // namespace mc::resource

namespace mc {
using resource::PackType;
} // namespace mc
