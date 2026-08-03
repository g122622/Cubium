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
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/PackMetadata.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::resource {

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
    ~ZipResourcePack() noexcept override;

    /**
     * @brief 创建 ZIP 资源包
     * @param zipPath ZIP 文件路径
     * @return 资源包实例或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ZipResourcePack>> create(const std::filesystem::path& zipPath);

    // IResourcePack 接口实现

    [[nodiscard]] Result<void> initialize() override;
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }
    [[nodiscard]] bool hasResource(PackType type, std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<u8>> readResource(PackType type, std::string_view resourcePath) const override;
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        PackType type, std::string_view directory, std::string_view extension) const override;
    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(PackType type) const override;
    [[nodiscard]] std::string name() const override { return m_name; }

    // 额外方法

    // 内部 API — 仅供诊断/管理代码使用
    [[nodiscard]] const std::filesystem::path& zipPath() const { return m_zipPath; }
    [[nodiscard]] size_t entryCount() const { return m_entries.size(); }
    void clearCache();

private:
    /// 私有构造函数
    explicit ZipResourcePack(std::filesystem::path zipPath);

    /**
     * @brief 规范化资源路径
     */
    [[nodiscard]] static std::string _normalizePath(std::string_view path);
    [[nodiscard]] static std::string _makeTypedPath(PackType type, std::string_view path);

    std::filesystem::path m_zipPath;           ///< ZIP 文件路径
    std::string m_name;                        ///< 资源包名称（文件名）
    PackMetadata m_metadata;                   ///< 元数据
    std::unordered_set<std::string> m_entries; ///< 文件路径索引

    /// 可变缓存（mutable 以支持 const 方法中的缓存）
    mutable std::unordered_map<std::string, std::vector<u8>> m_cache;
    mutable std::shared_mutex m_cacheMutex;
};

} // namespace mc::resource

namespace mc {
using ZipResourcePack = resource::ZipResourcePack;
} // namespace mc
