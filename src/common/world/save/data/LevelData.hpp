#pragma once

#include "WorldSettings.hpp"
#include "GameRules.hpp"
#include "../core/DataVersion.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include <memory>
#include <chrono>

namespace mc::world::save::data {

/**
 * @brief 世界元数据
 *
 * 对应 level.dat 文件的内容。
 * 参考 MC 1.16.5 ServerWorldInfo.java
 *
 * ## 使用示例
 * ```cpp
 * LevelData levelData;
 * levelData.levelName = "My World";
 * levelData.seed = 12345;
 * levelData.gameTime = 0;
 * levelData.dayTime = 0;
 *
 * // 序列化到 NBT
 * auto nbt = levelData.serialize();
 *
 * // 从 NBT 加载
 * auto result = LevelData::deserialize(*nbt);
 * if (result.success()) {
 *     levelData = std::move(result.value());
 * }
 * ```
 */
class LevelData {
public:
    // ========== 版本信息 ==========
    i32 dataVersion = DataVersion::CURRENT;  ///< 数据版本
    VersionData version;                      ///< 版本信息

    // ========== 世界设置 ==========
    String levelName;                         ///< 世界名称
    GameType gameType = GameType::Survival;   ///< 游戏模式
    bool hardcore = false;                    ///< 是否极限模式
    bool allowCommands = false;               ///< 是否允许作弊
    bool initialized = false;                 ///< 是否已初始化

    // ========== 生成点 ==========
    i32 spawnX = 0;                           ///< 出生点 X
    i32 spawnY = 64;                          ///< 出生点 Y
    i32 spawnZ = 0;                           ///< 出生点 Z
    f32 spawnAngle = 0.0f;                    ///< 出生点角度

    // ========== 时间 ==========
    i64 gameTime = 0;                         ///< 游戏总刻数
    i64 dayTime = 0;                          ///< 一天内的时间 (0-23999)
    i64 lastPlayed = 0;                       ///< 最后游玩时间戳（毫秒）

    // ========== 天气 ==========
    i32 clearWeatherTime = 0;                 ///< 晴天剩余时间
    i32 rainTime = 0;                         ///< 降雨计时器
    bool raining = false;                     ///< 是否降雨
    i32 thunderTime = 0;                      ///< 雷暴计时器
    bool thundering = false;                  ///< 是否雷暴

    // ========== 难度 ==========
    Difficulty difficulty = Difficulty::Normal;  ///< 难度
    bool difficultyLocked = false;            ///< 难度是否锁定

    // ========== 世界边界 ==========
    WorldBorderSettings worldBorder;

    // ========== 世界生成 ==========
    i64 randomSeed = 0;                       ///< 世界种子
    String generatorName = "default";         ///< 生成器名称
    bool generateFeatures = true;             ///< 是否生成结构
    bool bonusChest = false;                  ///< 是否生成奖励箱

    // ========== 游戏规则 ==========
    GameRules gameRules;

    // ========== 其他 ==========
    i32 wanderingTraderSpawnDelay = 0;        ///< 流浪商人生成延迟
    i32 wanderingTraderSpawnChance = 0;       ///< 流浪商人生成概率
    bool wasModded = false;                   ///< 是否被 mod 修改过

    // ========== 构造函数 ==========

    LevelData() = default;
    ~LevelData() = default;

    // 允许拷贝和移动
    LevelData(const LevelData&) = default;
    LevelData& operator=(const LevelData&) = default;
    LevelData(LevelData&&) = default;
    LevelData& operator=(LevelData&&) = default;

    // ========== 从设置创建 ==========

    /**
     * @brief 从世界设置创建 LevelData
     *
     * @param settings 世界设置
     * @return LevelData 实例
     */
    [[nodiscard]] static LevelData fromSettings(const WorldSettings& settings);

    // ========== 序列化 ==========

    /**
     * @brief 序列化到 NBT
     *
     * @param playerNbt 可选的玩家数据（单人模式）
     * @return NBT 复合标签
     */
    [[nodiscard]] std::unique_ptr<nbt::CompoundTag>
    serialize(const nbt::CompoundTag* playerNbt = nullptr) const;

    /**
     * @brief 从 NBT 反序列化
     *
     * @param nbt NBT 数据
     * @return 成功返回 LevelData，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<LevelData>>
    deserialize(const nbt::CompoundTag& nbt);

    // ========== 兼容性检查 ==========

    /**
     * @brief 检查数据版本是否兼容
     *
     * @return 如果可以加载返回 true
     */
    [[nodiscard]] bool isCompatible() const;

    // ========== 时间工具 ==========

    /**
     * @brief 获取天数
     *
     * @return 已过去的天数
     */
    [[nodiscard]] i64 dayCount() const {
        return gameTime / 24000;
    }

    /**
     * @brief 获取当前月相（0-7）
     *
     * @return 月相
     */
    [[nodiscard]] i32 moonPhase() const {
        return static_cast<i32>(dayCount() % 8);
    }

    /**
     * @brief 更新最后游玩时间
     */
    void updateLastPlayed() {
        lastPlayed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

private:
    void serializeVersion(nbt::CompoundTag& nbt) const;
    void serializeWorldGenSettings(nbt::CompoundTag& nbt) const;
    void serializeGameRules(nbt::CompoundTag& nbt) const;
    void serializeWorldBorder(nbt::CompoundTag& nbt) const;
    void serializeWeather(nbt::CompoundTag& nbt) const;

    static void deserializeVersion(LevelData& data, const nbt::CompoundTag& nbt);
    static void deserializeWorldGenSettings(LevelData& data, const nbt::CompoundTag& nbt);
    static void deserializeGameRules(LevelData& data, const nbt::CompoundTag& nbt);
    static void deserializeWorldBorder(LevelData& data, const nbt::CompoundTag& nbt);
    static void deserializeWeather(LevelData& data, const nbt::CompoundTag& nbt);
};

} // namespace mc::world::save::data
