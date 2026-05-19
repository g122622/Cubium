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
#include "world/storage/SingleLevelStorageManager.hpp"
#include "world/storage/core/WorldStoragePaths.hpp"
#include "world/storage/list/WorldListEntry.hpp"
#include "world/storage/list/WorldListService.hpp"
#include "world/storage/request/WorldRequests.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 跨存档全局存储门面
 *
 * 该类只负责“存档集合”层面的能力：
 * - 枚举存档列表
 * - 查询存档摘要
 * - 创建、删除、重命名存档
 * - 解析 saves/backups 根目录与单个存档目录
 * - 打开指定存档并返回对应的 `SingleLevelStorageManager`
 *
 * 该类不持有任何已打开存档的运行时状态。
 * 每次 `openLevel()` 都会创建一个新的单存档运行时门面，
 * 其生命周期由调用方显式持有和管理。
 */
class GlobalStorageManager {
public:
    /**
     * @brief 使用默认路径构造全局存储门面
     *
     * 默认使用 `WorldStoragePaths::defaultPaths()` 解析
     * 当前工作目录下的 `saves/` 与 `backups/`。
     */
    GlobalStorageManager();

    /**
     * @brief 列出所有存档
     * @return 世界列表条目集合，失败返回错误
     */
    [[nodiscard]] Result<std::vector<WorldListEntry>> listWorlds();

    /**
     * @brief 打开指定存档
     *
     * 该方法会根据 `levelId` 解析存档目录，创建新的
     * `SingleLevelStorageManager`，并执行 `open(...)`。
     *
     * 返回的运行时门面由调用方独占持有。
     *
     * @param levelId 存档目录名
     * @param config 单存档运行时存储配置
     * @return 已打开的单存档运行时门面
     */
    [[nodiscard]] Result<std::unique_ptr<SingleLevelStorageManager>> openLevel(
        const std::string& levelId, const SingleLevelStorageConfig& config);

    /**
     * @brief 获取 saves 根目录
     * @return saves 目录路径
     */
    [[nodiscard]] const std::filesystem::path& savesDirectory() const noexcept;

private:
    WorldStoragePaths m_paths = WorldStoragePaths::defaultPaths();
    WorldListService m_worldListService{m_paths};
};

} // namespace mc::world::storage
