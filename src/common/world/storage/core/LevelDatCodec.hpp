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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/storage/list/WorldListEntry.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::world::storage {

/**
 * @brief level.dat 版本信息
 */
struct LevelVersionInfo {
    /// 存储格式版本（19133 = Anvil）
    i32 storageVersion;

    /// 数据版本（原版 MC 版本号）
    i32 dataVersion;

    /// 版本名称（如 "1.16.5"）
    std::string versionName;

    /// 是否为快照版本
    bool snapshot;

    LevelVersionInfo(i32 storageVersion, i32 dataVersion, std::string versionName, bool snapshot);

    // 禁止默认构造
    LevelVersionInfo() = delete;
};

/**
 * @brief level.dat 最小摘要数据
 *
 * 用于世界列表显示，不包含完整世界状态。
 */
struct LevelSummaryData {
    /// 显示名称
    std::string displayName;

    /// 最后游玩时间（毫秒时间戳）
    i64 lastPlayedMs;

    /// 游戏模式
    GameMode gameMode;

    /// 难度
    Difficulty difficulty;

    /// 是否极限模式
    bool hardcore;

    /// 是否允许作弊
    bool allowCommands;

    /// 世界种子
    u64 seed;

    /// 世界类型
    WorldType worldType;

    /// 世界预设资源位置（数据驱动装配查 WorldPresetRegistry，如 "minecraft:normal"）
    resource::ResourceLocation worldPresetId;

    /// 版本信息
    LevelVersionInfo version;

    /// 存储格式版本（用于判断是否需要转换）
    i32 storageVersion;

    /// 数据版本（用于判断未来/旧版本）
    i32 dataVersion;

    /// 兼容性状态
    WorldCompatibility compatibility;

    /// 错误信息（如有）
    std::string errorMessage;

    /**
     * @brief 构造摘要数据
     */
    LevelSummaryData(std::string displayName,
        i64 lastPlayedMs,
        GameMode gameMode,
        Difficulty difficulty,
        bool hardcore,
        bool allowCommands,
        u64 seed,
        WorldType worldType,
        resource::ResourceLocation worldPresetId,
        LevelVersionInfo version,
        i32 storageVersion,
        i32 dataVersion,
        WorldCompatibility compatibility,
        std::string errorMessage);

    LevelSummaryData() = delete;
};

/**
 * @brief level.dat 运行时数据
 *
 * 包含启动世界所需的完整信息。
 */
struct LevelRuntimeData {
    /// 摘要数据
    LevelSummaryData summary;

    /// 出生点 X
    i32 spawnX;

    /// 出生点 Y
    i32 spawnY;

    /// 出生点 Z
    i32 spawnZ;

    /// 出生点朝向（角度）
    f32 spawnAngle;

    /// 游戏时间（刻）
    i64 gameTime;

    /// 日照时间（刻）
    i64 dayTime;

    /// 清晰天气剩余时间（刻）
    i32 clearWeatherTime;

    /// 下雨剩余时间（刻）
    i32 rainTime;

    /// 是否正在下雨
    bool raining;

    /// 雷暴剩余时间（刻）
    i32 thunderTime;

    /// 是否正在雷暴
    bool thundering;

    /// 世界是否已初始化
    bool initialized;

    /// 难度是否锁定
    bool difficultyLocked;

    /**
     * @brief 构造运行时数据
     */
    LevelRuntimeData(LevelSummaryData summary,
        i32 spawnX,
        i32 spawnY,
        i32 spawnZ,
        f32 spawnAngle,
        i64 gameTime,
        i64 dayTime,
        i32 clearWeatherTime,
        i32 rainTime,
        bool raining,
        i32 thunderTime,
        bool thundering,
        bool initialized,
        bool difficultyLocked);

    LevelRuntimeData() = delete;
};

/**
 * @brief level.dat 读写编解码器
 *
 * 负责 gzip NBT 文件的读写和 MC Java 1.16.5 兼容性。
 * 第一版只实现最小摘要读写，后续可扩展完整运行时数据。
 */
class LevelDatCodec {
public:
    /// 当前支持的存储格式版本（Anvil = 19133）
    static constexpr i32 CURRENT_STORAGE_VERSION = 19133;

    /// 当前数据版本（MC 1.16.5 = 2586）
    static constexpr i32 CURRENT_DATA_VERSION = 2586;

    /// 项目名称（用于版本名称）
    static constexpr const char* PROJECT_NAME = "Cubium";

    /// 项目数据版本（用于未来兼容）
    static constexpr i32 PROJECT_DATA_VERSION = 1;

    /**
     * @brief 从世界目录读取 level.dat 摘要
     *
     * 优先读取 level.dat，失败则尝试 level.dat_old。
     * 文件是 gzip 压缩的 Java NBT。
     *
     * @param worldDir 世界目录路径
     * @return 成功返回摘要数据，失败返回错误
     */
    static Result<LevelSummaryData> readSummary(const std::filesystem::path& worldDir);

    /**
     * @brief 从已读取的 NBT 复合标签解析摘要
     *
     * @param data NBT Data 复合标签
     * @return 成功返回摘要，失败返回错误
     */
    static Result<LevelSummaryData> parseSummary(const nbt::tags::compound_tag& data);

    /**
     * @brief 写入初始 level.dat
     *
     * 创建新世界时写入最小 level.dat。
     * 采用临时文件 + level.dat_old 备份策略。
     *
     * @param worldDir 世界目录路径（必须已存在）
     * @param request 创建世界请求
     * @return 成功返回空，失败返回错误
     */
    static Result<void> writeInitial(const std::filesystem::path& worldDir, const CreateWorldRequest& request);

    /**
     * @brief 更新 level.dat 中的显示名称
     *
     * @param worldDir 世界目录路径
     * @param newDisplayName 新的显示名称
     * @return 成功返回空，失败返回错误
     */
    static Result<void> updateDisplayName(const std::filesystem::path& worldDir, const std::string& newDisplayName);

    /**
     * @brief 更新 level.dat 中的最后游玩时间
     *
     * @param worldDir 世界目录路径
     * @param lastPlayedMs 最后游玩时间（毫秒时间戳）
     * @return 成功返回空，失败返回错误
     */
    static Result<void> updateLastPlayed(const std::filesystem::path& worldDir, i64 lastPlayedMs);

    /**
     * @brief 读取完整运行时数据
     *
     * 用于启动世界时读取所有必要字段。
     *
     * @param worldDir 世界目录路径
     * @return 成功返回运行时数据，失败返回错误
     */
    static Result<LevelRuntimeData> readRuntimeData(const std::filesystem::path& worldDir);

    /**
     * @brief 更新 level.dat 中的运行时数据
     *
     * 用于保存世界时写入时间、天气、出生点等运行时字段。
     *
     * @param worldDir 世界目录路径
     * @param gameTime 游戏时间（刻）
     * @param dayTime 日光时间（刻）
     * @param spawnX 出生点 X
     * @param spawnY 出生点 Y
     * @param spawnZ 出生点 Z
     * @param spawnAngle 出生点朝向（角度）
     * @param clearWeatherTime 清晰天气剩余时间（刻）
     * @param rainTime 下雨剩余时间（刻）
     * @param raining 是否正在下雨
     * @param thunderTime 雷暴剩余时间（刻）
     * @param thundering 是否正在雷暴
     * @param initialized 世界是否已完成首次出生点初始化（false 时下次启动会重新计算出生点）
     * @return 成功返回空，失败返回错误
     */
    static Result<void> updateRuntimeData(const std::filesystem::path& worldDir,
        i64 gameTime,
        i64 dayTime,
        i32 spawnX,
        i32 spawnY,
        i32 spawnZ,
        f32 spawnAngle,
        i32 clearWeatherTime,
        i32 rainTime,
        bool raining,
        i32 thunderTime,
        bool thundering,
        bool initialized);

    /**
     * @brief 从 level.dat 读取调度事件列表
     *
     * 读取 Data 复合标签下的 ScheduledEvents 列表标签。
     * 如果列表不存在或为空，返回空列表。
     *
     * @param worldDir 世界目录路径
     * @return 成功返回 NBT 复合列表，失败返回错误
     */
    static Result<std::unique_ptr<nbt::tags::compound_list_tag>> readScheduledEvents(
        const std::filesystem::path& worldDir);

    /**
     * @brief 更新 level.dat 中的调度事件列表
     *
     * 将 ScheduledEvents 列表标签写入 Data 复合标签。
     * 如果列表为空，则删除 ScheduledEvents 键。
     *
     * @param worldDir 世界目录路径
     * @param events 调度事件 NBT 列表（可为空）
     * @return 成功返回空，失败返回错误
     */
    static Result<void> updateScheduledEvents(
        const std::filesystem::path& worldDir, const nbt::tags::compound_list_tag& events);

private:
    /**
     * @brief 从 gzip 文件读取 NBT
     */
    static Result<std::unique_ptr<nbt::tags::compound_tag>> _readGzipNbt(const std::filesystem::path& filePath);

    /**
     * @brief 将 NBT 写入 gzip 文件
     */
    static Result<void> _writeGzipNbt(const std::filesystem::path& filePath, const nbt::tags::compound_tag& root);

    /**
     * @brief 构建最小 level.dat NBT 结构
     */
    static std::unique_ptr<nbt::tags::compound_tag> _buildInitialNbt(
        const CreateWorldRequest& request, i64 lastPlayedMs);

    /**
     * @brief 原子写入：临时文件 -> level.dat_old 备份 -> 替换 level.dat
     */
    static Result<void> _atomicWrite(const std::filesystem::path& worldDir, const nbt::tags::compound_tag& root);

    /**
     * @brief 读取 Data 复合标签
     *
     * 通用辅助方法，用于更新操作。
     *
     * @param worldDir 世界目录
     * @param outRoot 输出的根 NBT
     * @param outData 输出的 Data 复合标签
     * @return 成功返回 true，失败返回错误
     */
    static Result<void> _readDataCompound(const std::filesystem::path& worldDir,
        std::unique_ptr<nbt::tags::compound_tag>& outRoot,
        nbt::tags::compound_tag*& outData);

    /**
     * @brief 从 NBT 解析世界类型
     */
    static WorldType _parseWorldType(const nbt::tags::compound_tag& data);

    /**
     * @brief 从 NBT 解析世界预设资源位置（Data.Reborn.WorldPresetId）
     *
     * 缺失时默认 "minecraft:normal"。
     */
    static resource::ResourceLocation _parseWorldPresetId(const nbt::tags::compound_tag& data);

    /**
     * @brief 将 WorldType + worldPresetId 一并写入 Data.Reborn compound
     *
     * fetch-or-create Reborn compound 后一次性写 WorldType 与 WorldPresetId 两键，
     * 避免分两次 emplace("Reborn") 造成后者静默丢弃先写者。同时写兼容字段 generatorName。
     */
    static void _writeReborn(
        nbt::tags::compound_tag& data, WorldType worldType, const resource::ResourceLocation& worldPresetId);

    /**
     * @brief 从 NBT 解析游戏模式
     */
    static GameMode _parseGameMode(const nbt::tags::compound_tag& data);

    /**
     * @brief 将游戏模式写入 NBT
     */
    static void _writeGameMode(nbt::tags::compound_tag& data, GameMode gameMode);

    /**
     * @brief 从 NBT 解析难度
     */
    static Difficulty _parseDifficulty(const nbt::tags::compound_tag& data);

    /**
     * @brief 将难度写入 NBT
     */
    static void _writeDifficulty(nbt::tags::compound_tag& data, Difficulty difficulty);

    /**
     * @brief 判断兼容性状态
     */
    static WorldCompatibility _determineCompatibility(i32 storageVersion, i32 dataVersion);
};

} // namespace mc::world::storage
