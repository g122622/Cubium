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

#include "ResourceLocation.hpp"
#include "common/resource/PackType.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace mc::resource {

ResourceLocation::ResourceLocation()
    : m_namespace("minecraft")
    , m_path("")
{}

ResourceLocation::ResourceLocation(std::string_view fullPath)
    : ResourceLocation(parse(fullPath))
{}

ResourceLocation::ResourceLocation(std::string namespace_, std::string path)
    : m_namespace(std::move(namespace_))
    , m_path(std::move(path))
{
    if (m_namespace.empty()) {
        m_namespace = "minecraft";
    }
}

ResourceLocation ResourceLocation::parse(std::string_view fullPath)
{
    auto colonPos = fullPath.find(':');

    if (colonPos == std::string_view::npos) {
        // 没有命名空间，使用默认值
        return ResourceLocation("minecraft", std::string(fullPath));
    }

    std::string namespace_(fullPath.substr(0, colonPos));
    std::string path(fullPath.substr(colonPos + 1));

    if (namespace_.empty()) {
        namespace_ = "minecraft";
    }

    return ResourceLocation(std::move(namespace_), std::move(path));
}

std::string ResourceLocation::toString() const
{
    return m_namespace + ":" + m_path;
}

std::string ResourceLocation::toFilePath(PackType type) const
{
    // ClientResources -> "assets/namespace/path"
    // ServerData -> "data/namespace/path"
    return std::string(resource::packTypeDirectoryName(type)) + "/" + m_namespace + "/" + m_path;
}

std::string ResourceLocation::toFilePath(PackType type, std::string_view extension) const
{
    std::string result = toFilePath(type);
    if (!extension.empty()) {
        if (extension[0] != '.') {
            result += '.';
        }
        result += extension;
    }
    return result;
}

bool ResourceLocation::operator==(const ResourceLocation& other) const noexcept
{
    return m_namespace == other.m_namespace && m_path == other.m_path;
}

bool ResourceLocation::operator!=(const ResourceLocation& other) const noexcept
{
    return !(*this == other);
}

bool ResourceLocation::operator<(const ResourceLocation& other) const noexcept
{
    if (m_namespace != other.m_namespace) {
        return m_namespace < other.m_namespace;
    }
    return m_path < other.m_path;
}

size_t ResourceLocation::hash() const noexcept
{
    size_t h1 = std::hash<std::string>{}(m_namespace);
    size_t h2 = std::hash<std::string>{}(m_path);
    return h1 ^ (h2 << 1);
}

} // namespace mc::resource
