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
#include "common/resource/FolderResourcePack.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ZipResourcePack.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace mc::resource {

/**
 * @brief 资源包列表基类
 *
 * 提供资源包管理的公共逻辑，包括添加/移除/启用/禁用/优先级管理、
 * 资源查询、变更通知等。PackRepository 和 DataPackRepository 继承此类，
 * 分别针对客户端资源和服务端数据提供不同的默认 PackType。
 *
 * 线程安全：读操作使用 shared_lock，写操作使用 unique_lock。
 * 对外返回 PackInfo 一律按值拷贝，避免暴露内部容器元素地址导致悬垂指针。
 */
class PackListBase {
public:
    /**
     * @brief 资源包信息结构
     */
    struct PackInfo {
        std::string path;         ///< 资源包路径
        ResourcePackPtr pack;     ///< 资源包实例
        bool enabled = true;      ///< 是否启用
        i32 priority = 0;         ///< 优先级（越大越优先）
        bool isZip = false;       ///< 是否是 ZIP 文件
        bool initialized = false; ///< 是否已初始化
        std::string error;        ///< 初始化错误信息
    };

    /**
     * @brief 构造函数
     * @param defaultType 默认资源包类型
     */
    explicit PackListBase(PackType defaultType);

    /**
     * @brief 析构函数
     */
    virtual ~PackListBase() = default;

    // 禁止拷贝
    PackListBase(const PackListBase&) = delete;
    PackListBase& operator=(const PackListBase&) = delete;

    // 禁止移动（包含 std::shared_mutex）
    PackListBase(PackListBase&&) = delete;
    PackListBase& operator=(PackListBase&&) = delete;

    // ========================================================================
    // 资源包管理
    // ========================================================================

    [[nodiscard]] Result<size_t> scanDirectory(const std::filesystem::path& dir);
    [[nodiscard]] Result<PackInfo> addPack(const std::filesystem::path& path, bool enabled, i32 priority);
    bool removePack(const std::string& path);
    void clear();

    // ========================================================================
    // 启用/禁用和优先级
    // ========================================================================

    bool setEnabled(const std::string& path, bool enabled);
    bool setPriority(const std::string& path, i32 priority);
    bool moveUp(const std::string& path);
    bool moveDown(const std::string& path);

    // ========================================================================
    // 查询方法
    // ========================================================================

    [[nodiscard]] std::vector<PackInfo> getAllPacks() const;
    [[nodiscard]] std::vector<ResourcePackPtr> getEnabledPacks() const;
    [[nodiscard]] std::vector<PackInfo> getEnabledPackInfos() const;
    [[nodiscard]] std::optional<PackInfo> getPackInfo(const std::string& path) const;
    [[nodiscard]] bool containsPack(const std::string& path) const;
    [[nodiscard]] size_t packCount() const;
    [[nodiscard]] size_t enabledPackCount() const;

    // ========================================================================
    // 资源访问
    // ========================================================================

    [[nodiscard]] bool hasResource(PackType type, std::string_view resourcePath) const;
    [[nodiscard]] Result<std::vector<u8>> readResource(PackType type, std::string_view resourcePath) const;
    [[nodiscard]] Result<std::string> readTextResource(PackType type, std::string_view resourcePath) const;
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        PackType type, std::string_view directory, std::string_view extension) const;
    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(PackType type) const;

    // ========================================================================
    // 变更通知
    // ========================================================================

    void onChange(std::function<void()> callback);

protected:
    [[nodiscard]] PackType defaultType() const { return m_defaultType; }
    virtual void onPackListChanged();

private:
    PackType m_defaultType;
    std::vector<PackInfo> m_packs;
    std::function<void()> m_callback;
    mutable std::shared_mutex m_mutex;

    [[nodiscard]] static std::string _normalizePathKey(const std::filesystem::path& path);
    [[nodiscard]] static bool _isZipFile(const std::filesystem::path& path);
    [[nodiscard]] static bool _isPackDir(const std::filesystem::path& path);
    void _notifyChange();
};

} // namespace mc::resource

namespace mc {
using PackListBase = resource::PackListBase;
} // namespace mc
