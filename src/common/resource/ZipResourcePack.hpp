#pragma once

#include "common/resource/IResourcePack.hpp"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

namespace mc {

/**
 * @brief ZIP 资源包实现
 *
 * 从 ZIP 文件读取资源，支持标准的 Minecraft 资源包格式。
 * 使用 libarchive 库进行解压。
 */
class ZipResourcePack : public IResourcePack {
public:
    /**
     * @brief 析构函数
     */
    ~ZipResourcePack() override;

    /**
     * @brief 创建 ZIP 资源包
     * @param zipPath ZIP 文件路径
     * @return 资源包实例或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ZipResourcePack>> create(const std::filesystem::path& zipPath);

    // IResourcePack 接口实现

    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(StringView resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(StringView resourcePath) const override;
    [[nodiscard]] Result<std::vector<String>> listResources(
        StringView directory,
        StringView extension = "") const override;
    [[nodiscard]] String name() const override { return m_name; }

    // 额外方法

    [[nodiscard]] const std::filesystem::path& zipPath() const { return m_zipPath; }
    [[nodiscard]] size_t entryCount() const { return m_entries.size(); }
    void clearCache();

private:
    /// 私有构造函数
    explicit ZipResourcePack(std::filesystem::path zipPath);

    /**
     * @brief 规范化资源路径
     */
    [[nodiscard]] static String normalizePath(StringView path);

    std::filesystem::path m_zipPath;    ///< ZIP 文件路径
    String m_name;                       ///< 资源包名称（文件名）
    PackMetadata m_metadata;             ///< 元数据
    std::unordered_set<String> m_entries; ///< 文件路径索引

    /// 可变缓存（mutable 以支持 const 方法中的缓存）
    mutable std::unordered_map<String, std::vector<u8>> m_cache;
    mutable std::shared_mutex m_cacheMutex;
};

} // namespace mc
