#pragma once

#include "../../../core/Types.hpp"
#include "../../../world/dimension/DimensionSettings.hpp"
#include <string>
#include <optional>

namespace mc::world::save::data {

/**
 * @brief 游戏模式枚举
 */
enum class GameType : i32 {
    Survival = 0,    ///< 生存模式
    Creative = 1,    ///< 创造模式
    Adventure = 2,   ///< 冒险模式
    Spectator = 3    ///< 旁观者模式
};

/**
 * @brief 难度枚举
 */
enum class Difficulty : i32 {
    Peaceful = 0,    ///< 和平
    Easy = 1,        ///< 简单
    Normal = 2,      ///< 普通
    Hard = 3         ///< 困难
};

/**
 * @brief 世界设置
 *
 * 创建新世界时的设置。
 * 参考 MC 1.16.5 WorldSettings.java
 *
 * ## 使用示例
 * ```cpp
 * WorldSettings settings;
 * settings.levelName = "My World";
 * settings.seed = 12345;
 * settings.gameType = GameType::Survival;
 * settings.generateStructures = true;
 * ```
 */
struct WorldSettings {
    // ========== 基本信息 ==========
    String levelName;           ///< 世界名称
    i64 seed = 0;               ///< 世界种子
    GameType gameType = GameType::Survival;  ///< 游戏模式
    Difficulty difficulty = Difficulty::Normal;  ///< 难度

    // ========== 世界生成 ==========
    bool generateStructures = true;    ///< 是否生成结构
    bool bonusChest = false;           ///< 是否生成奖励箱
    String generatorName = "default";  ///< 生成器名称（default/flat/large_biomes/amplified等）
    String generatorOptions;           ///< 生成器选项（如 flat 世界的层设置）

    // ========== 游戏规则 ==========
    bool hardcore = false;             ///< 是否极限模式
    bool allowCommands = false;        ///< 是否允许作弊
    bool difficultyLocked = false;     ///< 难度是否锁定

    // ========== 默认值 ==========
    static WorldSettings survival(const String& name, i64 seed);
    static WorldSettings creative(const String& name, i64 seed);
    static WorldSettings flat(const String& name);
};

/**
 * @brief 世界版本信息
 *
 * 参考 MC 1.16.5 VersionData.java
 */
struct VersionData {
    i32 id = 2586;               ///< 版本 ID（MC 1.16.5 = 2586）
    String name = "1.16.5";      ///< 版本名称
    bool stable = true;          ///< 是否稳定版
    bool snapshot = false;       ///< 是否快照
};

/**
 * @brief 世界边界设置
 */
struct WorldBorderSettings {
    f64 centerX = 0.0;           ///< 中心 X
    f64 centerZ = 0.0;           ///< 中心 Z
    f64 size = 60000000.0;       ///< 边界大小（默认 6000 万格）
    f64 sizeL = 60000000.0;      ///< 边界大小（长整型版本）
    f64 safeZone = 5.0;          ///< 安全区宽度
    f64 damagePerBlock = 0.2;    ///< 每格伤害
    f64 warningBlocks = 5.0;     ///< 警告距离
    f64 warningTime = 15.0;      ///< 警告时间（秒）
};

} // namespace mc::world::save::data
