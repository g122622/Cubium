#pragma once

#include "IResourcePack.hpp"
#include <filesystem>

namespace mc {

/**
 * @brief 文件夹资源包实现
 *
 * 从文件系统目录读取资源
 * 目录结构应符合Minecraft资源包格式
 */
class FolderResourcePack : public IResourcePack {
public:
    explicit FolderResourcePack(std::string rootPath);
    ~FolderResourcePack() override = default;

    // IResourcePack接口实现
    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        std::string_view directory,
        std::string_view extension = "") const override;
    [[nodiscard]] std::string name() const override { return m_name; }

    // 获取根路径
    [[nodiscard]] const std::string& rootPath() const { return m_rootPath; }

private:
    std::string m_rootPath;
    std::string m_name;
    PackMetadata m_metadata;

    // 规范化路径
    [[nodiscard]] std::string normalizePath(std::string_view resourcePath) const;
};

} // namespace mc
