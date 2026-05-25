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
 * LIABILITY, WHO IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
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
 * @brief 数据包列表管理器
 *
 * 专门管理服务端数据包。
 * 数据包资源路径统一相对于 data/ 根目录，禁止再经由通用 PackType 资源接口分流。
 *
 * 数据包目录结构遵循 Minecraft 1.16.5 规范：
 * @code
 * datapacks/
 * ├── MyDataPack/
 * │   ├── pack.mcmeta
 * │   └── data/
 * │       └── minecraft/
 * │           ├── loot_tables/
 * │           ├── recipes/
 * │           ├── tags/
 * │           └── advancements/
 * └── AnotherPack.zip
 * @endcode
 *
 * 使用示例：
 * @code
 * DataPackList dataPacks;
 * auto count = dataPacks.scanDirectory("datapacks");
 *
 * // 读取数据包中的战利品表
 * auto data = dataPacks.readResource("minecraft/loot_tables/blocks/stone.json");
 *
 * // 列出所有命名空间
 * auto namespaces = dataPacks.getResourceNamespaces();
 * @endcode
 */
class DataPackList {
public:
    /**
     * @brief 数据包信息结构
     */
    struct PackInfo {
        std::string path;         ///< 数据包路径
        ResourcePackPtr pack;     ///< 资源包实例
        bool enabled = true;      ///< 是否启用
        i32 priority = 0;         ///< 优先级（越大越优先）
        bool isZip = false;       ///< 是否是 ZIP 文件
        bool initialized = false; ///< 是否已初始化
        std::string error;        ///< 初始化错误信息
    };

    /**
     * @brief 默认构造函数
     */
    DataPackList() = default;

    /**
     * @brief 析构函数
     */
    ~DataPackList() = default;

    // 禁止拷贝
    DataPackList(const DataPackList&) = delete;
    DataPackList& operator=(const DataPackList&) = delete;

    // 禁止移动（包含 std::shared_mutex）
    DataPackList(DataPackList&&) = delete;
    DataPackList& operator=(DataPackList&&) = delete;

    // ========================================================================
    // 数据包管理
    // ========================================================================

    /**
     * @brief 扫描目录发现数据包
     *
     * @param dir 要扫描的目录
     * @return 发现的数据包数量，或错误
     */
    [[nodiscard]] Result<size_t> scanDirectory(const std::filesystem::path& dir);

    /**
     * @brief 添加数据包
     *
     * @param path 数据包路径（ZIP 文件或目录）
     * @param enabled 是否启用
     * @param priority 优先级
     * @return 添加的数据包信息，或错误
     */
    [[nodiscard]] Result<PackInfo> addPack(const std::filesystem::path& path, bool enabled, i32 priority);

    /**
     * @brief 移除数据包
     * @param path 数据包路径
     * @return 是否成功移除
     */
    bool removePack(const std::string& path);

    /**
     * @brief 清空所有数据包
     */
    void clear();

    // ========================================================================
    // 启用/禁用和优先级
    // ========================================================================

    /**
     * @brief 启用或禁用数据包
     */
    bool setEnabled(const std::string& path, bool enabled);

    /**
     * @brief 设置数据包优先级
     */
    bool setPriority(const std::string& path, i32 priority);

    /**
     * @brief 向上移动数据包（增加优先级）
     */
    bool moveUp(const std::string& path);

    /**
     * @brief 向下移动数据包（降低优先级）
     */
    bool moveDown(const std::string& path);

    // ========================================================================
    // 查询方法
    // ========================================================================

    /**
     * @brief 获取所有数据包信息
     */
    [[nodiscard]] std::vector<PackInfo> getAllPacks() const;

    /**
     * @brief 获取启用的数据包（按优先级排序）
     */
    [[nodiscard]] std::vector<ResourcePackPtr> getEnabledPacks() const;

    /**
     * @brief 获取启用的数据包信息（按优先级排序）
     */
    [[nodiscard]] std::vector<PackInfo> getEnabledPackInfos() const;

    /**
     * @brief 查找数据包信息（返回拷贝）
     */
    [[nodiscard]] std::optional<PackInfo> getPackInfo(const std::string& path) const;

    /**
     * @brief 判断数据包是否存在
     */
    [[nodiscard]] bool containsPack(const std::string& path) const;

    /**
     * @brief 获取数据包数量
     */
    [[nodiscard]] size_t packCount() const;

    /**
     * @brief 获取启用的数据包数量
     */
    [[nodiscard]] size_t enabledPackCount() const;

    // ========================================================================
    // 资源访问（限定 PackType::ServerData）
    // ========================================================================

    /**
     * @brief 检查数据包中是否存在指定资源
     *
     * 资源路径相对于 data/ 目录，如 "minecraft/loot_tables/blocks/stone.json"
     *
     * @param resourcePath 资源路径（相对于 data/ 目录）
     * @return 是否存在
     */
    [[nodiscard]] bool hasResource(std::string_view resourcePath) const;

    /**
     * @brief 读取数据包中的资源
     *
     * @param resourcePath 资源路径（相对于 data/ 目录）
     * @return 资源数据，或错误
     */
    [[nodiscard]] Result<std::vector<u8>> readResource(std::string_view resourcePath) const;

    /**
     * @brief 读取数据包中的文本资源
     *
     * @param resourcePath 资源路径（相对于 data/ 目录）
     * @return 文本内容，或错误
     */
    [[nodiscard]] Result<std::string> readTextResource(std::string_view resourcePath) const;

    /**
     * @brief 列出数据包中指定目录下的资源
     *
     * @param directory 目录路径（相对于 data/ 目录，如 "minecraft/loot_tables"）
     * @param extension 文件扩展名过滤（可选）
     * @return 资源路径列表
     */
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        std::string_view directory, std::string_view extension) const;

    /**
     * @brief 获取所有命名空间
     *
     * @return 命名空间列表（如 ["minecraft", "mymod"]）
     */
    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces() const;

    // ========================================================================
    // 变更通知
    // ========================================================================

    /**
     * @brief 设置变更回调
     * @param callback 变更时调用的函数
     */
    void onChange(std::function<void()> callback);

private:
    std::vector<PackInfo> m_packs;
    std::function<void()> m_callback;
    mutable std::shared_mutex m_mutex;

    void notifyChange();

    [[nodiscard]] static std::string normalizePath(const std::filesystem::path& path);
    [[nodiscard]] static bool isZipFile(const std::filesystem::path& path);
    [[nodiscard]] static bool isDataPackDir(const std::filesystem::path& path);
};

} // namespace mc::resource
