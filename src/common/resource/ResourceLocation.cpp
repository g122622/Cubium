#include "ResourceLocation.hpp"
#include <algorithm>

namespace mc {

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

std::string ResourceLocation::toFilePath() const
{
    return "assets/" + m_namespace + "/" + m_path;
}

std::string ResourceLocation::toFilePath(std::string_view extension) const
{
    std::string result = "assets/" + m_namespace + "/" + m_path;
    if (!extension.empty()) {
        if (extension[0] != '.') {
            result += '.';
        }
        result += extension;
    }
    return result;
}

bool ResourceLocation::operator==(const ResourceLocation& other) const
{
    return m_namespace == other.m_namespace && m_path == other.m_path;
}

bool ResourceLocation::operator!=(const ResourceLocation& other) const
{
    return !(*this == other);
}

bool ResourceLocation::operator<(const ResourceLocation& other) const
{
    if (m_namespace != other.m_namespace) {
        return m_namespace < other.m_namespace;
    }
    return m_path < other.m_path;
}

size_t ResourceLocation::hash() const
{
    size_t h1 = std::hash<std::string>{}(m_namespace);
    size_t h2 = std::hash<std::string>{}(m_path);
    return h1 ^ (h2 << 1);
}

} // namespace mc
