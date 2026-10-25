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
 * @file GameRules.cpp
 * @brief 游戏规则容器实现
 *
 * 包含所有游戏规则的定义和默认值。
 */

#include "GameRules.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/gamerule/GameRule.hpp"
#include <algorithm>
#include <charconv>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::gamerule {

// ============================================================================
// 游戏规则类型定义（静态）
// ============================================================================

namespace {

// 布尔规则类型定义（无监听器）
BooleanGameRuleType createBooleanType(bool defaultValue)
{
    // 布尔值只有 false/true 两种取值，范围恒为其全域
    return BooleanGameRuleType(defaultValue, false, true);
}

// 整数规则类型定义（无监听器）
IntegerGameRuleType createIntegerType(i32 defaultValue, i32 minValue, i32 maxValue)
{
    return IntegerGameRuleType(defaultValue, minValue, maxValue);
}

// 判断字符串是否为十进制整数：可选单个 '-' 符号后跟数字（允许前导零，与 vanilla readInt 一致）。
bool _isCanonicalDecimalInteger(std::string_view value)
{
    if (value.empty()) {
        return false;
    }

    const bool negative = value.front() == '-';
    const std::string_view digits = negative ? value.substr(1) : value;
    if (digits.empty()) {
        return false;
    }

    return std::all_of(digits.begin(), digits.end(), [](char c) {
        return c >= '0' && c <= '9';
    });
}

// 规则类型注册表
struct RuleRegistry {
    // 布尔规则类型
    std::unordered_map<std::string, BooleanGameRuleType> booleanTypes;
    // 整数规则类型
    std::unordered_map<std::string, IntegerGameRuleType> integerTypes;
    // 规则分类
    std::unordered_map<std::string, GameRuleCategory> categories;
    // 规则名称列表（按注册顺序）
    std::vector<std::string> ruleNames;

    RuleRegistry()
    {
        registerBooleanRules();
        registerIntegerRules();
    }

    void registerBooleanRules()
    {
        // 规则名、默认值与原版 1.21.11 的 GameRules 一致（规则名是带命名空间的注册表项
        // `minecraft:<name>`，命令树里以短名出现，/gamerule 两种写法都接受）。
        // 玩家相关
        registerBoolean("advance_time", GameRuleCategory::Updates, true);
        registerBoolean("advance_weather", GameRuleCategory::Updates, true);
        registerBoolean("allow_entering_nether_using_portals", GameRuleCategory::Player, true);
        registerBoolean("block_drops", GameRuleCategory::Drops, true);
        registerBoolean("block_explosion_drop_decay", GameRuleCategory::Drops, true);
        registerBoolean("command_blocks_work", GameRuleCategory::Misc, true);
        registerBoolean("command_block_output", GameRuleCategory::Chat, true);
        registerBoolean("drowning_damage", GameRuleCategory::Player, true);
        registerBoolean("elytra_movement_check", GameRuleCategory::Player, true);
        registerBoolean("ender_pearls_vanish_on_death", GameRuleCategory::Player, true);
        registerBoolean("entity_drops", GameRuleCategory::Drops, true);
        registerBoolean("fall_damage", GameRuleCategory::Player, true);
        registerBoolean("fire_damage", GameRuleCategory::Player, true);
        registerBoolean("forgive_dead_players", GameRuleCategory::Mobs, true);
        registerBoolean("freeze_damage", GameRuleCategory::Player, true);
        registerBoolean("global_sound_events", GameRuleCategory::Misc, true);
        registerBoolean("immediate_respawn", GameRuleCategory::Player, false);
        registerBoolean("keep_inventory", GameRuleCategory::Player, false);
        registerBoolean("lava_source_conversion", GameRuleCategory::Updates, true);
        registerBoolean("limited_crafting", GameRuleCategory::Player, false);
        registerBoolean("locator_bar", GameRuleCategory::Player, true);
        registerBoolean("log_admin_commands", GameRuleCategory::Chat, true);
        registerBoolean("mob_drops", GameRuleCategory::Drops, true);
        registerBoolean("mob_explosion_drop_decay", GameRuleCategory::Mobs, true);
        registerBoolean("mob_griefing", GameRuleCategory::Mobs, true);
        registerBoolean("natural_health_regeneration", GameRuleCategory::Player, true);
        registerBoolean("player_movement_check", GameRuleCategory::Player, true);
        registerBoolean("projectiles_can_break_blocks", GameRuleCategory::Drops, true);
        registerBoolean("pvp", GameRuleCategory::Player, true);
        registerBoolean("raids", GameRuleCategory::Mobs, true);
        registerBoolean("reduced_debug_info", GameRuleCategory::Misc, false);
        registerBoolean("send_command_feedback", GameRuleCategory::Chat, true);
        registerBoolean("show_advancement_messages", GameRuleCategory::Chat, true);
        registerBoolean("show_death_messages", GameRuleCategory::Chat, true);
        registerBoolean("spawner_blocks_work", GameRuleCategory::Misc, true);
        registerBoolean("spawn_mobs", GameRuleCategory::Spawning, true);
        registerBoolean("spawn_monsters", GameRuleCategory::Spawning, true);
        registerBoolean("spawn_patrols", GameRuleCategory::Spawning, true);
        registerBoolean("spawn_phantoms", GameRuleCategory::Spawning, true);
        registerBoolean("spawn_wandering_traders", GameRuleCategory::Spawning, true);
        registerBoolean("spawn_wardens", GameRuleCategory::Spawning, true);
        registerBoolean("spectators_generate_chunks", GameRuleCategory::Player, true);
        registerBoolean("spread_vines", GameRuleCategory::Updates, true);
        registerBoolean("tnt_explodes", GameRuleCategory::Misc, true);
        registerBoolean("tnt_explosion_drop_decay", GameRuleCategory::Misc, false);
        registerBoolean("universal_anger", GameRuleCategory::Mobs, false);
        registerBoolean("water_source_conversion", GameRuleCategory::Updates, true);
    }

