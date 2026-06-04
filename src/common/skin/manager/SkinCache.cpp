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

#include "SkinCache.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::skin {

SkinCache::SkinCache(const std::string& cacheDir)
    : m_cacheDirStr(cacheDir)
    , m_cacheDir(cacheDir)
    , m_skinsDir(cacheDir + "/skins")
    , m_capesDir(cacheDir + "/capes")
    , m_metadataPath(cacheDir + "/metadata.json")
{}

SkinCache::~SkinCache()
{
    if (m_initialized) {
        _saveMetadata();
    }
}

Result<void> SkinCache::initialize()
{
    if (m_initialized) {
        return {};
    }

    // 创建目录
    auto result = _ensureDirectoriesExist();
    if (!result.success()) {
        return result;
    }

    // 加载元数据
    _loadMetadata();

    // 扫描现有文件
    _scanExistingFiles();

    m_initialized = true;
    spdlog::info("SkinCache initialized: {} skins, {} capes, {} bytes",
        m_skinEntries.size(),
        m_capeEntries.size(),
        m_totalCacheSize);
    return {};
}

void SkinCache::shutdown()
{
    if (!m_initialized) {
        return;
    }

    _saveMetadata();
    m_initialized = false;
    spdlog::info("SkinCache shutdown");
}

Result<void> SkinCache::_ensureDirectoriesExist()
{
    std::error_code ec;

    if (!std::filesystem::create_directories(m_cacheDir, ec) && ec) {
        return Error(ErrorCode::FileOpenFailed, "Failed to create cache directory: " + m_cacheDirStr);
    }

    if (!std::filesystem::create_directories(m_skinsDir, ec) && ec) {
        return Error(ErrorCode::FileOpenFailed, "Failed to create skins directory");
    }

    if (!std::filesystem::create_directories(m_capesDir, ec) && ec) {
        return Error(ErrorCode::FileOpenFailed, "Failed to create capes directory");
    }

    return {};
}

void SkinCache::_scanExistingFiles()
{
    std::lock_guard<std::mutex> lock(m_entriesMutex);
    m_totalCacheSize = 0;

    // 扫描皮肤文件
    if (std::filesystem::exists(m_skinsDir)) {
        for (const auto& subdir : std::filesystem::directory_iterator(m_skinsDir)) {
            if (subdir.is_directory()) {
                for (const auto& file : std::filesystem::directory_iterator(subdir.path())) {
                    if (file.is_regular_file()) {
                        std::string hash = file.path().filename().string();
                        CacheEntry entry;
                        entry.hash = hash;
                        entry.location = generateSkinLocation(hash);
                        entry.lastModified = std::filesystem::last_write_time(file.path());
                        entry.lastAccess = entry.lastModified;
                        entry.fileSize = static_cast<size_t>(file.file_size());
                        m_skinEntries[hash] = entry;
                        m_totalCacheSize += entry.fileSize;
                    }
                }
            }
        }
    }

    // 扫描披风文件
    if (std::filesystem::exists(m_capesDir)) {
        for (const auto& subdir : std::filesystem::directory_iterator(m_capesDir)) {
            if (subdir.is_directory()) {
                for (const auto& file : std::filesystem::directory_iterator(subdir.path())) {
                    if (file.is_regular_file()) {
                        std::string hash = file.path().filename().string();
                        CacheEntry entry;
                        entry.hash = hash;
                        entry.location = generateCapeLocation(hash);
                        entry.lastModified = std::filesystem::last_write_time(file.path());
                        entry.lastAccess = entry.lastModified;
                        entry.fileSize = static_cast<size_t>(file.file_size());
                        m_capeEntries[hash] = entry;
                        m_totalCacheSize += entry.fileSize;
                    }
                }
            }
        }
    }
}

void SkinCache::_loadMetadata()
{
    // TODO: 实现元数据加载，从 metadata.json 文件读取缓存条目信息
    // 当前简化实现：不持久化元数据，每次启动时重新扫描文件
}

