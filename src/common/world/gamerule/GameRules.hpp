/**
 * @file GameRules.hpp
 * @brief 游戏规则容器类
 *
 * 参考 MC 1.16.5: net.minecraft.world.GameRules
 *
 * GameRules 类管理所有游戏规则的运行时值，支持：
 * - 规则值获取/设置
 * - 序列化到/从 NBT
 * - 变更通知
 * - 规则遍历
 */

#pragma once

#include "GameRule.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace mc::server {
class MinecraftServer;
}

namespace mc::world::gamerule {

// 前向声明
class GameRules;

/**
 * @brief 规则访问者接口
 *
 * 用于遍历所有游戏规则
 */
class IGameRuleVisitor {
public:
    virtual ~IGameRuleVisitor() = default;

    virtual void visitBoolean(const BooleanGameRuleKey& key, const BooleanGameRuleType& type) = 0;
    virtual void visitInteger(const IntegerGameRuleKey& key, const IntegerGameRuleType& type) = 0;
};

// ============================================================================
// 游戏规则键定义（静态常量）
// ============================================================================

namespace GameRuleKeys {

// ============================================================================
// 玩家相关 (Player)
// ============================================================================

/// 玩家死亡后是否保留物品栏
/// MC 1.16.5: GameRules.KEEP_INVENTORY
extern const BooleanGameRuleKey KEEP_INVENTORY;

/// 玩家是否自然恢复生命值
/// MC 1.16.5: GameRules.NATURAL_REGENERATION
extern const BooleanGameRuleKey NATURAL_REGENERATION;

/// 玩家重生半径
/// MC 1.16.5: GameRules.SPAWN_RADIUS
extern const IntegerGameRuleKey SPAWN_RADIUS;

/// 旁观者是否生成区块
/// MC 1.16.5: GameRules.SPECTATORS_GENERATE_CHUNKS
extern const BooleanGameRuleKey SPECTATORS_GENERATE_CHUNKS;

/// 是否禁用鞘翅移动检查
/// MC 1.16.5: GameRules.DISABLE_ELYTRA_MOVEMENT_CHECK
extern const BooleanGameRuleKey DISABLE_ELYTRA_MOVEMENT_CHECK;

/// 是否立即重生
/// MC 1.16.5: GameRules.DO_IMMEDIATE_RESPAWN
extern const BooleanGameRuleKey DO_IMMEDIATE_RESPAWN;

/// 是否溺水受伤
/// MC 1.16.5: GameRules.DROWNING_DAMAGE
extern const BooleanGameRuleKey DROWNING_DAMAGE;

/// 是否摔落受伤
/// MC 1.16.5: GameRules.FALL_DAMAGE
extern const BooleanGameRuleKey FALL_DAMAGE;

/// 是否火焰受伤
/// MC 1.16.5: GameRules.FIRE_DAMAGE
extern const BooleanGameRuleKey FIRE_DAMAGE;

/// 是否限制合成
/// MC 1.16.5: GameRules.DO_LIMITED_CRAFTING
extern const BooleanGameRuleKey DO_LIMITED_CRAFTING;

// ============================================================================
// 生物相关 (Mobs)
// ============================================================================

/// 生物是否能破坏方块
/// MC 1.16.5: GameRules.MOB_GRIEFING
extern const BooleanGameRuleKey MOB_GRIEFING;

/// 实体挤压上限
/// MC 1.16.5: GameRules.MAX_ENTITY_CRAMMING
extern const IntegerGameRuleKey MAX_ENTITY_CRAMMING;

/// 是否禁用袭击
/// MC 1.16.5: GameRules.DISABLE_RAIDS
extern const BooleanGameRuleKey DISABLE_RAIDS;

/// 中立生物是否原谅死亡玩家
/// MC 1.16.5: GameRules.FORGIVE_DEAD_PLAYERS
extern const BooleanGameRuleKey FORGIVE_DEAD_PLAYERS;

/// 通用愤怒机制
/// MC 1.16.5: GameRules.UNIVERSAL_ANGER
extern const BooleanGameRuleKey UNIVERSAL_ANGER;

// ============================================================================
// 生成相关 (Spawning)
// ============================================================================

/// 是否生成生物
/// MC 1.16.5: GameRules.DO_MOB_SPAWNING
extern const BooleanGameRuleKey DO_MOB_SPAWNING;

/// 是否生成幻翼
/// MC 1.16.5: GameRules.DO_INSOMNIA
extern const BooleanGameRuleKey DO_INSOMNIA;

/// 是否生成巡逻队
/// MC 1.16.5: GameRules.DO_PATROL_SPAWNING
extern const BooleanGameRuleKey DO_PATROL_SPAWNING;

/// 是否生成流浪商人
/// MC 1.16.5: GameRules.DO_TRADER_SPAWNING
extern const BooleanGameRuleKey DO_TRADER_SPAWNING;

// ============================================================================
// 掉落相关 (Drops)
// ============================================================================

/// 生物是否掉落物品
/// MC 1.16.5: GameRules.DO_MOB_LOOT
extern const BooleanGameRuleKey DO_MOB_LOOT;

/// 方块是否掉落物品
/// MC 1.16.5: GameRules.DO_TILE_DROPS
extern const BooleanGameRuleKey DO_TILE_DROPS;

/// 实体是否掉落物品（矿车等）
/// MC 1.16.5: GameRules.DO_ENTITY_DROPS
extern const BooleanGameRuleKey DO_ENTITY_DROPS;

// ============================================================================
// 更新相关 (Updates)
// ============================================================================

/// 火焰是否蔓延
/// MC 1.16.5: GameRules.DO_FIRE_TICK
extern const BooleanGameRuleKey DO_FIRE_TICK;

/// 日照循环是否进行
/// MC 1.16.5: GameRules.DO_DAYLIGHT_CYCLE
extern const BooleanGameRuleKey DO_DAYLIGHT_CYCLE;

/// 随机刻速度（影响作物生长、火焰蔓延等）
/// MC 1.16.5: GameRules.RANDOM_TICK_SPEED
extern const IntegerGameRuleKey RANDOM_TICK_SPEED;

/// 天气循环是否进行
/// MC 1.16.5: GameRules.DO_WEATHER_CYCLE
extern const BooleanGameRuleKey DO_WEATHER_CYCLE;

// ============================================================================
// 聊天相关 (Chat)
// ============================================================================

/// 命令方块是否输出到聊天
/// MC 1.16.5: GameRules.COMMAND_BLOCK_OUTPUT
extern const BooleanGameRuleKey COMMAND_BLOCK_OUTPUT;

/// 是否记录管理员命令
/// MC 1.16.5: GameRules.LOG_ADMIN_COMMANDS
extern const BooleanGameRuleKey LOG_ADMIN_COMMANDS;

/// 是否显示死亡消息
/// MC 1.16.5: GameRules.SHOW_DEATH_MESSAGES
extern const BooleanGameRuleKey SHOW_DEATH_MESSAGES;

/// 是否发送命令反馈
/// MC 1.16.5: GameRules.SEND_COMMAND_FEEDBACK
extern const BooleanGameRuleKey SEND_COMMAND_FEEDBACK;

/// 是否公布成就
/// MC 1.16.5: GameRules.ANNOUNCE_ADVANCEMENTS
extern const BooleanGameRuleKey ANNOUNCE_ADVANCEMENTS;

// ============================================================================
// 杂项 (Misc)
// ============================================================================

/// 是否减少调试信息
/// MC 1.16.5: GameRules.REDUCED_DEBUG_INFO
extern const BooleanGameRuleKey REDUCED_DEBUG_INFO;

/// 最大命令链长度
/// MC 1.16.5: GameRules.MAX_COMMAND_CHAIN_LENGTH
extern const IntegerGameRuleKey MAX_COMMAND_CHAIN_LENGTH;

} // namespace GameRuleKeys

/**
 * @brief 游戏规则容器类
 *
 * 管理所有游戏规则的运行时值。
 *
 * 使用示例：
 * @code
 * GameRules rules;
 *
 * // 获取规则值
 * bool mobGriefing = rules.getBoolean(GameRuleKeys::MOB_GRIEFING);
 * i32 tickSpeed = rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED);
 *
 * // 设置规则值
 * rules.setBoolean(GameRuleKeys::MOB_GRIEFING, false, server);
 * rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 6, server);
 *
 * // 序列化到 NBT
 * auto nbt = rules.write();
 *
 * // 从 NBT 加载
 * rules.read(*nbt);
 * @endcode
 */
class GameRules {
public:
    /**
     * @brief 默认构造函数
     *
     * 使用默认值初始化所有游戏规则
     */
    GameRules();

