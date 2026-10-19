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

/**
 * @file GameRules.hpp
 * @brief 游戏规则容器类
 *
 * GameRules 类管理所有游戏规则的运行时值，支持：
 * - 规则值获取/设置
 * - 序列化到/从 NBT
 * - 变更通知
 * - 规则遍历
 */

#pragma once

#include "GameRule.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

/**
 * @brief 规则值类名
 *
 * 两个名称分别取自 vanilla GameRuleType.BOOLEAN/GAME_RULE_TYPE_INTEGER.value()。vanilla 的
 * /gamerule 命令按规则值的类名区分「查询」与「设置」两个字面量子节点，故这里同样以类名
 * 而非 Boolean/Integer 枚举区分。
 */
namespace GameRuleClassNames {
inline constexpr std::string_view BOOLEAN = "boolean";
inline constexpr std::string_view INTEGER = "integer";
} // namespace GameRuleClassNames

// ============================================================================
// 游戏规则键定义（静态常量）
// ============================================================================

namespace GameRuleKeys {

// ============================================================================
// 玩家相关 (Player)
// ============================================================================

/// 玩家死亡后是否保留物品栏
extern const BooleanGameRuleKey KEEP_INVENTORY;
/** @brief 是否启用玩家导航条。 */
extern const BooleanGameRuleKey LOCATOR_BAR;

/// 玩家是否自然恢复生命值
extern const BooleanGameRuleKey NATURAL_REGENERATION;

/// 玩家重生半径
extern const IntegerGameRuleKey SPAWN_RADIUS;

/// 旁观者是否生成区块
extern const BooleanGameRuleKey SPECTATORS_GENERATE_CHUNKS;

/// 是否禁用鞘翅移动检查
extern const BooleanGameRuleKey DISABLE_ELYTRA_MOVEMENT_CHECK;

/// 是否立即重生
extern const BooleanGameRuleKey DO_IMMEDIATE_RESPAWN;

/// 是否溺水受伤
extern const BooleanGameRuleKey DROWNING_DAMAGE;

/// 是否摔落受伤
extern const BooleanGameRuleKey FALL_DAMAGE;

/// 是否火焰受伤
extern const BooleanGameRuleKey FIRE_DAMAGE;

/// 是否冰冻受伤
extern const BooleanGameRuleKey FREEZE_DAMAGE;

/// 是否限制合成
extern const BooleanGameRuleKey DO_LIMITED_CRAFTING;

/// 是否允许玩家对玩家造成伤害
extern const BooleanGameRuleKey PVP;

// ============================================================================
// 生物相关 (Mobs)
// ============================================================================

/// 生物是否能破坏方块
extern const BooleanGameRuleKey MOB_GRIEFING;

/// 实体挤压上限
extern const IntegerGameRuleKey MAX_ENTITY_CRAMMING;

/// 是否禁用袭击
extern const BooleanGameRuleKey DISABLE_RAIDS;

/// 中立生物是否原谅死亡玩家
extern const BooleanGameRuleKey FORGIVE_DEAD_PLAYERS;

/// 通用愤怒机制
extern const BooleanGameRuleKey UNIVERSAL_ANGER;

// ============================================================================
// 生成相关 (Spawning)
// ============================================================================

/// 是否生成生物
extern const BooleanGameRuleKey DO_MOB_SPAWNING;

/// 是否生成幻翼
extern const BooleanGameRuleKey DO_INSOMNIA;

/// 是否生成巡逻队
extern const BooleanGameRuleKey DO_PATROL_SPAWNING;

/// 是否生成流浪商人
extern const BooleanGameRuleKey DO_TRADER_SPAWNING;

/// 是否生成监守者
extern const BooleanGameRuleKey DO_WARDEN_SPAWNING;

// ============================================================================
// 掉落相关 (Drops)
// ============================================================================

/// 生物是否掉落物品
extern const BooleanGameRuleKey DO_MOB_LOOT;

/// 方块是否掉落物品
extern const BooleanGameRuleKey DO_TILE_DROPS;

/// 实体是否掉落物品（矿车等）
extern const BooleanGameRuleKey DO_ENTITY_DROPS;

/// 投射物是否可以破坏方块（如箭矢破坏陶罐、紫颂花等）
/// 参考 MC Java 的 projectilesCanBreakBlocks 游戏规则，默认 true
extern const BooleanGameRuleKey PROJECTILES_CAN_BREAK_BLOCKS;

// ============================================================================
// 更新相关 (Updates)
// ============================================================================

/// 火焰是否蔓延
extern const BooleanGameRuleKey DO_FIRE_TICK;

/// 日照循环是否进行
extern const BooleanGameRuleKey DO_DAYLIGHT_CYCLE;

/// 随机刻速度（影响作物生长、火焰蔓延等）
extern const IntegerGameRuleKey RANDOM_TICK_SPEED;

/// 天气循环是否进行
extern const BooleanGameRuleKey DO_WEATHER_CYCLE;

/// 雪层最大堆积高度（0=不允许堆积，1-8=最大雪层数）
extern const IntegerGameRuleKey MAX_SNOW_ACCUMULATION_HEIGHT;

/// 火焰蔓延的玩家半径门控（-1=不限制，其余=仅当该半径（含）内存在非旁观者玩家时火焰才 tick）
/// 参考: net.minecraft.world.level.gamerules.GameRules.FIRE_SPREAD_RADIUS_AROUND_PLAYER
extern const IntegerGameRuleKey FIRE_SPREAD_RADIUS_AROUND_PLAYER;

/// 下界传送门默认权重延迟（tick）
extern const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_DEFAULT_DELAY;

/// 下界传送门创造模式权重延迟（tick）
extern const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_CREATIVE_DELAY;

// ============================================================================
// 聊天相关 (Chat)
// ============================================================================

/// 命令方块是否输出到聊天
extern const BooleanGameRuleKey COMMAND_BLOCK_OUTPUT;

/// 是否记录管理员命令
extern const BooleanGameRuleKey LOG_ADMIN_COMMANDS;

/// 是否显示死亡消息
extern const BooleanGameRuleKey SHOW_DEATH_MESSAGES;

/// 是否发送命令反馈
extern const BooleanGameRuleKey SEND_COMMAND_FEEDBACK;

/// 是否公布成就
extern const BooleanGameRuleKey ANNOUNCE_ADVANCEMENTS;

// ============================================================================
// 杂项 (Misc)
// ============================================================================

/// 是否减少调试信息
extern const BooleanGameRuleKey REDUCED_DEBUG_INFO;

/// TNT 是否允许爆炸（控制 TNT 方块点燃、TNT 实体爆炸、TNT 矿车引爆等）
extern const BooleanGameRuleKey TNT_EXPLODES;

/// 刷怪笼是否工作（刷怪蛋右键刷怪笼设置实体类型、刷怪笼生成生物）
extern const BooleanGameRuleKey SPAWNER_BLOCKS_WORK;

/// 全局声音事件（globalLevelEvent 门控）
/// 参考: net.minecraft.world.level.gamerule.GameRules.RULE_GLOBAL_SOUND_EVENTS
extern const BooleanGameRuleKey GLOBAL_SOUND_EVENTS;

/// 最大命令链长度
extern const IntegerGameRuleKey MAX_COMMAND_CHAIN_LENGTH;

/// 矿车最大速度（默认8，实际速度 = 规则值 / 20.0 方块/刻，水中减半）
/// 范围 [1, 1000]，对应 max_minecart_speed 游戏规则
extern const IntegerGameRuleKey MAX_MINECART_SPEED;

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
     * @brief 移动构造
     */
    GameRules(GameRules&& other) noexcept;

