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

#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::resource {

InMemoryResourcePack::InMemoryResourcePack(std::string name)
    : m_name(std::move(name))
    , m_metadata(3, "Built-in resources") // pack_format 3 for 1.16.x
{}

void InMemoryResourcePack::addResource(PackType type, std::string_view path, std::vector<u8> content)
{
    std::string normalized = _makeTypedPath(type, path);
    m_resources[normalized] = std::move(content);
    _addDirectoryEntries(normalized);
}

void InMemoryResourcePack::addClientResource(std::string path, std::string content)
{
    std::vector<u8> data(content.begin(), content.end());
    addResource(PackType::ClientResources, path, std::move(data));
}

void InMemoryResourcePack::addServerDataResource(std::string path, std::string content)
{
    std::vector<u8> data(content.begin(), content.end());
    addResource(PackType::ServerData, path, std::move(data));
}

void InMemoryResourcePack::addClientResource(std::string path, std::vector<u8> data)
{
    addResource(PackType::ClientResources, path, std::move(data));
}

void InMemoryResourcePack::addServerDataResource(std::string path, std::vector<u8> data)
{
    addResource(PackType::ServerData, path, std::move(data));
}

void InMemoryResourcePack::addDirectory(PackType type, std::string directory)
{
    std::string normalized = _makeTypedPath(type, directory);
    m_directories.insert(normalized);
}

Result<void> InMemoryResourcePack::initialize()
{
    spdlog::info("In-memory resource pack '{}' initialized: {} resources", m_name, m_resources.size());
    return Result<void>::ok();
}

bool InMemoryResourcePack::hasResource(PackType type, std::string_view resourcePath) const
{
    const std::string normalized = _makeTypedPath(type, resourcePath);
    return m_resources.find(normalized) != m_resources.end();
}

Result<std::vector<u8>> InMemoryResourcePack::readResource(PackType type, std::string_view resourcePath) const
{
    const std::string normalized = _makeTypedPath(type, resourcePath);

    auto it = m_resources.find(normalized);
    if (it != m_resources.end()) {
        return it->second;
    }

    std::ostringstream oss;
    oss << "Resource not found in memory pack: " << m_name << ", name: " << normalized;
    return Error(ErrorCode::ResourceNotFound, oss.str());
}

Result<std::vector<std::string>> InMemoryResourcePack::listResources(
    PackType type, std::string_view directory, std::string_view extension) const
{
    std::vector<std::string> resources;
    std::string normalizedDir = _makeTypedPath(type, directory);

    // 确保目录以斜杠结尾
    if (!normalizedDir.empty() && normalizedDir.back() != '/') {
        normalizedDir += '/';
    }

    // 类型目录前缀，如 "assets/" 或 "data/"
    // 返回的路径应相对于类型目录根，与 FolderResourcePack 保持一致
    std::string typePrefix = std::string(resource::packTypeDirectoryName(type)) + "/";

    for (const auto& [path, data] : m_resources) {
        // 检查是否在指定目录下
        if (path.size() > normalizedDir.size() && path.substr(0, normalizedDir.size()) == normalizedDir) {

            // 相对于类型目录根的路径（如 "minecraft/blockstates/stone.json"）
            std::string relativePath = path.substr(typePrefix.size());
            if (extension.empty()) {
                resources.push_back(relativePath);
            } else {
                if (relativePath.size() >= extension.size() &&
                    relativePath.substr(relativePath.size() - extension.size()) == extension) {
                    resources.push_back(relativePath);
                }
            }
        }
    }

    // 排序
    std::sort(resources.begin(), resources.end());

    return resources;
}

Result<std::vector<std::string>> InMemoryResourcePack::getResourceNamespaces(PackType type) const
{
    std::string typeDir(resource::packTypeDirectoryName(type));
    std::string prefix = typeDir + "/";

    std::unordered_set<std::string> namespaces;

    for (const auto& [path, data] : m_resources) {
        // 检查路径是否以类型目录前缀开头
        if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
            // 提取类型目录下的第一级子目录作为命名空间
            std::string rest = path.substr(prefix.size());
            size_t slashPos = rest.find('/');
            if (slashPos != std::string::npos) {
                namespaces.insert(rest.substr(0, slashPos));
            }
        }
    }

    std::vector<std::string> result(namespaces.begin(), namespaces.end());
    std::sort(result.begin(), result.end());
    return result;
}

std::string InMemoryResourcePack::_normalizePath(std::string_view path)
{
    std::string result(path);

    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');

    // 移除前导斜杠
    while (!result.empty() && result.front() == '/') {
        result.erase(0, 1);
    }

    return result;
}

std::string InMemoryResourcePack::_makeTypedPath(PackType type, std::string_view path)
{
    return _normalizePath(std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(path));
}

void InMemoryResourcePack::_addDirectoryEntries(const std::string& normalizedPath)
{
    // 自动添加目录条目
    size_t lastSlash = normalizedPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = normalizedPath.substr(0, lastSlash);
        while (!dir.empty()) {
            m_directories.insert(dir);
            size_t pos = dir.find_last_of('/');
            if (pos == std::string::npos) {
                break;
            }
            dir = dir.substr(0, pos);
        }
    }
}

} // namespace mc::resource
