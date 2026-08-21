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
    return BooleanGameRuleType(defaultValue);
}

// 整数规则类型定义（无监听器）
IntegerGameRuleType createIntegerType(i32 defaultValue)
{
    return IntegerGameRuleType(defaultValue);
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
        registerInteger("randomTickSpeed", GameRuleCategory::Updates, 3);
        registerInteger("snowAccumulationHeight", GameRuleCategory::Updates, 1);

        // 聊天相关
        registerBoolean("commandBlockOutput", GameRuleCategory::Chat, true);
        registerBoolean("logAdminCommands", GameRuleCategory::Chat, true);
        registerBoolean("showDeathMessages", GameRuleCategory::Chat, true);
        registerBoolean("sendCommandFeedback", GameRuleCategory::Chat, true);
        registerBoolean("announceAdvancements", GameRuleCategory::Chat, true);

        // 杂项
        registerBoolean("reducedDebugInfo", GameRuleCategory::Misc, false);
        registerBoolean("tntExplodes", GameRuleCategory::Misc, true);
        registerInteger("max_minecart_speed", GameRuleCategory::Misc, 8);
    }

    void registerIntegerRules()
    {
        // 玩家相关
        registerInteger("spawnRadius", GameRuleCategory::Player, 10);

        // 生物相关
        registerInteger("maxEntityCramming", GameRuleCategory::Mobs, 24);

        // 更新相关
        registerInteger("randomTickSpeed", GameRuleCategory::Updates, 3);

        // 杂项
        registerInteger("maxCommandChainLength", GameRuleCategory::Misc, 65536);
    }

    void registerBoolean(const std::string& name, GameRuleCategory category, bool defaultValue)
    {
        booleanTypes.emplace(name, createBooleanType(defaultValue));
        categories.emplace(name, category);
        ruleNames.push_back(name);
    }

    void registerInteger(const std::string& name, GameRuleCategory category, i32 defaultValue)
    {
        integerTypes.emplace(name, createIntegerType(defaultValue));
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
const BooleanGameRuleKey NATURAL_REGENERATION("naturalRegeneration", GameRuleCategory::Player);
const IntegerGameRuleKey SPAWN_RADIUS("spawnRadius", GameRuleCategory::Player);
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

// 聊天相关
const BooleanGameRuleKey COMMAND_BLOCK_OUTPUT("commandBlockOutput", GameRuleCategory::Chat);
const BooleanGameRuleKey LOG_ADMIN_COMMANDS("logAdminCommands", GameRuleCategory::Chat);
const BooleanGameRuleKey SHOW_DEATH_MESSAGES("showDeathMessages", GameRuleCategory::Chat);
const BooleanGameRuleKey SEND_COMMAND_FEEDBACK("sendCommandFeedback", GameRuleCategory::Chat);
const BooleanGameRuleKey ANNOUNCE_ADVANCEMENTS("announceAdvancements", GameRuleCategory::Chat);

// 杂项
const BooleanGameRuleKey REDUCED_DEBUG_INFO("reducedDebugInfo", GameRuleCategory::Misc);
const BooleanGameRuleKey TNT_EXPLODES("tntExplodes", GameRuleCategory::Misc);
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

bool GameRules::setFromString(const std::string& ruleName, const std::string& value, server::MinecraftServer* server)
{
    // 检查布尔规则
    auto boolIt = m_booleanRules.find(ruleName);
    if (boolIt != m_booleanRules.end()) {
        return boolIt->second.fromString(value);
    }

    // 检查整数规则
    auto intIt = m_integerRules.find(ruleName);
    if (intIt != m_integerRules.end()) {
        return intIt->second.fromString(value);
    }

    return false; // 规则不存在
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
            } else if (strTag.value == "false" || strTag.value == "FALSE" || strTag.value == "0") {
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
