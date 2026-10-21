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

    return std::all_of(digits.begin(), digits.end(), [](char c) { return c >= '0' && c <= '9'; });
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
        // 玩家相关
        registerBoolean("keepInventory", GameRuleCategory::Player, false);
        registerBoolean("locator_bar", GameRuleCategory::Player, true);
        registerBoolean("naturalRegeneration", GameRuleCategory::Player, true);
        registerBoolean("spectatorsGenerateChunks", GameRuleCategory::Player, true);
        registerBoolean("disableElytraMovementCheck", GameRuleCategory::Player, false);
        registerBoolean("doImmediateRespawn", GameRuleCategory::Player, false);
        registerBoolean("drowningDamage", GameRuleCategory::Player, true);
        registerBoolean("fallDamage", GameRuleCategory::Player, true);
        registerBoolean("fireDamage", GameRuleCategory::Player, true);
        registerBoolean("freezeDamage", GameRuleCategory::Player, true);
        registerBoolean("doLimitedCrafting", GameRuleCategory::Player, false);
        registerBoolean("pvp", GameRuleCategory::Player, true);

        // 生物相关
        registerBoolean("mobGriefing", GameRuleCategory::Mobs, true);
        registerBoolean("disableRaids", GameRuleCategory::Mobs, false);
        registerBoolean("forgiveDeadPlayers", GameRuleCategory::Mobs, true);
        registerBoolean("universalAnger", GameRuleCategory::Mobs, false);

        // 生成相关
        registerBoolean("doMobSpawning", GameRuleCategory::Spawning, true);
        registerBoolean("doInsomnia", GameRuleCategory::Spawning, true);
        registerBoolean("doPatrolSpawning", GameRuleCategory::Spawning, true);
        registerBoolean("doTraderSpawning", GameRuleCategory::Spawning, true);
        registerBoolean("doWardenSpawning", GameRuleCategory::Spawning, true);

        // 掉落相关
        registerBoolean("doMobLoot", GameRuleCategory::Drops, true);
        registerBoolean("doTileDrops", GameRuleCategory::Drops, true);
        registerBoolean("doEntityDrops", GameRuleCategory::Drops, true);
        registerBoolean("projectilesCanBreakBlocks", GameRuleCategory::Drops, true);

        // 更新相关
        registerBoolean("doFireTick", GameRuleCategory::Updates, true);
        registerBoolean("doDaylightCycle", GameRuleCategory::Updates, true);
        registerBoolean("doWeatherCycle", GameRuleCategory::Updates, true);

        // 聊天相关
        registerBoolean("commandBlockOutput", GameRuleCategory::Chat, true);
        registerBoolean("logAdminCommands", GameRuleCategory::Chat, true);
        registerBoolean("showDeathMessages", GameRuleCategory::Chat, true);
        registerBoolean("sendCommandFeedback", GameRuleCategory::Chat, true);
        registerBoolean("announceAdvancements", GameRuleCategory::Chat, true);

        // 杂项
        registerBoolean("reducedDebugInfo", GameRuleCategory::Misc, false);
        registerBoolean("commandBlocksWork", GameRuleCategory::Misc, true);
        registerBoolean("tntExplodes", GameRuleCategory::Misc, true);
        registerBoolean("spawnerBlocksWork", GameRuleCategory::Misc, true);
        registerBoolean("globalSoundEvents", GameRuleCategory::Misc, true);
    }

    // 默认值与取值范围取自 vanilla GameRules：未给出上界的规则上界为 INT_MAX。
    // 整型规则必须只在此处注册——m_booleanRules 与 m_integerRules 分表存储，
    // getInt/setInt 只查整数表，误注册到布尔表会让该规则静默失效。
    void registerIntegerRules()
    {
        // 玩家相关
        registerInteger("spawnRadius", GameRuleCategory::Player, 10, 0, INT32_MAX);
        registerInteger("playersNetherPortalDefaultDelay", GameRuleCategory::Player, 80, 0, INT32_MAX);
        registerInteger("playersNetherPortalCreativeDelay", GameRuleCategory::Player, 0, 0, INT32_MAX);

        // 生物相关
        registerInteger("maxEntityCramming", GameRuleCategory::Mobs, 24, 0, INT32_MAX);

        // 更新相关
        registerInteger("randomTickSpeed", GameRuleCategory::Updates, 3, 0, INT32_MAX);
        registerInteger("snowAccumulationHeight", GameRuleCategory::Updates, 1, 0, 8);
        registerInteger("fire_spread_radius_around_player", GameRuleCategory::Updates, 128, -1, INT32_MAX);

        // 杂项
        registerInteger("maxCommandChainLength", GameRuleCategory::Misc, 65536, 0, INT32_MAX);
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
const BooleanGameRuleKey KEEP_INVENTORY("keepInventory", GameRuleCategory::Player);
const BooleanGameRuleKey LOCATOR_BAR("locator_bar", GameRuleCategory::Player);
const BooleanGameRuleKey NATURAL_REGENERATION("naturalRegeneration", GameRuleCategory::Player);
const IntegerGameRuleKey SPAWN_RADIUS("spawnRadius", GameRuleCategory::Player);
const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_DEFAULT_DELAY(
    "playersNetherPortalDefaultDelay", GameRuleCategory::Player);
const IntegerGameRuleKey PLAYERS_NETHER_PORTAL_CREATIVE_DELAY(
    "playersNetherPortalCreativeDelay", GameRuleCategory::Player);
const BooleanGameRuleKey SPECTATORS_GENERATE_CHUNKS("spectatorsGenerateChunks", GameRuleCategory::Player);
const BooleanGameRuleKey DISABLE_ELYTRA_MOVEMENT_CHECK("disableElytraMovementCheck", GameRuleCategory::Player);
const BooleanGameRuleKey DO_IMMEDIATE_RESPAWN("doImmediateRespawn", GameRuleCategory::Player);
const BooleanGameRuleKey DROWNING_DAMAGE("drowningDamage", GameRuleCategory::Player);
const BooleanGameRuleKey FALL_DAMAGE("fallDamage", GameRuleCategory::Player);
const BooleanGameRuleKey FIRE_DAMAGE("fireDamage", GameRuleCategory::Player);
const BooleanGameRuleKey FREEZE_DAMAGE("freezeDamage", GameRuleCategory::Player);
const BooleanGameRuleKey DO_LIMITED_CRAFTING("doLimitedCrafting", GameRuleCategory::Player);
const BooleanGameRuleKey PVP("pvp", GameRuleCategory::Player);

// 生物相关
const BooleanGameRuleKey MOB_GRIEFING("mobGriefing", GameRuleCategory::Mobs);
const IntegerGameRuleKey MAX_ENTITY_CRAMMING("maxEntityCramming", GameRuleCategory::Mobs);
const BooleanGameRuleKey DISABLE_RAIDS("disableRaids", GameRuleCategory::Mobs);
const BooleanGameRuleKey FORGIVE_DEAD_PLAYERS("forgiveDeadPlayers", GameRuleCategory::Mobs);
const BooleanGameRuleKey UNIVERSAL_ANGER("universalAnger", GameRuleCategory::Mobs);

// 生成相关
const BooleanGameRuleKey DO_MOB_SPAWNING("doMobSpawning", GameRuleCategory::Spawning);
const BooleanGameRuleKey DO_INSOMNIA("doInsomnia", GameRuleCategory::Spawning);
const BooleanGameRuleKey DO_PATROL_SPAWNING("doPatrolSpawning", GameRuleCategory::Spawning);
const BooleanGameRuleKey DO_TRADER_SPAWNING("doTraderSpawning", GameRuleCategory::Spawning);
const BooleanGameRuleKey DO_WARDEN_SPAWNING("doWardenSpawning", GameRuleCategory::Spawning);

// 掉落相关
const BooleanGameRuleKey DO_MOB_LOOT("doMobLoot", GameRuleCategory::Drops);
const BooleanGameRuleKey DO_TILE_DROPS("doTileDrops", GameRuleCategory::Drops);
const BooleanGameRuleKey DO_ENTITY_DROPS("doEntityDrops", GameRuleCategory::Drops);
const BooleanGameRuleKey PROJECTILES_CAN_BREAK_BLOCKS("projectilesCanBreakBlocks", GameRuleCategory::Drops);

// 更新相关
const BooleanGameRuleKey DO_FIRE_TICK("doFireTick", GameRuleCategory::Updates);
const BooleanGameRuleKey DO_DAYLIGHT_CYCLE("doDaylightCycle", GameRuleCategory::Updates);
const IntegerGameRuleKey RANDOM_TICK_SPEED("randomTickSpeed", GameRuleCategory::Updates);
const BooleanGameRuleKey DO_WEATHER_CYCLE("doWeatherCycle", GameRuleCategory::Updates);
const IntegerGameRuleKey MAX_SNOW_ACCUMULATION_HEIGHT("snowAccumulationHeight", GameRuleCategory::Updates);
const IntegerGameRuleKey FIRE_SPREAD_RADIUS_AROUND_PLAYER(
    "fire_spread_radius_around_player", GameRuleCategory::Updates);

// 聊天相关
const BooleanGameRuleKey COMMAND_BLOCK_OUTPUT("commandBlockOutput", GameRuleCategory::Chat);
const BooleanGameRuleKey LOG_ADMIN_COMMANDS("logAdminCommands", GameRuleCategory::Chat);
const BooleanGameRuleKey SHOW_DEATH_MESSAGES("showDeathMessages", GameRuleCategory::Chat);
const BooleanGameRuleKey SEND_COMMAND_FEEDBACK("sendCommandFeedback", GameRuleCategory::Chat);
const BooleanGameRuleKey ANNOUNCE_ADVANCEMENTS("announceAdvancements", GameRuleCategory::Chat);

// 杂项
const BooleanGameRuleKey REDUCED_DEBUG_INFO("reducedDebugInfo", GameRuleCategory::Misc);
const BooleanGameRuleKey COMMAND_BLOCKS_WORK("commandBlocksWork", GameRuleCategory::Misc);
const BooleanGameRuleKey TNT_EXPLODES("tntExplodes", GameRuleCategory::Misc);
const BooleanGameRuleKey SPAWNER_BLOCKS_WORK("spawnerBlocksWork", GameRuleCategory::Misc);
const BooleanGameRuleKey GLOBAL_SOUND_EVENTS("globalSoundEvents", GameRuleCategory::Misc);
const IntegerGameRuleKey MAX_COMMAND_CHAIN_LENGTH("maxCommandChainLength", GameRuleCategory::Misc);
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
