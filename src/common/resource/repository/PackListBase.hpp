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
#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/ZipResourcePack.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
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

    /**
     * @brief 读取同一资源路径在所有已启用数据包中的文本内容
     *
     * 与 readTextResource 只返回最高优先级数据包的内容不同，此方法遍历所有
     * 已启用数据包，收集同一资源路径的所有版本。返回结果按数据包优先级从
     * 高到低排序（与 getEnabledPacks 的顺序一致），每个条目包含数据包名称
     * 和资源内容。
     *
     * 这对于标签系统的多数据包合并至关重要：MC Java 的标签加载需要按
     * 数据包优先级从高到低遍历同名标签文件，replace=true 时清空已有条目后
     * 追加，默认追加。
     *
     * @param type 资源包类型
     * @param resourcePath 相对于类型根目录的资源路径
     * @return 成功时返回 (packName, content) 对的向量；若没有任何数据包包含
     *         该资源则返回 ResourceNotFound 错误
     */
    struct ResourceVersion {
        std::string packName; ///< 数据包名称
        std::string content;  ///< 资源文本内容
    };
    [[nodiscard]] Result<std::vector<ResourceVersion>> readAllResourceVersions(
        PackType type, std::string_view resourcePath) const;

    /**
     * @brief 列出目录下的所有资源路径及其在各数据包中的内容栈
     *
     * 与 listResources 只返回去重后的路径列表不同，此方法对每个资源路径
     * 收集所有数据包中的文本内容。返回结果为 map，键为资源路径，值为
     * ResourceVersion 向量（按数据包优先级从高到低排序）。
     *
     * @param type 资源包类型
     * @param directory 相对于类型根目录的目录前缀
     * @param extension 文件扩展名过滤（如 ".json"）
     * @return 成功时返回 路径 -> 内容栈 的映射；若没有任何数据包包含
     *         匹配资源则返回空映射（非错误）
     */
    [[nodiscard]] Result<std::map<std::string, std::vector<ResourceVersion>>> listResourceStacks(
        PackType type, std::string_view directory, std::string_view extension) const;

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

    // 已启用且已初始化 pack 的排序缓存（按 priority 降序）。
    // 加载批次内 pack 列表恒定（无 loader 在循环内改 pack 状态），排序结果可跨文件复用，
    // 避免每次 readResource/hasResource/listResources 都走 shared_lock+复制+stable_sort。
    // _notifyChange 失效（全部 9 个变更方法最终都调它）。mutable 因 readResource 等 const 方法写入。
    mutable std::optional<std::vector<PackInfo>> m_enabledPackInfosCache;

    [[nodiscard]] static std::string _normalizePathKey(const std::filesystem::path& path);
    [[nodiscard]] static bool _isZipFile(const std::filesystem::path& path);
    [[nodiscard]] static bool _isPackDir(const std::filesystem::path& path);
    void _notifyChange();
};

} // namespace mc::resource

namespace mc {
using PackListBase = resource::PackListBase;
} // namespace mc