void SkinCache::_saveMetadata()
{
    // TODO: 实现元数据保存，将缓存条目信息写入 metadata.json 文件
    // 当前简化实现：不持久化元数据
}

std::filesystem::path SkinCache::_getCacheFilePath(const std::string& type, const std::string& hash) const
{
    // 使用哈希前2个字符作为子目录名
    if (hash.length() >= 2) {
        return m_cacheDir / type / hash.substr(0, 2) / hash;
    }
    return m_cacheDir / type / hash;
}

bool SkinCache::hasSkin(const std::string& hash) const
{
    return _hasTexture("skins", hash);
}

std::optional<std::filesystem::path> SkinCache::getSkinPath(const std::string& hash) const
{
    std::filesystem::path path = _getCacheFilePath("skins", hash);
    if (std::filesystem::exists(path)) {
        return path;
    }
    return std::nullopt;
}

Result<std::filesystem::path> SkinCache::saveSkin(const std::string& hash, const std::vector<u8>& data)
{
    return _saveTexture("skins", hash, data);
}

Result<std::vector<u8>> SkinCache::readSkin(const std::string& hash) const
{
    return _readTexture("skins", hash);
}

bool SkinCache::removeSkin(const std::string& hash)
{
    auto path = getSkinPath(hash);
    if (path.has_value()) {
        std::error_code ec;
        if (std::filesystem::remove(*path, ec)) {
            std::lock_guard<std::mutex> lock(m_entriesMutex);
            auto it = m_skinEntries.find(hash);
            if (it != m_skinEntries.end()) {
                m_totalCacheSize -= it->second.fileSize;
                m_skinEntries.erase(it);
            }
            return true;
        }
    }
    return false;
}

bool SkinCache::hasCape(const std::string& hash) const
{
    return _hasTexture("capes", hash);
}

Result<std::filesystem::path> SkinCache::saveCape(const std::string& hash, const std::vector<u8>& data)
{
    return _saveTexture("capes", hash, data);
}

Result<std::vector<u8>> SkinCache::readCape(const std::string& hash) const
{
    return _readTexture("capes", hash);
}

bool SkinCache::_hasTexture(const std::string& type, const std::string& hash) const
{
    std::filesystem::path path = _getCacheFilePath(type, hash);
    return std::filesystem::exists(path);
}

Result<std::filesystem::path> SkinCache::_saveTexture(
    const std::string& type, const std::string& hash, const std::vector<u8>& data)
{
    std::filesystem::path path = _getCacheFilePath(type, hash);
    std::filesystem::path parentDir = path.parent_path();

    std::error_code ec;
    std::filesystem::create_directories(parentDir, ec);
    if (ec) {
        return Error(ErrorCode::FileOpenFailed, "Failed to create directory: " + parentDir.string());
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileWriteFailed, "Failed to open file for writing: " + path.string());
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) {
        return Error(ErrorCode::FileWriteFailed, "Failed to write data to file: " + path.string());
    }

    {
        std::lock_guard<std::mutex> lock(m_entriesMutex);
        CacheEntry entry;
        entry.hash = hash;
        entry.lastAccess = std::filesystem::file_time_type::clock::now();
        entry.lastModified = entry.lastAccess;
        entry.fileSize = data.size();

        if (type == "skins") {
            entry.location = generateSkinLocation(hash);
            auto it = m_skinEntries.find(hash);
            if (it != m_skinEntries.end()) {
                m_totalCacheSize -= it->second.fileSize;
            }
            m_skinEntries[hash] = entry;
        } else if (type == "capes") {
            entry.location = generateCapeLocation(hash);
            auto it = m_capeEntries.find(hash);
            if (it != m_capeEntries.end()) {
                m_totalCacheSize -= it->second.fileSize;
            }
            m_capeEntries[hash] = entry;
        }

        m_totalCacheSize += entry.fileSize;
    }

    return path;
}

