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

#include <filesystem>
#include <string>

namespace mc {

/**
 * @brief 游戏目录管理器
 *
 * 统一管理所有游戏相关路径，替代原来分散的路径硬编码。
 * 所有游戏数据（配置、资源包、数据包、存档、日志等）均位于游戏根目录下。
 *
 * 目录结构：
 * @code
 * gameRoot/                        # 游戏根目录（默认 ~/minecraft_reborn/）
 * ├── client_options.json          # 客户端配置
 * ├── server_options.json          # 服务端配置
 * ├── resourcepacks/               # 客户端资源包目录
 * ├── datapacks/                   # 服务端数据包目录
 * ├── saves/                       # 存档目录
 * ├── backups/                     # 备份目录
 * ├── logs/                        # 日志目录
 * ├── cache/                       # 缓存目录
 * └── builtin/                     # 内置包目录（随游戏分发）
 *     └── minecraft/               # 内置原版包
 *         ├── pack.mcmeta
 *         ├── assets/
 *         └── data/
 * @endcode
 *
 * 使用示例：
 * @code
 * // 从配置文件路径推导游戏目录
 * auto gameDir = GameDirectory::fromConfigPath("~/minecraft_reborn/client_options.json");
 *
 * // 使用默认游戏目录
 * auto defaultDir = GameDirectory::defaultDirectory();
 *
 * // 访问路径
 * auto resourcePacks = gameDir.resourcePacksDir();
 * auto dataPacks = gameDir.dataPacksDir();
 *
 * // 确保目录结构存在
 * gameDir.ensureDirectoriesExist();
 * @endcode
 */
class GameDirectory {
public:
    /**
     * @brief 从配置文件路径推导游戏目录
     *
     * 配置文件路径的父目录即为游戏目录。
     * 如果配置文件路径为空，返回默认目录。
     *
     * @param configPath 配置文件路径（如 ~/minecraft_reborn/client_options.json）
     * @return GameDirectory 实例
     */
    [[nodiscard]] static GameDirectory fromConfigPath(const std::filesystem::path& configPath);

    /**
     * @brief 使用默认游戏目录
     *
     * 默认路径为 ~/minecraft_reborn/（跨平台展开 ~）
     *
     * @return GameDirectory 实例
     */
    [[nodiscard]] static GameDirectory defaultDirectory();

    /**
     * @brief 使用显式指定的游戏根目录
     *
     * @param root 游戏根目录路径
     * @return GameDirectory 实例
     */
    [[nodiscard]] static GameDirectory fromRoot(std::filesystem::path root);

    // 默认构造产生空目录，需通过工厂方法创建有效实例
    GameDirectory() = default;
    ~GameDirectory() = default;

    // 允许拷贝和移动
    GameDirectory(const GameDirectory&) = default;
    GameDirectory& operator=(const GameDirectory&) = default;
    GameDirectory(GameDirectory&&) = default;
    GameDirectory& operator=(GameDirectory&&) = default;

    // ========================================================================
    // 路径访问器
    // ========================================================================

    /**
     * @brief 获取游戏根目录
     * @return 游戏根目录的绝对路径
     */
    [[nodiscard]] const std::filesystem::path& root() const { return m_root; }

    /**
     * @brief 获取客户端配置文件路径
     * @return ~/minecraft_reborn/client_options.json
     */
    [[nodiscard]] std::filesystem::path clientOptionsPath() const;

    /**
     * @brief 获取服务端配置文件路径
     * @return ~/minecraft_reborn/server_options.json
     */
    [[nodiscard]] std::filesystem::path serverOptionsPath() const;

    /**
     * @brief 获取资源包目录
     * @return ~/minecraft_reborn/resourcepacks/
     */
    [[nodiscard]] std::filesystem::path resourcePacksDir() const;

    /**
     * @brief 获取数据包目录
     * @return ~/minecraft_reborn/datapacks/
     */
    [[nodiscard]] std::filesystem::path dataPacksDir() const;

    /**
     * @brief 获取存档目录
     * @return ~/minecraft_reborn/saves/
     */
    [[nodiscard]] std::filesystem::path savesDir() const;

    /**
     * @brief 获取备份目录
     * @return ~/minecraft_reborn/backups/
     */
    [[nodiscard]] std::filesystem::path backupsDir() const;

    /**
     * @brief 获取日志目录
     * @return ~/minecraft_reborn/logs/
     */
    [[nodiscard]] std::filesystem::path logsDir() const;

    /**
     * @brief 获取缓存目录
     * @return ~/minecraft_reborn/cache/
     */
    [[nodiscard]] std::filesystem::path cacheDir() const;

    /**
     * @brief 获取内置包目录
     *
     * 内置包位于可执行文件旁的 resources/builtin/ 目录下，
     * 随游戏分发，包含原版资源的基础定义。
     *
     * @return 可执行文件旁的 resources/builtin/ 路径
     */
    [[nodiscard]] std::filesystem::path builtinPackDir() const;

    // ========================================================================
    // 目录操作
    // ========================================================================

    /**
     * @brief 确保游戏目录结构存在
     *
     * 创建所有必要的子目录。如果目录不存在则创建。
     * 不会创建 builtinPackDir()，因为它是随游戏分发的。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> ensureDirectoriesExist() const;

    /**
     * @brief 检查游戏根目录是否有效
     * @return 根目录路径是否非空
     */
    [[nodiscard]] bool isValid() const { return !m_root.empty(); }

private:
    std::filesystem::path m_root;

    explicit GameDirectory(std::filesystem::path root);
};

} // namespace mc