    // 默认值与取值范围取自 vanilla GameRules：未给出上界的规则上界为 INT_MAX。
    // 整型规则必须只在此处注册——m_booleanRules 与 m_integerRules 分表存储，
    // getInt/setInt 只查整数表，误注册到布尔表会让该规则静默失效。
    void registerIntegerRules()
    {
        // 玩家相关
        registerInteger("respawn_radius", GameRuleCategory::Player, 10, 0, INT32_MAX);
        registerInteger("players_nether_portal_default_delay", GameRuleCategory::Player, 80, 0, INT32_MAX);
        registerInteger("players_nether_portal_creative_delay", GameRuleCategory::Player, 0, 0, INT32_MAX);
        registerInteger("players_sleeping_percentage", GameRuleCategory::Player, 100, 0, 100);

        // 生物相关
        registerInteger("max_entity_cramming", GameRuleCategory::Mobs, 24, 0, INT32_MAX);

        // 更新相关
        registerInteger("random_tick_speed", GameRuleCategory::Updates, 3, 0, INT32_MAX);
        registerInteger("max_snow_accumulation_height", GameRuleCategory::Updates, 1, 0, 8);
        registerInteger("fire_spread_radius_around_player", GameRuleCategory::Updates, 128, -1, INT32_MAX);

        // 命令相关
        registerInteger("max_block_modifications", GameRuleCategory::Misc, 32768, 1, INT32_MAX);
        registerInteger("max_command_forks", GameRuleCategory::Misc, 65536, 0, INT32_MAX);
        registerInteger("max_command_sequence_length", GameRuleCategory::Misc, 65536, 0, INT32_MAX);

        // 杂项
        registerInteger("max_minecart_speed", GameRuleCategory::Misc, 8, 1, 1000);
    }

    void registerBoolean(const std::string& name, GameRuleCategory category, bool defaultValue)
    {
        booleanTypes.emplace(name, createBooleanType(defaultValue));
        categories.emplace(name, category);
        ruleNames.push_back(name);
    }

    void registerInteger(
        const std::string& name, GameRuleCategory category, i32 defaultValue, i32 minValue, i32 maxValue)
    {
        integerTypes.emplace(name, createIntegerType(defaultValue, minValue, maxValue));
        categories.emplace(name, category);
        ruleNames.push_back(name);
    }
};

// 获取全局注册表（静态初始化）
RuleRegistry& getRegistry()
{
    static RuleRegistry registry;
    return registry;
}

} // namespace

