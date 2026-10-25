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

// 规则名与原版 1.21.11 的注册表项一致（`minecraft:<name>` 的短名）。
// 常量标识符保留 Cubium 的命名习惯（大写下划线），只有名字字符串对齐原版。

// ============================================================================
// 玩家相关 (Player)
// ============================================================================

/// 玩家死亡后是否保留物品栏
extern const BooleanGameRuleKey KEEP_INVENTORY;

/// 是否显示定位栏
extern const BooleanGameRuleKey LOCATOR_BAR;

/// 自然生命恢复
extern const BooleanGameRuleKey NATURAL_HEALTH_REGENERATION;

/// 重生半径
extern const IntegerGameRuleKey RESPAWN_RADIUS;

/// 下界传送门默认冷却
extern const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_DEFAULT_DELAY;

/// 下界传送门创造模式冷却
extern const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_CREATIVE_DELAY;

/// 睡眠跳过夜晚所需玩家百分比
extern const IntegerGameRuleKey PLAYERS_SLEEPING_PERCENTAGE;

/// 旁观者是否加载区块
extern const BooleanGameRuleKey SPECTATORS_GENERATE_CHUNKS;

/// 鞘翅移动距离校验
extern const BooleanGameRuleKey ELYTRA_MOVEMENT_CHECK;

/// 死亡后是否立即重生
extern const BooleanGameRuleKey IMMEDIATE_RESPAWN;

/// 溺水伤害
extern const BooleanGameRuleKey DROWNING_DAMAGE;

/// 摔落伤害
extern const BooleanGameRuleKey FALL_DAMAGE;

/// 火焰伤害
extern const BooleanGameRuleKey FIRE_DAMAGE;

/// 冰冻伤害
extern const BooleanGameRuleKey FREEZE_DAMAGE;

/// 是否限制合成
extern const BooleanGameRuleKey LIMITED_CRAFTING;

/// 是否允许通过下界传送门
extern const BooleanGameRuleKey ALLOW_ENTERING_NETHER_USING_PORTALS;

/// 末影珍珠是否在死亡时消失
extern const BooleanGameRuleKey ENDER_PEARLS_VANISH_ON_DEATH;

/// 玩家移动距离校验
extern const BooleanGameRuleKey PLAYER_MOVEMENT_CHECK;

/// 玩家对战
extern const BooleanGameRuleKey PVP;

// ============================================================================
// 生物相关 (Mobs)
// ============================================================================

/// 生物是否能破坏方块
extern const BooleanGameRuleKey MOB_GRIEFING;

/// 生物爆炸是否衰减掉落
extern const BooleanGameRuleKey MOB_EXPLOSION_DROP_DECAY;

/// 实体挤压上限
extern const IntegerGameRuleKey MAX_ENTITY_CRAMMING;

/// 是否允许袭击
extern const BooleanGameRuleKey RAIDS;

/// 中立生物是否原谅死亡玩家
extern const BooleanGameRuleKey FORGIVE_DEAD_PLAYERS;

/// 通用愤怒机制
extern const BooleanGameRuleKey UNIVERSAL_ANGER;

// ============================================================================
// 生成相关 (Spawning)
// ============================================================================

/// 是否生成生物
extern const BooleanGameRuleKey SPAWN_MOBS;

/// 是否生成怪物
extern const BooleanGameRuleKey SPAWN_MONSTERS;

/// 是否生成幻翼
extern const BooleanGameRuleKey SPAWN_PHANTOMS;

/// 是否生成巡逻队
extern const BooleanGameRuleKey SPAWN_PATROLS;

/// 是否生成流浪商人
extern const BooleanGameRuleKey SPAWN_WANDERING_TRADERS;

/// 是否生成监守者
extern const BooleanGameRuleKey SPAWN_WARDENS;

// ============================================================================
// 掉落相关 (Drops)
// ============================================================================

/// 生物是否掉落物品
extern const BooleanGameRuleKey MOB_DROPS;

/// 方块是否掉落物品
extern const BooleanGameRuleKey BLOCK_DROPS;

/// 实体是否掉落物品（矿车等）
extern const BooleanGameRuleKey ENTITY_DROPS;

/// 方块爆炸是否衰减掉落
extern const BooleanGameRuleKey BLOCK_EXPLOSION_DROP_DECAY;

/// 投射物是否可以破坏方块（如箭矢破坏陶罐、紫颂花等），默认 true
extern const BooleanGameRuleKey PROJECTILES_CAN_BREAK_BLOCKS;

// ============================================================================
// 更新相关 (Updates)
// ============================================================================

/// 是否推进时间（昼夜）
extern const BooleanGameRuleKey ADVANCE_TIME;

/// 是否推进天气
extern const BooleanGameRuleKey ADVANCE_WEATHER;

/// 随机刻速度（影响作物生长、火焰蔓延等）
extern const IntegerGameRuleKey RANDOM_TICK_SPEED;

/// 积雪最大高度
extern const IntegerGameRuleKey MAX_SNOW_ACCUMULATION_HEIGHT;

/// 火焰蔓延半径（原版以 -1 表示不蔓延）
extern const IntegerGameRuleKey FIRE_SPREAD_RADIUS_AROUND_PLAYER;

/// 岩浆是否生成新岩浆源
extern const BooleanGameRuleKey LAVA_SOURCE_CONVERSION;

/// 水是否生成新水源
extern const BooleanGameRuleKey WATER_SOURCE_CONVERSION;

/// 藤蔓是否蔓延
extern const BooleanGameRuleKey SPREAD_VINES;

// ============================================================================
// 聊天相关 (Chat)
// ============================================================================

/// 命令方块是否输出
extern const BooleanGameRuleKey COMMAND_BLOCK_OUTPUT;

/// 是否记录管理员命令
extern const BooleanGameRuleKey LOG_ADMIN_COMMANDS;

/// 是否显示死亡消息
extern const BooleanGameRuleKey SHOW_DEATH_MESSAGES;

/// 是否发送命令反馈
extern const BooleanGameRuleKey SEND_COMMAND_FEEDBACK;

/// 是否显示进度消息
extern const BooleanGameRuleKey SHOW_ADVANCEMENT_MESSAGES;

// ============================================================================
// 杂项 (Misc)
// ============================================================================

/// 是否显示简化调试信息
extern const BooleanGameRuleKey REDUCED_DEBUG_INFO;

/// 命令方块是否工作
extern const BooleanGameRuleKey COMMAND_BLOCKS_WORK;

/// TNT 是否爆炸
extern const BooleanGameRuleKey TNT_EXPLODES;

/// TNT 爆炸是否衰减掉落
extern const BooleanGameRuleKey TNT_EXPLOSION_DROP_DECAY;

/// 刷怪笼是否工作
extern const BooleanGameRuleKey SPAWNER_BLOCKS_WORK;

/// 是否向所有人广播声音事件
extern const BooleanGameRuleKey GLOBAL_SOUND_EVENTS;

/// 命令序列最大长度
extern const IntegerGameRuleKey MAX_COMMAND_SEQUENCE_LENGTH;

/// 命令分支最多数量
extern const IntegerGameRuleKey MAX_COMMAND_FORKS;

/// 单条命令最多修改方块数
extern const IntegerGameRuleKey MAX_BLOCK_MODIFICATIONS;

/// 矿车最大速度
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
     * @param ruleName 规则名（如 "mob_griefing"、"random_tick_speed"）
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
     * "random_tick_speed" 只接受 boolean=false 的查询、不接受 boolean=true 的设置）。
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
     *     "mob_griefing": "true",
     *     "natural_health_regeneration": "true",
     *     "random_tick_speed": "3",
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
