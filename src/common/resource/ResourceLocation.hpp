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

#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace mc::resource {

/**
 * @brief 资源位置标识符
 *
 * Minecraft标准资源格式: namespace:path
 * 例如: minecraft:textures/blocks/stone
 *       minecraft:models/block/cube_all
 */
class ResourceLocation {
public:
    ResourceLocation();
    explicit ResourceLocation(std::string_view fullPath);
    ResourceLocation(std::string namespace_, std::string path);

    // 解析资源路径
    [[nodiscard]] static ResourceLocation parse(std::string_view fullPath);

    // 获取命名空间
    [[nodiscard]] const std::string& namespace_() const noexcept { return m_namespace; }

    // 获取路径
    [[nodiscard]] const std::string& path() const noexcept { return m_path; }

    // 检查是否有效
    [[nodiscard]] bool isValid() const noexcept { return !m_namespace.empty() || !m_path.empty(); }

    // 转换为完整字符串 "namespace:path"
    [[nodiscard]] std::string toString() const;

    // 根据 PackType 转换为文件路径
    // ClientResources -> "assets/namespace/path"
    // ServerData -> "data/namespace/path"
    [[nodiscard]] std::string toFilePath(PackType type) const;

    // 根据 PackType 转换为文件路径（带扩展名）
    [[nodiscard]] std::string toFilePath(PackType type, std::string_view extension) const;

    // 比较
    [[nodiscard]] bool operator==(const ResourceLocation& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceLocation& other) const noexcept;
    [[nodiscard]] bool operator<(const ResourceLocation& other) const noexcept;

    // 哈希
    [[nodiscard]] size_t hash() const noexcept;

private:
    std::string m_namespace;
    std::string m_path;
};

} // namespace mc::resource

// std::hash特化
namespace std {
template <>
struct hash<mc::resource::ResourceLocation> {
    size_t operator()(const mc::resource::ResourceLocation& loc) const noexcept { return loc.hash(); }
};
} // namespace std

namespace mc {
using ResourceLocation = resource::ResourceLocation;
} // namespace mc