// ============================================================================
// 游戏规则键定义
// ============================================================================

namespace GameRuleKeys {

// 玩家相关
const BooleanGameRuleKey KEEP_INVENTORY("keep_inventory", GameRuleCategory::Player);
const BooleanGameRuleKey LOCATOR_BAR("locator_bar", GameRuleCategory::Player);
const BooleanGameRuleKey NATURAL_HEALTH_REGENERATION("natural_health_regeneration", GameRuleCategory::Player);
const IntegerGameRuleKey RESPAWN_RADIUS("respawn_radius", GameRuleCategory::Player);
const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_DEFAULT_DELAY(
    "players_nether_portal_default_delay", GameRuleCategory::Player);
const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_CREATIVE_DELAY(
    "players_nether_portal_creative_delay", GameRuleCategory::Player);
const IntegerGameRuleKey PLAYERS_SLEEPING_PERCENTAGE("players_sleeping_percentage", GameRuleCategory::Player);
const BooleanGameRuleKey SPECTATORS_GENERATE_CHUNKS("spectators_generate_chunks", GameRuleCategory::Player);
const BooleanGameRuleKey ELYTRA_MOVEMENT_CHECK("elytra_movement_check", GameRuleCategory::Player);
const BooleanGameRuleKey IMMEDIATE_RESPAWN("immediate_respawn", GameRuleCategory::Player);
const BooleanGameRuleKey DROWNING_DAMAGE("drowning_damage", GameRuleCategory::Player);
const BooleanGameRuleKey FALL_DAMAGE("fall_damage", GameRuleCategory::Player);
const BooleanGameRuleKey FIRE_DAMAGE("fire_damage", GameRuleCategory::Player);
const BooleanGameRuleKey FREEZE_DAMAGE("freeze_damage", GameRuleCategory::Player);
const BooleanGameRuleKey LIMITED_CRAFTING("limited_crafting", GameRuleCategory::Player);
const BooleanGameRuleKey ALLOW_ENTERING_NETHER_USING_PORTALS(
    "allow_entering_nether_using_portals", GameRuleCategory::Player);
const BooleanGameRuleKey ENDER_PEARLS_VANISH_ON_DEATH("ender_pearls_vanish_on_death", GameRuleCategory::Player);
const BooleanGameRuleKey PLAYER_MOVEMENT_CHECK("player_movement_check", GameRuleCategory::Player);
const BooleanGameRuleKey PVP("pvp", GameRuleCategory::Player);

// 生物相关
const BooleanGameRuleKey MOB_GRIEFING("mob_griefing", GameRuleCategory::Mobs);
const BooleanGameRuleKey MOB_EXPLOSION_DROP_DECAY("mob_explosion_drop_decay", GameRuleCategory::Mobs);
const IntegerGameRuleKey MAX_ENTITY_CRAMMING("max_entity_cramming", GameRuleCategory::Mobs);
const BooleanGameRuleKey RAIDS("raids", GameRuleCategory::Mobs);
const BooleanGameRuleKey FORGIVE_DEAD_PLAYERS("forgive_dead_players", GameRuleCategory::Mobs);
const BooleanGameRuleKey UNIVERSAL_ANGER("universal_anger", GameRuleCategory::Mobs);

// 生成相关
const BooleanGameRuleKey SPAWN_MOBS("spawn_mobs", GameRuleCategory::Spawning);
const BooleanGameRuleKey SPAWN_MONSTERS("spawn_monsters", GameRuleCategory::Spawning);
const BooleanGameRuleKey SPAWN_PHANTOMS("spawn_phantoms", GameRuleCategory::Spawning);
const BooleanGameRuleKey SPAWN_PATROLS("spawn_patrols", GameRuleCategory::Spawning);
const BooleanGameRuleKey SPAWN_WANDERING_TRADERS("spawn_wandering_traders", GameRuleCategory::Spawning);
const BooleanGameRuleKey SPAWN_WARDENS("spawn_wardens", GameRuleCategory::Spawning);

// 掉落相关
const BooleanGameRuleKey MOB_DROPS("mob_drops", GameRuleCategory::Drops);
const BooleanGameRuleKey BLOCK_DROPS("block_drops", GameRuleCategory::Drops);
const BooleanGameRuleKey ENTITY_DROPS("entity_drops", GameRuleCategory::Drops);
const BooleanGameRuleKey BLOCK_EXPLOSION_DROP_DECAY("block_explosion_drop_decay", GameRuleCategory::Drops);
const BooleanGameRuleKey PROJECTILES_CAN_BREAK_BLOCKS("projectiles_can_break_blocks", GameRuleCategory::Drops);

// 更新相关
const BooleanGameRuleKey ADVANCE_TIME("advance_time", GameRuleCategory::Updates);
const BooleanGameRuleKey ADVANCE_WEATHER("advance_weather", GameRuleCategory::Updates);
const IntegerGameRuleKey RANDOM_TICK_SPEED("random_tick_speed", GameRuleCategory::Updates);
const IntegerGameRuleKey MAX_SNOW_ACCUMULATION_HEIGHT("max_snow_accumulation_height", GameRuleCategory::Updates);
const IntegerGameRuleKey FIRE_SPREAD_RADIUS_AROUND_PLAYER(
    "fire_spread_radius_around_player", GameRuleCategory::Updates);
const BooleanGameRuleKey LAVA_SOURCE_CONVERSION("lava_source_conversion", GameRuleCategory::Updates);
const BooleanGameRuleKey WATER_SOURCE_CONVERSION("water_source_conversion", GameRuleCategory::Updates);
const BooleanGameRuleKey SPREAD_VINES("spread_vines", GameRuleCategory::Updates);

// 聊天相关
const BooleanGameRuleKey COMMAND_BLOCK_OUTPUT("command_block_output", GameRuleCategory::Chat);
const BooleanGameRuleKey LOG_ADMIN_COMMANDS("log_admin_commands", GameRuleCategory::Chat);
const BooleanGameRuleKey SHOW_DEATH_MESSAGES("show_death_messages", GameRuleCategory::Chat);
const BooleanGameRuleKey SEND_COMMAND_FEEDBACK("send_command_feedback", GameRuleCategory::Chat);
const BooleanGameRuleKey SHOW_ADVANCEMENT_MESSAGES("show_advancement_messages", GameRuleCategory::Chat);

// 杂项
const BooleanGameRuleKey REDUCED_DEBUG_INFO("reduced_debug_info", GameRuleCategory::Misc);
const BooleanGameRuleKey COMMAND_BLOCKS_WORK("command_blocks_work", GameRuleCategory::Misc);
const BooleanGameRuleKey TNT_EXPLODES("tnt_explodes", GameRuleCategory::Misc);
const BooleanGameRuleKey TNT_EXPLOSION_DROP_DECAY("tnt_explosion_drop_decay", GameRuleCategory::Misc);
const BooleanGameRuleKey SPAWNER_BLOCKS_WORK("spawner_blocks_work", GameRuleCategory::Misc);
const BooleanGameRuleKey GLOBAL_SOUND_EVENTS("global_sound_events", GameRuleCategory::Misc);
const IntegerGameRuleKey MAX_COMMAND_SEQUENCE_LENGTH("max_command_sequence_length", GameRuleCategory::Misc);
const IntegerGameRuleKey MAX_COMMAND_FORKS("max_command_forks", GameRuleCategory::Misc);
const IntegerGameRuleKey MAX_BLOCK_MODIFICATIONS("max_block_modifications", GameRuleCategory::Misc);
const IntegerGameRuleKey MAX_MINECART_SPEED("max_minecart_speed", GameRuleCategory::Misc);

} // namespace GameRuleKeys

