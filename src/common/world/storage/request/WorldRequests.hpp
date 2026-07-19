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

#include "common/core/Types.hpp"
#include "common/world/WorldConfig.hpp"
#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 创建世界请求
 *
 * 用户从创建世界界面提交的数据，用于创建新世界并初始化 level.dat。
 * 所有字段均由调用方显式填充。
 */
struct CreateWorldRequest {
    /// 世界显示名称
    std::string displayName;

    /// 用户请求的目录名（可为空，由系统生成）
    std::string requestedLevelId;

    /// 世界种子
    u64 seed;

    /// 世界类型
    WorldType worldType;

    /// 世界预设资源位置（数据驱动装配查 WorldPresetRegistry，如 "minecraft:normal"）
    resource::ResourceLocation worldPresetId;

    /// 游戏模式
    GameMode gameMode;

    /// 难度
    Difficulty difficulty;

    /// 是否为极限模式
    bool hardcore;

    /// 是否允许作弊
    bool allowCommands;

    /// 视距
    i32 viewDistance;

    /**
     * @brief 构造创建世界请求
     *
     * 所有字段必须显式提供。
     */
    CreateWorldRequest(std::string displayName,
        std::string requestedLevelId,
        u64 seed,
        WorldType worldType,
        resource::ResourceLocation worldPresetId,
        GameMode gameMode,
        Difficulty difficulty,
        bool hardcore,
        bool allowCommands,
        i32 viewDistance);

    // 禁止默认构造
    CreateWorldRequest() = delete;
};

/**
 * @brief 加载世界请求
 *
 * 从存档列表选择已有世界时提交的数据。
 */
struct LoadWorldRequest {
    /// 世界目录名
    std::string levelId;

    /// 是否允许加载未来版本的世界
    bool allowFutureVersion;

    /// 是否在升级前创建备份
    bool createBackupBeforeUpgrade;

    /// 是否允许存储格式转换
    bool allowStorageConversion;

    /**
     * @brief 构造加载世界请求
     */
    LoadWorldRequest(
        std::string levelId, bool allowFutureVersion, bool createBackupBeforeUpgrade, bool allowStorageConversion);

    // 禁止默认构造
    LoadWorldRequest() = delete;
};

/**
 * @brief 重命名世界请求
 */
struct RenameWorldRequest {
    std::string levelId;
    std::string newDisplayName;

    RenameWorldRequest(std::string levelId, std::string newDisplayName);

    RenameWorldRequest() = delete;
};

/**
 * @brief 删除世界请求
 */
struct DeleteWorldRequest {
    std::string levelId;

    explicit DeleteWorldRequest(std::string levelId);

    DeleteWorldRequest() = delete;
};

/**
 * @brief 备份世界请求
 */
struct BackupWorldRequest {
    std::string levelId;
    std::string reason;

    BackupWorldRequest(std::string levelId, std::string reason);

    BackupWorldRequest() = delete;
};

/**
 * @brief 备份世界结果
 */
struct BackupWorldResult {
    std::filesystem::path zipPath;
    u64 sizeBytes;

    BackupWorldResult(std::filesystem::path zipPath, u64 sizeBytes);

    BackupWorldResult() = delete;
};

} // namespace mc::world::storage