Result<std::vector<u8>> SkinCache::_readTexture(const std::string& type, const std::string& hash) const
{
    std::filesystem::path path = _getCacheFilePath(type, hash);

    if (!std::filesystem::exists(path)) {
        return Error(ErrorCode::FileNotFound, "Texture not found in cache: " + hash);
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open file for reading: " + path.string());
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        return Error(ErrorCode::FileReadFailed, "Failed to read data from file: " + path.string());
    }

    // 更新访问时间
    const_cast<SkinCache*>(this)->_updateAccessTime(hash);

    return data;
}

void SkinCache::_updateAccessTime(const std::string& hash)
{
    std::lock_guard<std::mutex> lock(m_entriesMutex);

    auto skinIt = m_skinEntries.find(hash);
    if (skinIt != m_skinEntries.end()) {
        skinIt->second.lastAccess = std::filesystem::file_time_type::clock::now();
        return;
    }

    auto capeIt = m_capeEntries.find(hash);
    if (capeIt != m_capeEntries.end()) {
        capeIt->second.lastAccess = std::filesystem::file_time_type::clock::now();
    }
}

ResourceLocation SkinCache::generateSkinLocation(const std::string& hash) const
{
    // 格式: minecraft:skins/ab/abcdef...
    if (hash.length() >= 2) {
        return ResourceLocation("minecraft:skins/" + hash.substr(0, 2) + "/" + hash);
    }
    return ResourceLocation("minecraft:skins/" + hash);
}

ResourceLocation SkinCache::generateCapeLocation(const std::string& hash) const
{
    // 格式: minecraft:capes/ab/abcdef...
    if (hash.length() >= 2) {
        return ResourceLocation("minecraft:capes/" + hash.substr(0, 2) + "/" + hash);
    }
    return ResourceLocation("minecraft:capes/" + hash);
}

void SkinCache::cleanExpired(std::chrono::seconds maxAge)
{
    auto now = std::filesystem::file_time_type::clock::now();
    auto cutoff = now - maxAge;

    std::lock_guard<std::mutex> lock(m_entriesMutex);

    // 清理过期皮肤
    for (auto it = m_skinEntries.begin(); it != m_skinEntries.end();) {
        if (it->second.lastAccess < cutoff) {
            std::filesystem::path path = _getCacheFilePath("skins", it->first);
            std::error_code ec;
            if (std::filesystem::exists(path) && std::filesystem::remove(path, ec)) {
                m_totalCacheSize -= it->second.fileSize;
            }
            it = m_skinEntries.erase(it);
        } else {
            ++it;
        }
    }

    // 清理过期披风
    for (auto it = m_capeEntries.begin(); it != m_capeEntries.end();) {
        if (it->second.lastAccess < cutoff) {
            std::filesystem::path path = _getCacheFilePath("capes", it->first);
            std::error_code ec;
            if (std::filesystem::exists(path) && std::filesystem::remove(path, ec)) {
                m_totalCacheSize -= it->second.fileSize;
            }
            it = m_capeEntries.erase(it);
        } else {
            ++it;
        }
    }
}

void SkinCache::clearAll()
{
    std::lock_guard<std::mutex> lock(m_entriesMutex);

    std::error_code ec;

    // 清理皮肤目录
    if (std::filesystem::exists(m_skinsDir)) {
        std::filesystem::remove_all(m_skinsDir, ec);
        std::filesystem::create_directories(m_skinsDir, ec);
    }

    // 清理披风目录
    if (std::filesystem::exists(m_capesDir)) {
        std::filesystem::remove_all(m_capesDir, ec);
        std::filesystem::create_directories(m_capesDir, ec);
    }

    m_skinEntries.clear();
    m_capeEntries.clear();
    m_totalCacheSize = 0;

    spdlog::info("SkinCache: Cleared all cache");
}

size_t SkinCache::cacheSize() const noexcept
{
    return m_totalCacheSize;
}

size_t SkinCache::cacheCount() const
{
    std::lock_guard<std::mutex> lock(m_entriesMutex);
    return m_skinEntries.size() + m_capeEntries.size();
}

} // namespace mc::skin
