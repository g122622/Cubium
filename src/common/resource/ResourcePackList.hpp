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
#include "common/core/settings/ResourcePackListOption.hpp"
#include "common/resource/FolderResourcePack.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/ZipResourcePack.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace mc {

/**
 * @brief 资源包列表管理器
 *
 * 管理多个客户端资源包，支持文件夹和 ZIP 格式的资源包。
 * 实现类似 Minecraft 1.16.5 的资源加载优先级系统：
 * -
 * 高优先级的资源包后加载，其资源会覆盖低优先级的同名资源
 * - 查找资源时，先从高优先级资源包查找
 *
 * 使用示例:
 * @code
 * ResourcePackList packList;
 *
 * // 扫描目录
 * auto result = packList.scanDirectory("resourcepacks");
 * if (result.success()) {
 *     spdlog::info("Found {} packs", result.value());
 * }
 *
 * // 获取启用的资源包（按优先级排序）
 * auto packs = packList.getEnabledPacks();
 *
 * // 查找资源
 * auto data = packList.readResource("minecraft/textures/block/stone.png");
 *
 * // 从设置同步
 * packList.loadFromSettings(settings.resourcePacks);
 * packList.saveToSettings(settings.resourcePacks);
 * @endcode
 */
class ResourcePackList {
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
     * @brief 默认构造函数
     */
    ResourcePackList() = default;

    /**
     * @brief 析构函数
     */
    ~ResourcePackList() = default;

    // 禁止拷贝
    ResourcePackList(const ResourcePackList&) = delete;
    ResourcePackList& operator=(const ResourcePackList&) = delete;

    // 允许移动
    ResourcePackList(ResourcePackList&&) = default;
    ResourcePackList& operator=(ResourcePackList&&) = default;

    // ========================================================================
    // 资源包管理
    // ========================================================================

    /**
     * @brief 扫描目录发现资源包
     *
     * 扫描指定目录，发现所有 ZIP 文件和文件夹形式的资源包。
     * 自动调用每个资源包的 initialize() 方法。
     *
     * @param dir 要扫描的目录
     * @return 发现的资源包数量，或错误
     */
    [[nodiscard]] Result<size_t> scanDirectory(const std::filesystem::path& dir);

    /**
     * @brief 添加资源包
     *
     * @param path 资源包路径（ZIP 文件或目录）
     * @param enabled 是否启用
     * @param priority 优先级
     * @return 添加的资源包信息，或错误
     */
    [[nodiscard]] Result<PackInfo> addPack(const std::filesystem::path& path, bool enabled, i32 priority);

    /**
     * @brief 移除资源包
     * @param path 资源包路径
     * @return 是否成功移除
     */
    bool removePack(const std::string& path);

    /**
     * @brief 清空所有资源包
     */
    void clear();

    // ========================================================================
    // 启用/禁用和优先级
    // ========================================================================

    /**
     * @brief 启用或禁用资源包
     * @param path 资源包路径
     * @param enabled 是否启用
     * @return 是否成功更新
     */
    bool setEnabled(const std::string& path, bool enabled);

    /**
     * @brief 设置资源包优先级
     * @param path 资源包路径
     * @param priority 新优先级
     * @return 是否成功更新
     */
    bool setPriority(const std::string& path, i32 priority);

    /**
     * @brief 向上移动资源包（增加优先级）
     * @param path 资源包路径
     * @return 是否成功移动
     */
    bool moveUp(const std::string& path);

    /**
     * @brief 向下移动资源包（降低优先级）
     * @param path 资源包路径
     * @return 是否成功移动
     */
    bool moveDown(const std::string& path);

    // ========================================================================
    // 查询方法
    // ========================================================================

    /**
     * @brief 获取所有资源包信息
     * @return 资源包信息列表
     */
    [[nodiscard]] std::vector<PackInfo> getAllPacks() const;