    /**
     * @brief 从 NBT 加载构造
     * @param nbt NBT 复合标签
     */
    explicit GameRules(const nbt::tags::compound_tag& nbt);

    /**
     * @brief 复制构造
     */
    GameRules(const GameRules& other);

    /**
     * @brief 赋值运算符
     */
    GameRules& operator=(const GameRules& other);

    // ============================================================================
    // 规则值获取
    // ============================================================================

    /**
     * @brief 获取布尔规则值
     * @param key 规则键
     * @return 规则值
     */
    [[nodiscard]] bool getBoolean(const BooleanGameRuleKey& key) const;

    /**
     * @brief 获取整数规则值
     * @param key 规则键
     * @return 规则值
     */
    [[nodiscard]] i32 getInt(const IntegerGameRuleKey& key) const;

    /**
     * @brief 获取布尔规则值对象
     * @param key 规则键
     * @return 规则值对象的引用
     */
    [[nodiscard]] const BooleanGameRuleValue& getBooleanValue(const BooleanGameRuleKey& key) const;
    [[nodiscard]] BooleanGameRuleValue& getBooleanValue(const BooleanGameRuleKey& key);

    /**
     * @brief 获取整数规则值对象
     * @param key 规则键
     * @return 规则值对象的引用
     */
    [[nodiscard]] const IntegerGameRuleValue& getIntegerValue(const IntegerGameRuleKey& key) const;
    [[nodiscard]] IntegerGameRuleValue& getIntegerValue(const IntegerGameRuleKey& key);

