#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include <string>
#include <unordered_map>

namespace mc::world::save::data {

/**
 * @brief 游戏规则集合
 *
 * 存储世界的游戏规则设置。
 * 参考 MC 1.16.5 GameRules.java
 *
 * ## 使用示例
 * ```cpp
 * GameRules rules;
 * rules.setDoDaylightCycle(false);
 * rules.setKeepInventory(true);
 *
 * // 序列化到 NBT
 * auto nbt = rules.serialize();
 *
 * // 从 NBT 加载
 * rules.deserialize(*nbt);
 * ```
 */
class GameRules {
public:
    /**
     * @brief 游戏规则类型
     */
    enum class Type : u8 {
        Boolean,  ///< 布尔型规则
        Integer,  ///< 整型规则
        Double    ///< 浮点型规则
    };

    /**
     * @brief 游戏规则键
     */
    enum class Key : u16 {
        // 布尔型规则
        DoDaylightCycle,       ///< 日光周期是否启用
        DoEntityDrops,         ///< 实体是否掉落物品
        DoFireTick,            ///< 火是否蔓延
        DoMobLoot,             ///< 生物是否掉落物品
        DoMobSpawning,         ///< 生物是否自然生成
        DoTileDrops,           ///< 方块是否掉落物品
        DoWeatherCycle,        ///< 天气周期是否启用
        KeepInventory,         ///< 死亡是否保留物品
        LogAdminCommands,      ///< 是否记录管理员命令
        MobGriefing,           ///< 生物是否能破坏方块
        NaturalRegeneration,   ///< 自然回血是否启用
        ReducedDebugInfo,      ///< 是否减少调试信息
        SendCommandFeedback,   ///< 是否发送命令反馈
        ShowDeathMessages,     ///< 是否显示死亡消息
        SpectatorsGenerateChunks, ///< 旁观者是否能生成区块
        DisableElytraMovementCheck, ///< 是否禁用鞘翅移动检查
        DoInsomnia,            ///< 是否启用幻翼生成
        DoLimitedCrafting,     ///< 是否限制合成
        DoPatrolSpawning,      ///< 是否生成掠夺者巡逻队
        DoTraderSpawning,      ///< 是否生成流浪商人
        FallDamage,            ///< 是否有摔落伤害
        FireDamage,            ///< 是否有火焰伤害
        FreezeDamage,          ///< 是否有冰冻伤害
        UniversalAnger,        ///< 仇恨是否通用
        ForgivingDeathMessages, ///< 死亡消息是否更宽容

        // 整型规则
        MaxCommandChainLength, ///< 命令链最大长度
        MaxEntityCramming,     ///< 实体堆叠上限
        RandomTickSpeed,       ///< 随机刻速度
        SpawnRadius,           ///< 出生点半径
        CommandBlockOutput,    ///< 命令方块输出

        // 浮点型规则（1.16+）
        PlayerSpawnAngle       ///< 玩家出生角度
    };

    GameRules();
    ~GameRules() = default;

    // 禁止拷贝
    GameRules(const GameRules&) = delete;
    GameRules& operator=(const GameRules&) = delete;

    // 允许移动
    GameRules(GameRules&&) noexcept = default;
    GameRules& operator=(GameRules&&) noexcept = default;

    // ========== 布尔型规则访问器 ==========

    [[nodiscard]] bool doDaylightCycle() const;
    void setDoDaylightCycle(bool value);

    [[nodiscard]] bool doEntityDrops() const;
    void setDoEntityDrops(bool value);

    [[nodiscard]] bool doFireTick() const;
    void setDoFireTick(bool value);

    [[nodiscard]] bool doMobLoot() const;
    void setDoMobLoot(bool value);

    [[nodiscard]] bool doMobSpawning() const;
    void setDoMobSpawning(bool value);

    [[nodiscard]] bool doTileDrops() const;
    void setDoTileDrops(bool value);

    [[nodiscard]] bool doWeatherCycle() const;
    void setDoWeatherCycle(bool value);

    [[nodiscard]] bool keepInventory() const;
    void setKeepInventory(bool value);

    [[nodiscard]] bool mobGriefing() const;
    void setMobGriefing(bool value);

    [[nodiscard]] bool naturalRegeneration() const;
    void setNaturalRegeneration(bool value);

    // ========== 整型规则访问器 ==========

    [[nodiscard]] i32 maxCommandChainLength() const;
    void setMaxCommandChainLength(i32 value);

    [[nodiscard]] i32 maxEntityCramming() const;
    void setMaxEntityCramming(i32 value);

    [[nodiscard]] i32 randomTickSpeed() const;
    void setRandomTickSpeed(i32 value);

    [[nodiscard]] i32 spawnRadius() const;
    void setSpawnRadius(i32 value);

    // ========== 浮点型规则访问器 ==========

    [[nodiscard]] f64 playerSpawnAngle() const;
    void setPlayerSpawnAngle(f64 value);

    // ========== 通用访问器 ==========

    /**
     * @brief 获取布尔型规则值
     * @param key 规则键
     * @param defaultValue 默认值（规则不存在时返回）
     * @return 规则值
     */
    [[nodiscard]] bool getBoolean(Key key, bool defaultValue = false) const;

    /**
     * @brief 设置布尔型规则值
     */
    void setBoolean(Key key, bool value);

    /**
     * @brief 获取整型规则值
     */
    [[nodiscard]] i32 getInteger(Key key, i32 defaultValue = 0) const;

    /**
     * @brief 设置整型规则值
     */
    void setInteger(Key key, i32 value);

    /**
     * @brief 获取浮点型规则值
     */
    [[nodiscard]] f64 getDouble(Key key, f64 defaultValue = 0.0) const;

    /**
     * @brief 设置浮点型规则值
     */
    void setDouble(Key key, f64 value);

    /**
     * @brief 通过名称设置规则值
     * @param name 规则名称（如 "doDaylightCycle"）
     * @param value 规则值字符串
     * @return 成功返回 true
     */
    bool setByName(const String& name, const String& value);

    /**
     * @brief 获取规则类型
     */
    [[nodiscard]] static Type getType(Key key);

    /**
     * @brief 获取规则名称
     */
    [[nodiscard]] static const char* getName(Key key);

    // ========== 序列化 ==========

    /**
     * @brief 序列化到 NBT
     */
    [[nodiscard]] std::unique_ptr<nbt::CompoundTag> serialize() const;

    /**
     * @brief 从 NBT 反序列化
     */
    void deserialize(const nbt::CompoundTag& nbt);

private:
    std::unordered_map<Key, bool> m_boolRules;
    std::unordered_map<Key, i32> m_intRules;
    std::unordered_map<Key, f64> m_doubleRules;

    void initializeDefaults();
};

} // namespace mc::world::save::data