// ============================================================================
// GameRules 实现
// ============================================================================

GameRules::GameRules()
{
    _initializeRules();
}

GameRules::GameRules(const nbt::tags::compound_tag& nbt)
{
    _initializeRules();
    read(nbt);
}

GameRules::GameRules(const GameRules& other)
{
    _initializeRules();
    // 复制值
    for (const auto& [name, value] : other.m_booleanRules) {
        m_booleanRules[name] = value.clone();
    }
    for (const auto& [name, value] : other.m_integerRules) {
        m_integerRules[name] = value.clone();
    }
}

GameRules::GameRules(GameRules&& other) noexcept
    : m_booleanRules(std::move(other.m_booleanRules))
    , m_integerRules(std::move(other.m_integerRules))
{}

GameRules& GameRules::operator=(const GameRules& other)
{
    if (this != &other) {
        // 复制值
        for (const auto& [name, value] : other.m_booleanRules) {
            m_booleanRules[name] = value.clone();
        }
        for (const auto& [name, value] : other.m_integerRules) {
            m_integerRules[name] = value.clone();
        }
    }
    return *this;
}

GameRules& GameRules::operator=(GameRules&& other) noexcept
{
    if (this != &other) {
        m_booleanRules = std::move(other.m_booleanRules);
        m_integerRules = std::move(other.m_integerRules);
    }
    return *this;
}

