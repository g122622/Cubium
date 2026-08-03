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

#include "common/resource/pack/ZipResourcePack.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/PackMetadata.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::resource {

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

ZipResourcePack::ZipResourcePack(std::filesystem::path zipPath)
    : m_zipPath(std::move(zipPath))
    , m_name(m_zipPath.stem().string())
{}

ZipResourcePack::~ZipResourcePack() noexcept = default;

// ============================================================================
// 静态工厂方法
// ============================================================================

Result<std::unique_ptr<ZipResourcePack>> ZipResourcePack::create(const std::filesystem::path& zipPath)
{
    if (!std::filesystem::exists(zipPath)) {
        return Error(ErrorCode::FileNotFound, "ZIP file not found: " + zipPath.string());
    }

    if (!std::filesystem::is_regular_file(zipPath)) {
        return Error(ErrorCode::FileNotFound, "Path is not a regular file: " + zipPath.string());
    }

    auto pack = std::unique_ptr<ZipResourcePack>(new ZipResourcePack(zipPath));
    return pack;
}

// ============================================================================
// IResourcePack 接口实现
// ============================================================================

Result<void> ZipResourcePack::initialize()
{
    // 初始化时先清空缓存，避免重载后残留旧条目
    {
        std::unique_lock lock(m_cacheMutex);
        m_cache.clear();
    }

    // 使用 libarchive 打开 ZIP 文件
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

#ifdef _WIN32
    int r = archive_read_open_filename_w(a, m_zipPath.wstring().c_str(), 10240);
#else
    int r = archive_read_open_filename(a, m_zipPath.string().c_str(), 10240);
#endif

    if (r != ARCHIVE_OK) {
        spdlog::error("Failed to open ZIP file: {}", archive_error_string(a));
        archive_read_free(a);
        return Error(ErrorCode::FileOpenFailed,
            "Failed to open ZIP file: " + m_zipPath.string() + " - " + std::string(archive_error_string(a)));
    }

    // 读取所有条目，构建索引
    m_entries.clear();
    struct archive_entry* entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (pathname && *pathname) {
            std::string normalizedPath = _normalizePath(pathname);
            // 跳过目录条目
            if (!normalizedPath.empty() && normalizedPath.back() != '/') {
                m_entries.insert(std::move(normalizedPath));
            }
        }
        // 跳过文件数据
        archive_read_data_skip(a);
    }

    archive_read_free(a);

    // 读取 pack.mcmeta
    const std::string mcmetaPath = "pack.mcmeta";
    if (m_entries.find(mcmetaPath) != m_entries.end()) {
        auto dataResult = readResource(PackType::ClientResources, "../pack.mcmeta");
        if (dataResult.success()) {
            const auto& data = dataResult.value();
            std::string jsonStr(data.begin(), data.end());
            auto metadataResult = PackMetadata::parse(jsonStr);
            if (metadataResult.success()) {
                m_metadata = std::move(metadataResult.value());
            } else {
                spdlog::error("Failed to parse pack.mcmeta for {}: {}", m_name, metadataResult.error().toString());
                m_metadata = PackMetadata();
            }
        }
    } else {
        m_metadata = PackMetadata();
    }

    spdlog::info(
        "ZIP resource pack '{}' loaded: {} entries, format {}", m_name, m_entries.size(), m_metadata.packFormat());
    return Result<void>::ok();
}

bool ZipResourcePack::hasResource(PackType type, std::string_view resourcePath) const
{
    const std::string normalized = _makeTypedPath(type, resourcePath);
    return m_entries.find(normalized) != m_entries.end();
}

Result<std::vector<u8>> ZipResourcePack::readResource(PackType type, std::string_view resourcePath) const
{
    std::string normalized;
    if (resourcePath == "../pack.mcmeta") {
        normalized = "pack.mcmeta";
    } else {
        normalized = _makeTypedPath(type, resourcePath);
    }

    // 先查缓存，避免并发读取时重复解压同一个条目
    {
        std::shared_lock lock(m_cacheMutex);
        auto cacheIt = m_cache.find(normalized);
        if (cacheIt != m_cache.end()) {
            return cacheIt->second;
        }
    }

    // 检查条目是否存在
    if (m_entries.find(normalized) == m_entries.end()) {
        return Error(ErrorCode::ResourceNotFound, "Resource not found in ZIP: " + normalized);
    }

    // 使用 libarchive 读取文件
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

#ifdef _WIN32
    int r = archive_read_open_filename_w(a, m_zipPath.wstring().c_str(), 10240);
#else
    int r = archive_read_open_filename(a, m_zipPath.string().c_str(), 10240);
#endif

    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return Error(ErrorCode::FileOpenFailed, "Failed to open ZIP file: " + m_zipPath.string());
    }

    // 查找目标条目
    struct archive_entry* entry;
    std::vector<u8> data;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (pathname && _normalizePath(pathname) == normalized) {
            // 找到目标条目，读取数据
            la_int64_t size = archive_entry_size(entry);
            if (size > 0) {
                data.resize(static_cast<size_t>(size));
                la_ssize_t read = archive_read_data(a, data.data(), data.size());
                if (read < 0) {
                    spdlog::error("Failed to read ZIP entry {}: {}", normalized, archive_error_string(a));
                    archive_read_free(a);
                    return Error(ErrorCode::FileReadFailed, "Failed to read ZIP entry: " + normalized);
                }
                data.resize(static_cast<size_t>(read));
            }
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_free(a);

    // 缓存结果（写锁），减少后续重复解压的开销
    {
        std::unique_lock lock(m_cacheMutex);
        m_cache[normalized] = data;
    }

    return data;
}

Result<std::vector<std::string>> ZipResourcePack::listResources(
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

    for (const auto& path : m_entries) {
        // 检查是否在指定目录下
        if (path.size() <= normalizedDir.size() || path.substr(0, normalizedDir.size()) != normalizedDir) {
            continue;
        }

        // 相对于类型目录根的路径（如 "minecraft/blockstates/stone.json"）
        std::string relativePath = path.substr(typePrefix.size());

        if (extension.empty()) {
            resources.push_back(relativePath);
            continue;
        }

        if (relativePath.size() >= extension.size() &&
            relativePath.substr(relativePath.size() - extension.size()) == extension) {
            resources.push_back(relativePath);
        }
    }

    std::sort(resources.begin(), resources.end());
    return resources;
}

Result<std::vector<std::string>> ZipResourcePack::getResourceNamespaces(PackType type) const
{
    std::string typeDir(resource::packTypeDirectoryName(type));
    std::string prefix = typeDir + "/";

    std::unordered_set<std::string> namespaces;

    for (const auto& path : m_entries) {
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

// ============================================================================
// 额外方法
// ============================================================================

void ZipResourcePack::clearCache()
{
    std::unique_lock lock(m_cacheMutex);
    m_cache.clear();
}

// ============================================================================
// 私有方法
// ============================================================================

std::string ZipResourcePack::_normalizePath(std::string_view path)
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

std::string ZipResourcePack::_makeTypedPath(PackType type, std::string_view path)
{
    return _normalizePath(std::string(resource::packTypeDirectoryName(type)) + "/" + std::string(path));
}

} // namespace mc::resource
