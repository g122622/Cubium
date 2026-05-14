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

#include "InMemoryResourcePack.hpp"
#include <algorithm>
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc {

InMemoryResourcePack::InMemoryResourcePack(std::string name)
    : m_name(std::move(name))
    , m_metadata(3, "Built-in resources") // pack_format 3 for 1.16.x
{}

void InMemoryResourcePack::addResource(std::string path, std::string content)
{
    std::string normalized = normalizePath(path);
    std::vector<u8> data(content.begin(), content.end());
    m_resources[normalized] = std::move(data);

    // 自动添加目录条目
    size_t lastSlash = normalized.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = normalized.substr(0, lastSlash);
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

void InMemoryResourcePack::addResource(std::string path, std::vector<u8> data)
{
    std::string normalized = normalizePath(path);
    m_resources[normalized] = std::move(data);

    // 自动添加目录条目
    size_t lastSlash = normalized.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = normalized.substr(0, lastSlash);
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

void InMemoryResourcePack::addDirectory(std::string directory)
{
    std::string normalized = normalizePath(directory);
    m_directories.insert(normalized);
}

Result<void> InMemoryResourcePack::initialize()
{
    spdlog::info("In-memory resource pack '{}' initialized: {} resources", m_name, m_resources.size());
    return Result<void>::ok();
}

bool InMemoryResourcePack::hasResource(std::string_view resourcePath) const
{
    std::string normalized = normalizePath(resourcePath);
    return m_resources.find(normalized) != m_resources.end();
}

Result<std::vector<u8>> InMemoryResourcePack::readResource(std::string_view resourcePath) const
{
    std::string normalized = normalizePath(resourcePath);

    auto it = m_resources.find(normalized);
    if (it != m_resources.end()) {
        return it->second;
    }

    std::ostringstream oss;
    oss << "Resource not found in memory pack: " << m_name << ", name: " << normalized;
    return Error(ErrorCode::ResourceNotFound, oss.str());
}

Result<std::vector<std::string>> InMemoryResourcePack::listResources(
    std::string_view directory, std::string_view extension) const
{
    std::vector<std::string> resources;
    std::string normalizedDir = normalizePath(directory);

    // 确保目录以斜杠结尾
    if (!normalizedDir.empty() && normalizedDir.back() != '/') {
        normalizedDir += '/';
    }

    for (const auto& [path, data] : m_resources) {
        // 检查是否在指定目录下
        if (path.size() > normalizedDir.size() && path.substr(0, normalizedDir.size()) == normalizedDir) {

            // 检查扩展名
            if (extension.empty()) {
                resources.push_back(path);
            } else {
                if (path.size() >= extension.size() && path.substr(path.size() - extension.size()) == extension) {
                    resources.push_back(path);
                }
            }
        }
    }

    // 排序
    std::sort(resources.begin(), resources.end());

    return resources;
}

std::string InMemoryResourcePack::normalizePath(std::string_view path)
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

} // namespace mc
