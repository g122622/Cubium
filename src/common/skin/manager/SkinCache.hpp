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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::skin {

/**
 * @brief 缓存条目
 *
 * 存储单个皮肤文件的缓存信息。
 */
struct CacheEntry {
    ResourceLocation location;                    // 纹理资源位置
    std::string hash;                             // 文件哈希
    std::filesystem::file_time_type lastAccess;   // 最后访问时间
    std::filesystem::file_time_type lastModified; // 最后修改时间
    size_t fileSize = 0;                          // 文件大小（字节）
};

/**
 * @brief 皮肤缓存管理器
 *
 * 负责皮肤文件的磁盘缓存管理：
 * - 缓存文件存储和检索
 * - 缓存清理策略
 * - 内存索引加速
 *
 * 缓存目录结构：
 * @code
 * cacheDir/
 * ├── skins/           # 皮肤文件
 * │   ├── ab/
 * │   │   └── abcdef123456...  # SHA1 哈希分目录存储
 * ├── capes/           # 披风文件
 * └── metadata.json    # 缓存元数据
 * @endcode
 *
 * 线程安全：所有公共方法都是线程安全的。
 */
class SkinCache {
public:
    /**
     * @brief 构造缓存管理器
     * @param cacheDir 缓存目录路径
     */
    explicit SkinCache(const std::string& cacheDir);

    /**
     * @brief 析构函数
     */
    ~SkinCache();

    // 禁止拷贝
    SkinCache(const SkinCache&) = delete;
    SkinCache& operator=(const SkinCache&) = delete;

    // 禁止移动（包含 std::mutex）
    SkinCache(SkinCache&&) noexcept = delete;
    SkinCache& operator=(SkinCache&&) noexcept = delete;

    /**
     * @brief 初始化缓存
     *
     * 创建必要的目录结构，加载元数据。
     *
     * @return 成功或错误
     */
    Result<void> initialize();

    /**
     * @brief 关闭缓存
     *
     * 保存元数据，清理资源。
     */
    void shutdown();

    // ========== 皮肤缓存操作 ==========

    /**
     * @brief 检查皮肤是否已缓存
     * @param hash 皮肤哈希（SHA1）
     * @return 是否已缓存
     */
    [[nodiscard]] bool hasSkin(const std::string& hash) const;

    /**
     * @brief 获取缓存的皮肤路径
     * @param hash 皮肤哈希
     * @return 文件路径，不存在返回空
     */
    [[nodiscard]] std::optional<std::filesystem::path> getSkinPath(const std::string& hash) const;

    /**
     * @brief 保存皮肤到缓存
     * @param hash 皮肤哈希（SHA1）
     * @param data PNG 数据
     * @return 缓存文件路径
     */
    Result<std::filesystem::path> saveSkin(const std::string& hash, const std::vector<u8>& data);

    /**
     * @brief 读取缓存的皮肤数据
     * @param hash 皮肤哈希
     * @return PNG 数据
     */
    Result<std::vector<u8>> readSkin(const std::string& hash) const;

    /**
     * @brief 删除缓存的皮肤
     * @param hash 皮肤哈希
     * @return 是否成功删除
     */
    bool removeSkin(const std::string& hash);

    // ========== 披风缓存操作 ==========

    [[nodiscard]] bool hasCape(const std::string& hash) const;
    Result<std::filesystem::path> saveCape(const std::string& hash, const std::vector<u8>& data);
    Result<std::vector<u8>> readCape(const std::string& hash) const;

    // ========== ResourceLocation 生成 ==========

    /**
     * @brief 生成皮肤的 ResourceLocation
     * @param hash 皮肤哈希
     * @return ResourceLocation（如 minecraft:skins/ab/abcdef...）
     */
    [[nodiscard]] ResourceLocation generateSkinLocation(const std::string& hash) const;

    /**
     * @brief 生成披风的 ResourceLocation
     * @param hash 披风哈希
     * @return ResourceLocation
     */
    [[nodiscard]] ResourceLocation generateCapeLocation(const std::string& hash) const;

    // ========== 缓存维护 ==========

    /**
     * @brief 清理过期缓存
     * @param maxAge 最大缓存时间
     */
    void cleanExpired(std::chrono::seconds maxAge);

    /**
     * @brief 清理所有缓存
     */
    void clearAll();

    /**
     * @brief 获取缓存大小（字节）
     */
    [[nodiscard]] size_t cacheSize() const noexcept;

    /**
     * @brief 获取缓存条目数量
     */
    [[nodiscard]] size_t cacheCount() const;

    /**
     * @brief 获取缓存目录路径
     */
    [[nodiscard]] const std::filesystem::path& cacheDir() const noexcept { return m_cacheDir; }

private:
    /**
     * @brief 获取缓存文件路径
     * @param type 类型（"skins" 或 "capes"）
     * @param hash 哈希值
     * @return 文件路径
     */
    std::filesystem::path _getCacheFilePath(const std::string& type, const std::string& hash) const;

    /**
     * @brief 从缓存目录扫描已有文件
     */
    void _scanExistingFiles();

    /**
     * @brief 加载元数据
     */
    void _loadMetadata();

    /**
     * @brief 保存元数据
     */
    void _saveMetadata();

    /**
     * @brief 更新访问时间
     * @param hash 哈希值
     */
    void _updateAccessTime(const std::string& hash);

    /**
     * @brief 确保目录存在
     */
    Result<void> _ensureDirectoriesExist();

    /**
     * @brief 通用保存方法
     */
    Result<std::filesystem::path> _saveTexture(
        const std::string& type, const std::string& hash, const std::vector<u8>& data);

    /**
     * @brief 通用读取方法
     */
    Result<std::vector<u8>> _readTexture(const std::string& type, const std::string& hash) const;

    /**
     * @brief 通用检查方法
     */
    [[nodiscard]] bool _hasTexture(const std::string& type, const std::string& hash) const;

    std::string m_cacheDirStr;
    std::filesystem::path m_cacheDir;
    std::filesystem::path m_skinsDir;
    std::filesystem::path m_capesDir;
    std::filesystem::path m_metadataPath;

    mutable std::mutex m_entriesMutex;
    std::unordered_map<std::string, CacheEntry> m_skinEntries;
    std::unordered_map<std::string, CacheEntry> m_capeEntries;

    std::atomic<bool> m_initialized{false};
    size_t m_totalCacheSize = 0;
};

} // namespace mc::skin