    /**
     * @brief 获取启用的资源包（按优先级排序）
     *
     * 返回按优先级降序排列的资源包列表。
     * 查找资源时按此顺序遍历，高优先级先查找。
     *
     * @return 启用的资源包列表
     */
    [[nodiscard]] std::vector<ResourcePackPtr> getEnabledPacks() const;

    /**
     * @brief 获取启用的资源包信息（按优先级排序）
     * @return 启用的资源包信息列表
     */
    [[nodiscard]] std::vector<PackInfo> getEnabledPackInfos() const;

    /**
     * @brief 查找资源包信息（返回拷贝）
     *
     * 为了支持主线程与音频线程并发访问，本接口返回 PackInfo 的拷贝，
     * 避免暴露内部容器元素地址导致悬垂指针问题。
     *
     * @param path 资源包路径
     * @return 找到则返回 PackInfo，否则返回空
     */
    [[nodiscard]] std::optional<PackInfo> getPackInfo(const std::string& path) const;

    /**
     * @brief 判断资源包是否存在
     *
     * 这是并发安全的只读查询，不会暴露内部容器元素地址。
     *
     * @param path 资源包路径
     * @return 是否已存在
     */
    [[nodiscard]] bool containsPack(const std::string& path) const;

    /**
     * @brief 获取资源包数量
     */
    [[nodiscard]] size_t packCount() const;

    /**
     * @brief 获取启用的资源包数量
     */
    [[nodiscard]] size_t enabledPackCount() const;

    // ========================================================================
    // 资源访问
    // ========================================================================

    /**
     * @brief 检查资源是否存在
     *
     * 按优先级从高到低检查各资源包。
     *
     * @param resourcePath 资源路径（相对于 assets/ 根，如 "minecraft/textures/block/stone.png"）
     * @return
     * 是否存在
     */
    [[nodiscard]] bool hasResource(std::string_view resourcePath) const;

    /**
     * @brief 读取资源
     *
     * 按优先级从高到低查找资源，返回第一个找到的。
     *
     * @param resourcePath 资源路径
     * @return 资源数据，或错误
     */
    [[nodiscard]] Result<std::vector<u8>> readResource(std::string_view resourcePath) const;

    /**
     * @brief 读取文本资源
     * @param resourcePath 资源路径
     * @return 文本内容，或错误
     */
    [[nodiscard]] Result<std::string> readTextResource(std::string_view resourcePath) const;

    /**
     * @brief 列出资源
     *
     * 汇总所有启用的资源包中的资源列表。
     *
     * @param directory 目录路径
     * @param extension 文件扩展名过滤（可选）
     * @return 资源路径列表
     */
    [[nodiscard]] Result<std::vector<std::string>> listResources(
        std::string_view directory, std::string_view extension) const;

    /**
     * @brief 获取所有客户端资源命名空间
     *
     * @return 命名空间列表（如 ["minecraft", "mymod"]）

     */
    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces() const;

    // ========================================================================
    // 设置同步
    // ========================================================================

    /**
     * @brief 从设置加载资源包配置
     *
     * 根据设置中的资源包列表启用/禁用资源包并设置优先级。
     * 不会添加新的资源包，只会更新已存在资源包的状态。
     *
     * @param settings 设置选项
     */
    void loadFromSettings(const ResourcePackListOption& settings);

    /**
     * @brief 保存资源包配置到设置
     *
     * 将当前资源包状态保存到设置。
     *
     * @param settings 设置选项
     */
    void saveToSettings(ResourcePackListOption& settings) const;

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

    /**
     * @brief 通知变更
     */
    void notifyChange();

    /**
     * @brief 规范化路径
     */
    [[nodiscard]] static std::string normalizePath(const std::filesystem::path& path);

    /**
     * @brief 检查是否是 ZIP 文件
     */
    [[nodiscard]] static bool isZipFile(const std::filesystem::path& path);

    /**
     * @brief 检查是否是有效的资源包目录
     */
    [[nodiscard]] static bool isResourcePackDir(const std::filesystem::path& path);
};

} // namespace mc