    // ============================================================================
    // 规则值设置
    // ============================================================================

    /**
     * @brief 设置布尔规则值
     * @param key 规则键
     * @param value 新值
     * @param server Minecraft 服务器实例（可选，用于触发变更监听器）
     */
    void setBoolean(const BooleanGameRuleKey& key, bool value, server::MinecraftServer* server = nullptr);

    /**
     * @brief 设置整数规则值
     * @param key 规则键
     * @param value 新值
     * @param server Minecraft 服务器实例（可选）
     */
    void setInt(const IntegerGameRuleKey& key, i32 value, server::MinecraftServer* server = nullptr);

    /**
     * @brief 从字符串设置规则值
     * @param ruleName 规则名称
     * @param value 字符串值
     * @param server Minecraft 服务器实例（可选）
     * @return 是否设置成功（规则存在且值有效）
     */
    bool setFromString(
        const std::string& ruleName, const std::string& value, server::MinecraftServer* server = nullptr);

    // ============================================================================
    // 序列化
    // ============================================================================

    /**
     * @brief 序列化到 NBT
     * @return NBT 复合标签
     *
     * 输出格式：
     * @code
     * {
     *     "mobGriefing": "true",
     *     "naturalRegeneration": "true",
     *     "randomTickSpeed": "3",
     *     ...
     * }
     * @endcode
     */
    [[nodiscard]] std::unique_ptr<nbt::tags::compound_tag> write() const;

    /**
     * @brief 从 NBT 加载
     * @param nbt NBT 复合标签
     */
    void read(const nbt::tags::compound_tag& nbt);

    // ============================================================================
    // 规则遍历
    // ============================================================================

    /**
     * @brief 遍历所有已注册的游戏规则
     * @param visitor 访问者
     */
    static void visitAll(IGameRuleVisitor& visitor);

    /**
     * @brief 获取规则名称列表
     * @return 所有规则名称
     */
    [[nodiscard]] static std::vector<std::string> getRuleNames();

    /**
     * @brief 检查规则是否存在
     * @param ruleName 规则名称
     * @return 是否存在
     */
    [[nodiscard]] static bool hasRule(const std::string& ruleName);

    /**
     * @brief 获取规则类型
     * @param ruleName 规则名称
     * @return 规则类型（如果不存在返回 std::nullopt）
     */
    [[nodiscard]] static std::optional<GameRuleValueType> getRuleType(const std::string& ruleName);

    // ============================================================================
    // 重置
    // ============================================================================

    /**
     * @brief 重置所有规则为默认值
     */
    void resetAll();

    /**
     * @brief 重置指定规则为默认值
     * @param ruleName 规则名称
     * @param server Minecraft 服务器实例（可选）
     * @return 是否重置成功
     */
    bool reset(const std::string& ruleName, server::MinecraftServer* server = nullptr);

private:
    // 布尔规则存储
    std::unordered_map<std::string, BooleanGameRuleValue> m_booleanRules;

    // 整数规则存储
    std::unordered_map<std::string, IntegerGameRuleValue> m_integerRules;

    // 初始化所有规则
    void initializeRules();

    // 从复合标签获取布尔值（辅助方法）
    static bool getBooleanFromNbt(const nbt::tags::compound_tag& nbt, const std::string& key, bool defaultValue);

    // 从复合标签获取整数值（辅助方法）
    static i32 getIntFromNbt(const nbt::tags::compound_tag& nbt, const std::string& key, i32 defaultValue);
};

} // namespace mc::world::gamerule
