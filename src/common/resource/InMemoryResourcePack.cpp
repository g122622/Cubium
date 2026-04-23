#include "InMemoryResourcePack.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>

namespace mc {

InMemoryResourcePack::InMemoryResourcePack(String name)
    : m_name(std::move(name))
    , m_metadata(3, "Built-in resources")  // pack_format 3 for 1.16.x
{
}

void InMemoryResourcePack::addResource(String path, String content) {
    String normalized = normalizePath(path);
    std::vector<u8> data(content.begin(), content.end());
    m_resources[normalized] = std::move(data);

    // 自动添加目录条目
    size_t lastSlash = normalized.find_last_of('/');
    if (lastSlash != String::npos) {
        String dir = normalized.substr(0, lastSlash);
        while (!dir.empty()) {
            m_directories.insert(dir);
            size_t pos = dir.find_last_of('/');
            if (pos == String::npos) {
                break;
            }
            dir = dir.substr(0, pos);
        }
    }
}

void InMemoryResourcePack::addResource(String path, std::vector<u8> data) {
    String normalized = normalizePath(path);
    m_resources[normalized] = std::move(data);

    // 自动添加目录条目
    size_t lastSlash = normalized.find_last_of('/');
    if (lastSlash != String::npos) {
        String dir = normalized.substr(0, lastSlash);
        while (!dir.empty()) {
            m_directories.insert(dir);
            size_t pos = dir.find_last_of('/');
            if (pos == String::npos) {
                break;
            }
            dir = dir.substr(0, pos);
        }
    }
}

void InMemoryResourcePack::addDirectory(String directory) {
    String normalized = normalizePath(directory);
    m_directories.insert(normalized);
}

Result<void> InMemoryResourcePack::initialize() {
    spdlog::info("In-memory resource pack '{}' initialized: {} resources",
                 m_name, m_resources.size());
    return Result<void>::ok();
}

bool InMemoryResourcePack::hasResource(StringView resourcePath) const {
    String normalized = normalizePath(resourcePath);
    return m_resources.find(normalized) != m_resources.end();
}

Result<std::vector<u8>> InMemoryResourcePack::readResource(StringView resourcePath) const {
    String normalized = normalizePath(resourcePath);

    auto it = m_resources.find(normalized);
    if (it != m_resources.end()) {
        return it->second;
    }

    std::ostringstream oss;
    oss << "Resource not found in memory pack: " << m_name << ", name: " << normalized;
    return Error(ErrorCode::ResourceNotFound, oss.str());
}

Result<std::vector<String>> InMemoryResourcePack::listResources(
    StringView directory,
    StringView extension) const
{
    std::vector<String> resources;
    String normalizedDir = normalizePath(directory);

    // 确保目录以斜杠结尾
    if (!normalizedDir.empty() && normalizedDir.back() != '/') {
        normalizedDir += '/';
    }

    for (const auto& [path, data] : m_resources) {
        // 检查是否在指定目录下
        if (path.size() > normalizedDir.size() &&
            path.substr(0, normalizedDir.size()) == normalizedDir) {

            // 检查扩展名
            if (extension.empty()) {
                resources.push_back(path);
            } else {
                if (path.size() >= extension.size() &&
                    path.substr(path.size() - extension.size()) == extension) {
                    resources.push_back(path);
                }
            }
        }
    }

    // 排序
    std::sort(resources.begin(), resources.end());

    return resources;
}

String InMemoryResourcePack::normalizePath(StringView path) {
    String result(path);

    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');

    // 移除前导斜杠
    while (!result.empty() && result.front() == '/') {
        result.erase(0, 1);
    }

    return result;
}

} // namespace mc
