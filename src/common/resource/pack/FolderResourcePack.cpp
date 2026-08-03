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

#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/PackMetadata.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace mc::resource {

namespace {
std::string _getDirectoryName(const std::string& path)
{
    fs::path p(path);
    return p.filename().string();
}
} // namespace

FolderResourcePack::FolderResourcePack(std::string rootPath)
    : m_rootPath(std::move(rootPath))
    , m_name(_getDirectoryName(m_rootPath))
{}

Result<void> FolderResourcePack::initialize()
{
    // 检查根目录是否存在
    if (!fs::exists(m_rootPath)) {
        return Error(ErrorCode::ResourcePackNotFound, std::string("Resource pack not found: ") + m_rootPath);
    }

    if (!fs::is_directory(m_rootPath)) {
        return Error(
            ErrorCode::ResourcePackInvalid, std::string("Resource pack path is not a directory: ") + m_rootPath);
    }

    // 读取pack.mcmeta
    std::string mcmetaPath = m_rootPath + "/pack.mcmeta";

    if (fs::exists(mcmetaPath)) {
        auto result = PackMetadata::parseFile(mcmetaPath);
        if (result.success()) {
            m_metadata = result.value();
        } else {
            // pack.mcmeta存在但解析失败，继续但记录警告
            m_metadata = PackMetadata();
        }
    }

    return Result<void>::ok();
}

bool FolderResourcePack::hasResource(PackType type, std::string_view resourcePath) const
{
    const std::string fullPath =
        _normalize_path(std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(resourcePath));
    return fs::exists(fullPath) && fs::is_regular_file(fullPath);
}

Result<std::vector<u8>> FolderResourcePack::readResource(PackType type, std::string_view resourcePath) const
{
    const std::string fullPath =
        _normalize_path(std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(resourcePath));

    if (!fs::exists(fullPath)) {
        return Error(ErrorCode::ResourceNotFound,
            std::string("Resource not found in folder pack: ") + std::string(m_name) +
                ", name: " + std::string(resourcePath));
    }

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, std::string("Cannot open resource: ") + fullPath);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        return Error(ErrorCode::FileReadFailed, std::string("Failed to read resource: ") + fullPath);
    }

    return data;
}

Result<std::vector<std::string>> FolderResourcePack::listResources(
    PackType type, std::string_view directory, std::string_view extension) const
{
    std::string fullPath =
        m_rootPath + "/" + std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(directory);

    if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
        return Error(ErrorCode::NotFound,
            std::string("Directory not found in folder pack: ") + std::string(m_name) + ", path: " + fullPath);
    }

    std::vector<std::string> resources;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(fullPath)) {
            if (!entry.is_regular_file()) continue;

            std::string filename = entry.path().filename().string();

            if (!extension.empty()) {
                std::string ext = entry.path().extension().string();
                std::string checkExt(extension);
                if (!ext.empty() && ext[0] == '.') {
                    ext = ext.substr(1);
                }
                if (checkExt[0] == '.') {
                    checkExt = checkExt.substr(1);
                }

                if (ext != checkExt) continue;
            }

            const fs::path basePath = fs::path(m_rootPath) / resource::packTypeDirectoryName(type);
            std::string relativePath = fs::relative(entry.path(), basePath).string();
            // 将反斜杠转换为正斜杠
            for (char& c : relativePath) {
                if (c == '\\') c = '/';
            }
            resources.push_back(relativePath);
        }
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::OperationFailed, std::string("Failed to list directory: ") + e.what());
    }

    return resources;
}

Result<std::vector<std::string>> FolderResourcePack::getResourceNamespaces(PackType type) const
{
    std::string typeDir(resource::packTypeDirectoryName(type));
    fs::path typePath = fs::path(m_rootPath) / typeDir;

    std::vector<std::string> namespaces;

    if (!fs::exists(typePath) || !fs::is_directory(typePath)) {
        return namespaces;
    }

    try {
        for (const auto& entry : fs::directory_iterator(typePath)) {
            if (entry.is_directory()) {
                namespaces.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::OperationFailed, std::string("Failed to list namespaces: ") + e.what());
    }

    return namespaces;
}

std::string FolderResourcePack::_normalize_path(std::string_view resourcePath) const
{
    std::string path(resourcePath);

    // 确保使用正斜杠
    for (char& c : path) {
        if (c == '\\') c = '/';
    }

    // 移除前导斜杠
    while (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }

    return m_rootPath + "/" + path;
}

} // namespace mc::resource