void GameRules::_initializeRules()
{
    const auto& registry = getRegistry();

    // 初始化布尔规则
    for (const auto& [name, type] : registry.booleanTypes) {
        m_booleanRules.emplace(name, type.createValue());
    }

    // 初始化整数规则
    for (const auto& [name, type] : registry.integerTypes) {
        m_integerRules.emplace(name, type.createValue());
    }
}

// ============================================================================
// 规则值获取
// ============================================================================

bool GameRules::getBoolean(const BooleanGameRuleKey& key) const
{
    auto it = m_booleanRules.find(key.getName());
    if (it != m_booleanRules.end()) {
        return it->second.get();
    }
    // 返回注册表中的默认值
    const auto& registry = getRegistry();
    auto typeIt = registry.booleanTypes.find(key.getName());
    if (typeIt != registry.booleanTypes.end()) {
        return typeIt->second.getDefaultValue();
    }
    return true; // 默认值
}

i32 GameRules::getInt(const IntegerGameRuleKey& key) const
{
    auto it = m_integerRules.find(key.getName());
    if (it != m_integerRules.end()) {
        return it->second.get();
    }
    // 返回注册表中的默认值
    const auto& registry = getRegistry();
    auto typeIt = registry.integerTypes.find(key.getName());
    if (typeIt != registry.integerTypes.end()) {
        return typeIt->second.getDefaultValue();
    }
    return 0; // 默认值
}

const BooleanGameRuleValue& GameRules::getBooleanValue(const BooleanGameRuleKey& key) const
{
    auto it = m_booleanRules.find(key.getName());
    MC_ASSERT_RELEASE(it != m_booleanRules.end());
    return it->second;
}

BooleanGameRuleValue& GameRules::getBooleanValue(const BooleanGameRuleKey& key)
{
    auto it = m_booleanRules.find(key.getName());
    MC_ASSERT_RELEASE(it != m_booleanRules.end());
    return it->second;
}

const IntegerGameRuleValue& GameRules::getIntegerValue(const IntegerGameRuleKey& key) const
{
    auto it = m_integerRules.find(key.getName());
    MC_ASSERT_RELEASE(it != m_integerRules.end());
    return it->second;
}

IntegerGameRuleValue& GameRules::getIntegerValue(const IntegerGameRuleKey& key)
{
    auto it = m_integerRules.find(key.getName());
    MC_ASSERT_RELEASE(it != m_integerRules.end());
    return it->second;
}

// ============================================================================
// 规则值设置
// ============================================================================

void GameRules::setBoolean(const BooleanGameRuleKey& key, bool value, server::MinecraftServer* server)
{
    auto it = m_booleanRules.find(key.getName());
    if (it != m_booleanRules.end()) {
        it->second.set(value, server);
    }
}

void GameRules::setInt(const IntegerGameRuleKey& key, i32 value, server::MinecraftServer* server)
{
    auto it = m_integerRules.find(key.getName());
    if (it != m_integerRules.end()) {
        it->second.set(value, server);
    }
}