    /**
     * @brief 拷贝赋值
     */
    GameRules& operator=(const GameRules& other);

    /**
     * @brief 移动赋值
     */
    GameRules& operator=(GameRules&& other) noexcept;

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
     * @brief 按规则名取当前值的字符串表示（脚本/命令侧统一读取入口）
     *
     * getBoolean/getInt 需编译期 GameRuleKey（BooleanGameRuleKey/IntegerGameRuleKey），
     * 仅适用已知规则。脚本侧（@minecraft/server）与命令侧仅持规则名字符串，
     * 无法构造编译期 key，故提供此按名查询入口：先查当前值 map（m_booleanRules/
     * m_integerRules），命中返回字符串表示（bool→"true"/"false"，i32→十进制串）；
     * 未命中则回退注册表默认值；规则不存在返回空串。
     *
     * @param ruleName 规则名（如 "mobGriefing"、"randomTickSpeed"）
     * @return 当前值字符串表示；规则不存在返回空串
     */
    [[nodiscard]] std::string getValueAsString(const std::string& ruleName) const;

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
     * @brief 按规则值类名从字符串设置规则值
     *
     * 对应 vanilla GameRuleType.deserialize：规则名与值的类名必须同时匹配（例如
     * "randomTickSpeed" 只接受 boolean=false 的查询、不接受 boolean=true 的设置）。
     * 整型值超出规则注册的 [min, max] 时视为无效值。
     *
     * @param ruleName 规则名称
     * @param valueClassName 值的类名（"boolean" / "integer"，见 GameRuleClassNames）
     * @param value 字符串值
     * @param server Minecraft 服务器实例（可选，用于触发变更监听器）
     * @return 是否设置成功（规则存在、类名匹配且值有效）
     */
    bool setTypedFromString(const std::string& ruleName,
        const std::string& valueClassName,
        const std::string& value,
        server::MinecraftServer* server = nullptr);

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

    /**
     * @brief 获取规则值类名
     * @param ruleName 规则名称
     * @return "boolean" / "integer"；规则不存在返回 std::nullopt
     */
    [[nodiscard]] static std::optional<std::string_view> getRuleClass(const std::string& ruleName);

    /**
     * @brief 获取规则默认值的字符串表示
     *
     * 供 /gamerule 查询反馈拼接 "(default: ...)"（vanilla getValueMessage 在当前值
     * 不同于默认值时才展示默认值）。
     *
     * @param ruleName 规则名称
     * @return 默认值字符串；规则不存在返回空串
     */
    [[nodiscard]] static std::string getDefaultValueAsString(const std::string& ruleName);

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
    void _initializeRules();

    // 从复合标签获取布尔值（辅助方法）
    static bool _getBooleanFromNbt(const nbt::tags::compound_tag& nbt, const std::string& key, bool defaultValue);

    // 从复合标签获取整数值（辅助方法）
    static i32 _getIntFromNbt(const nbt::tags::compound_tag& nbt, const std::string& key, i32 defaultValue);
};

} // namespace mc::world::gamerule
