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

#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 存档目录路径配置
 *
 * 提供 saves 和 backups 目录的路径解析。第一版使用工作目录下的 saves/，
 * 后续可迁移到平台用户数据目录。
 *
 * 目录结构：
 * saves/
 * └── {levelId}/
 *     ├── level.dat              # 世界元数据（NBT格式）
 *     ├── session.lock           # 会话锁
 *     ├── icon.png               # 世界图标
 *     ├── db/                    # RocksDB数据库目录
 *     │   ├── chunks/            # 区块数据（Section粒度）
 *     │   ├── entities/          # 实体数据
 *     │   ├── poi/               # 兴趣点数据
 *     │   └── snapshots/         # 快照元数据
 *     ├── snapshots/             # 快照数据目录
 *     │   └── {snapshot_id}/     # 具体快照
 *     │       ├── manifest.json  # 快照清单
 *     │       └── delta/         # 增量数据
 *     └── import/                # 导入临时目录
 *         ├── java/              # Java版存档导入
 *         └── bedrock/           # 基岩版存档导入
 */
class WorldStoragePaths {
public:
    /**
     * @brief 构造存档路径配置
     * @param savesDir 存档根目录路径，如果为空则使用工作目录下的 "saves"
     * @param backupsDir 备份目录路径，如果为空则使用 savesDir 同级的 "backups"
     */
    explicit WorldStoragePaths(std::filesystem::path savesDir, std::filesystem::path backupsDir);

    /**
     * @brief 使用默认路径构造
     *
     * saves 目录为当前工作目录下的 "saves"，
     * backups 目录为 saves 同级的 "backups"。
     */
    static WorldStoragePaths defaultPaths();

    // ============================================================================
    // 基础路径
    // ============================================================================

    /**
     * @brief 获取存档根目录
     */
    [[nodiscard]] const std::filesystem::path& savesDir() const noexcept;

    /**
     * @brief 获取备份目录
     */
    [[nodiscard]] const std::filesystem::path& backupsDir() const noexcept;

    /**
     * @brief 获取指定世界目录路径
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path worldDir(const std::string& levelId) const;

    // ============================================================================
    // 传统文件路径
    // ============================================================================

    /**
     * @brief 获取指定世界的 level.dat 路径
     */
    [[nodiscard]] std::filesystem::path levelDatPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的 level.dat_old 备份路径
     */
    [[nodiscard]] std::filesystem::path levelDatOldPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的 session.lock 路径
     */
    [[nodiscard]] std::filesystem::path sessionLockPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的图标路径
     */
    [[nodiscard]] std::filesystem::path iconPath(const std::string& levelId) const;

    // ============================================================================
    // RocksDB 数据库路径（自有格式）
    // ============================================================================

    /**
     * @brief 获取指定世界的数据库根目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path dbPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的区块数据目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path dbChunksPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的实体数据目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path dbEntitiesPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的兴趣点数据目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path dbPoiPath(const std::string& levelId) const;

    /**
     * @brief 获取指定世界的快照元数据目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path dbSnapshotsMetaPath(const std::string& levelId) const;

    // ============================================================================
    // 快照路径（版本控制）
    // ============================================================================

    /**
     * @brief 获取指定世界的快照目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path snapshotsPath(const std::string& levelId) const;

    /**
     * @brief 获取指定快照的目录
     * @param levelId 世界目录名
     * @param snapshotId 快照ID
     */
    [[nodiscard]] std::filesystem::path snapshotPath(const std::string& levelId, const std::string& snapshotId) const;

    /**
     * @brief 获取指定快照的清单文件路径
     * @param levelId 世界目录名
     * @param snapshotId 快照ID
     */
    [[nodiscard]] std::filesystem::path snapshotManifestPath(
        const std::string& levelId, const std::string& snapshotId) const;

    /**
     * @brief 获取指定快照的增量数据目录
     * @param levelId 世界目录名
     * @param snapshotId 快照ID
     */
    [[nodiscard]] std::filesystem::path snapshotDeltaPath(
        const std::string& levelId, const std::string& snapshotId) const;

    // ============================================================================
    // 导入路径（格式转换）
    // ============================================================================

    /**
     * @brief 获取指定世界的导入临时目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path importPath(const std::string& levelId) const;

    /**
     * @brief 获取 Java 版导入目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path importJavaPath(const std::string& levelId) const;

    /**
     * @brief 获取基岩版导入目录
     * @param levelId 世界目录名
     */
    [[nodiscard]] std::filesystem::path importBedrockPath(const std::string& levelId) const;

    // ============================================================================
    // 目录创建
    // ============================================================================

    /**
     * @brief 确保存档根目录存在
     * @return 成功返回 true，失败返回 false
     */
    bool ensureSavesDirExists() const;

    /**
     * @brief 确保备份目录存在
     * @return 成功返回 true，失败返回 false
     */
    bool ensureBackupsDirExists() const;

    /**
     * @brief 确保世界目录结构存在
     * @param levelId 世界目录名
     * @return 成功返回 true，失败返回 false
     */
    bool ensureWorldDirExists(const std::string& levelId) const;

    /**
     * @brief 确保数据库目录结构存在
     * @param levelId 世界目录名
     * @return 成功返回 true，失败返回 false
     */
    bool ensureDbDirExists(const std::string& levelId) const;

private:
    std::filesystem::path m_savesDir;
    std::filesystem::path m_backupsDir;
};

} // namespace mc::world::storage