bool GameRules::setTypedFromString(const std::string& ruleName,
    const std::string& valueClassName,
    const std::string& value,
    server::MinecraftServer* server)
{
    const auto& registry = getRegistry();

    // 布尔规则：类名必须为 boolean
    if (registry.booleanTypes.count(ruleName) > 0) {
        const auto valueIt = m_booleanRules.find(ruleName);
        MC_ASSERT_RELEASE(valueIt != m_booleanRules.end());

        if (valueClassName != GameRuleClassNames::BOOLEAN) {
            return false;
        }

        // 在副本上解析，解析失败不得污染既有值（GameRuleValue::fromString 失败时会把值置 false/0）
        GameRuleValue<bool> candidate = valueIt->second.clone();
        if (!candidate.fromString(value)) {
            return false;
        }
        valueIt->second.set(candidate.get(), server);
        return true;
    }

    // 整型规则：类名必须为 integer，且解析结果落在注册的 [min, max] 内
    if (const auto typeIt = registry.integerTypes.find(ruleName); typeIt != registry.integerTypes.end()) {
        const auto& type = typeIt->second;
        const auto valueIt = m_integerRules.find(ruleName);
        MC_ASSERT_RELEASE(valueIt != m_integerRules.end());

        if (valueClassName != GameRuleClassNames::INTEGER) {
            return false;
        }

        // 整型规则值必须是完整的十进制整数。"1.5"、"3abc" 与超出 i32 的数值都视为无效值，
        // 不能像 std::stoi 那样静默截断。vanilla 通过 readInt() 复用整数读取器，同样不接受
        // 前导 '+'、前导零与尾随空白。
        if (!_isCanonicalDecimalInteger(value)) {
            return false;
        }
        i32 parsedValue = 0;
        const auto [end, parseError] = std::from_chars(value.data(), value.data() + value.size(), parsedValue);
        if (parseError != std::errc{} || end != value.data() + value.size() || !type.isValueInRange(parsedValue)) {
            return false;
        }
        m_integerRules.at(ruleName).set(parsedValue, server);
        return true;
    }

    return false;
}

std::string GameRules::getDefaultValueAsString(const std::string& ruleName)
{
    const auto& registry = getRegistry();
    if (const auto boolIt = registry.booleanTypes.find(ruleName); boolIt != registry.booleanTypes.end()) {
        return boolIt->second.getDefaultValue() ? "true" : "false";
    }
    if (const auto intIt = registry.integerTypes.find(ruleName); intIt != registry.integerTypes.end()) {
        return std::to_string(intIt->second.getDefaultValue());
    }
    return {};
}

std::optional<std::string_view> GameRules::getRuleClass(const std::string& ruleName)
{
    const auto& registry = getRegistry();
    if (registry.booleanTypes.count(ruleName) > 0) {
        return GameRuleClassNames::BOOLEAN;
    }
    if (registry.integerTypes.count(ruleName) > 0) {
        return GameRuleClassNames::INTEGER;
    }
    return std::nullopt;
}

// ============================================================================
// 序列化
// ============================================================================

std::unique_ptr<nbt::tags::compound_tag> GameRules::write() const
{
    auto nbt = std::make_unique<nbt::tags::compound_tag>();

    // 写入布尔规则
    for (const auto& [name, value] : m_booleanRules) {
        nbt->put(name, value.toString());
    }

    // 写入整数规则
    for (const auto& [name, value] : m_integerRules) {
        nbt->put(name, value.toString());
    }

    return nbt;
}

void GameRules::read(const nbt::tags::compound_tag& nbt)
{
    // 读取布尔规则
    for (auto& [name, value] : m_booleanRules) {
        auto it = nbt.value.find(name);
        if (it != nbt.value.end() && it->second->id() == nbt::TagId::String) {
            const auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
            value.fromString(strTag.value);
        }
    }

    // 读取整数规则
    for (auto& [name, value] : m_integerRules) {
        auto it = nbt.value.find(name);
        if (it != nbt.value.end() && it->second->id() == nbt::TagId::String) {
            const auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
            value.fromString(strTag.value);
        }
    }
}

// ============================================================================
// 规则遍历
// ============================================================================

void GameRules::visitAll(IGameRuleVisitor& visitor)
{
    const auto& registry = getRegistry();

    // 遍历布尔规则
    for (const auto& [name, type] : registry.booleanTypes) {
        BooleanGameRuleKey key(name, registry.categories.at(name));
        visitor.visitBoolean(key, type);
    }

    // 遍历整数规则
    for (const auto& [name, type] : registry.integerTypes) {
        IntegerGameRuleKey key(name, registry.categories.at(name));
        visitor.visitInteger(key, type);
    }
}

std::vector<std::string> GameRules::getRuleNames()
{
    return getRegistry().ruleNames;
}

bool GameRules::hasRule(const std::string& ruleName)
{
    const auto& registry = getRegistry();
    return registry.booleanTypes.count(ruleName) > 0 || registry.integerTypes.count(ruleName) > 0;
}

std::optional<GameRuleValueType> GameRules::getRuleType(const std::string& ruleName)
{
    const auto& registry = getRegistry();
    if (registry.booleanTypes.count(ruleName) > 0) {
        return GameRuleValueType::Boolean;
    }
    if (registry.integerTypes.count(ruleName) > 0) {
        return GameRuleValueType::Integer;
    }
    return std::nullopt;
}

std::string GameRules::getValueAsString(const std::string& ruleName) const
{
    // 按名取当前值字符串：先查当前值 map，命中取 .get()；未命中回退注册表默认值。
    const auto& registry = getRegistry();
    {
        auto it = m_booleanRules.find(ruleName);
        if (it != m_booleanRules.end()) {
            return it->second.get() ? "true" : "false";
        }
    }
    {
        auto it = m_integerRules.find(ruleName);
        if (it != m_integerRules.end()) {
            return std::to_string(it->second.get());
        }
    }
    // 回退注册表默认值（规则已注册但本实例未显式设置时）。
    {
        auto it = registry.booleanTypes.find(ruleName);
        if (it != registry.booleanTypes.end()) {
            return it->second.getDefaultValue() ? "true" : "false";
        }
    }
    {
        auto it = registry.integerTypes.find(ruleName);
        if (it != registry.integerTypes.end()) {
            return std::to_string(it->second.getDefaultValue());
        }
    }
    return {}; // 规则不存在
}

// ============================================================================
// 重置
// ============================================================================

void GameRules::resetAll()
{
    for (auto& [name, value] : m_booleanRules) {
        value.reset(nullptr);
    }
    for (auto& [name, value] : m_integerRules) {
        value.reset(nullptr);
    }
}

bool GameRules::reset(const std::string& ruleName, server::MinecraftServer* server)
{
    auto boolIt = m_booleanRules.find(ruleName);
    if (boolIt != m_booleanRules.end()) {
        boolIt->second.reset(server);
        return true;
    }

    auto intIt = m_integerRules.find(ruleName);
    if (intIt != m_integerRules.end()) {
        intIt->second.reset(server);
        return true;
    }

    return false;
}

// ============================================================================
// NBT 辅助方法
// ============================================================================

bool GameRules::_getBooleanFromNbt(const nbt::tags::compound_tag& nbt, const std::string& key, bool defaultValue)
{
    auto it = nbt.value.find(key);
    if (it != nbt.value.end()) {
        if (it->second->id() == nbt::TagId::String) {
            const auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
            if (strTag.value == "true" || strTag.value == "TRUE" || strTag.value == "1") {
                return true;
            }
            if (strTag.value == "false" || strTag.value == "FALSE" || strTag.value == "0") {
                return false;
            }
        }
    }
    return defaultValue;
}

i32 GameRules::_getIntFromNbt(const nbt::tags::compound_tag& nbt, const std::string& key, i32 defaultValue)
{
    auto it = nbt.value.find(key);
    if (it != nbt.value.end()) {
        if (it->second->id() == nbt::TagId::String) {
            const auto& strTag = dynamic_cast<const nbt::tags::string_tag&>(*it->second);
            try {
                return std::stoi(strTag.value);
            }
            catch (...) {
                return defaultValue;
            }
        }
    }
    return defaultValue;
}

} // namespace mc::world::gamerule
