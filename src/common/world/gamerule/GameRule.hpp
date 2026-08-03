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
 * @file GameRule.hpp
 * @brief 游戏规则类型定义
 *
 * 游戏规则是 Minecraft 中控制游戏行为的配置项，如：
 * - mobGriefing: 生物是否能破坏方块
 * - naturalRegeneration: 玩家是否自然恢复生命
 * - doDaylightCycle: 日照循环是否进行
 * 等
 *
 * 设计模式：
 * - RuleKey: 规则的唯一标识符
 * - RuleType: 规则的类型定义（包含默认值、变更监听器）
 * - RuleValue: 规则的运行时值
 */

#pragma once

#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <variant>

// 前向声明
namespace mc::server {
class MinecraftServer;
}

namespace mc::world::gamerule {

/**
 * @brief 游戏规则分类
 *
 * 用于在 UI 中分组显示游戏规则
 */
enum class GameRuleCategory : u8 {
    Player,   ///< 玩家相关（keepInventory, naturalRegeneration 等）
    Mobs,     ///< 生物相关（mobGriefing, maxEntityCramming 等）
    Spawning, ///< 生成相关（doMobSpawning, doInsomnia 等）
    Drops,    ///< 掉落相关（doMobLoot, doTileDrops 等）
    Updates,  ///< 更新相关（doFireTick, randomTickSpeed 等）
    Chat,     ///< 聊天相关（commandBlockOutput, logAdminCommands 等）
    Misc      ///< 杂项（reducedDebugInfo, maxCommandChainLength 等）
};

/**
 * @brief 游戏规则变更监听器类型
 *
 * 当规则值变更时调用，可以触发服务器端操作（如同步给客户端）
 *
 * @param server Minecraft 服务器实例
 * @param newValue 新的规则值
 */
template <typename T>
using GameRuleChangeListener = std::function<void(server::MinecraftServer* server, T newValue)>;

/**
 * @brief 游戏规则类型枚举
 */
enum class GameRuleValueType : u8 {
    Boolean, ///< 布尔类型
    Integer  ///< 整数类型
};

// 前向声明
template <typename T>
class GameRuleValue;

/**
 * @brief 游戏规则键
 *
 * 规则的唯一标识符，包含名称和分类。
 * 模板参数 T 表示规则值类型（bool 或 i32）。
 *
 * @tparam T 规则值类型
 */
template <typename T>
class GameRuleKey {
public:
    /**
     * @brief 构造规则键
     * @param name 规则名称（如 "mobGriefing"）
     * @param category 规则分类
     */
    GameRuleKey(std::string name, GameRuleCategory category)
        : m_name(std::move(name))
        , m_category(category)
    {}

    /**
     * @brief 获取规则名称
     */
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

    /**
     * @brief 获取规则分类
     */
    [[nodiscard]] GameRuleCategory getCategory() const noexcept { return m_category; }

    /**
     * @brief 获取本地化键
     * @return 如 "gamerule.mobGriefing"
     */
    [[nodiscard]] std::string getTranslationKey() const { return "gamerule." + m_name; }

    /**
     * @brief 相等比较
     */
    bool operator==(const GameRuleKey& other) const noexcept { return m_name == other.m_name; }

    /**
     * @brief 哈希值
     */
    [[nodiscard]] size_t hashCode() const noexcept { return std::hash<std::string>{}(m_name); }

private:
    std::string m_name;
    GameRuleCategory m_category;
};

/**
 * @brief 游戏规则类型定义
 *
 * 包含规则的默认值和变更监听器。
 * 用于创建 GameRuleValue 实例。
 *
 * @tparam T 规则值类型
 */
template <typename T>
class GameRuleType {
public:
    /**
     * @brief 构造规则类型
     * @param defaultValue 默认值
     * @param changeListener 变更监听器（可选）
     */
    GameRuleType(T defaultValue, GameRuleChangeListener<T> changeListener = nullptr)
        : m_defaultValue(defaultValue)
        , m_changeListener(std::move(changeListener))
    {}

    /**
     * @brief 获取默认值
     */
    [[nodiscard]] T getDefaultValue() const noexcept { return m_defaultValue; }

    /**
     * @brief 获取变更监听器
     */
    [[nodiscard]] const GameRuleChangeListener<T>& getChangeListener() const noexcept { return m_changeListener; }

    /**
     * @brief 创建规则值实例
     */
    [[nodiscard]] GameRuleValue<T> createValue() const;

private:
    T m_defaultValue;
    GameRuleChangeListener<T> m_changeListener;
};

/**
 * @brief 游戏规则值基类
 *
 * 存储规则的运行时值，支持序列化和变更通知。
 *
 * @tparam T 规则值类型
 */
template <typename T>
class GameRuleValue {
public:
    /**
     * @brief 默认构造函数
     *
     * 用于 unordered_map 等容器要求
     */
    GameRuleValue()
        : m_type(nullptr)
        , m_defaultValue{}
        , m_value{}
    {}

    /**
     * @brief 构造规则值
     * @param type 规则类型定义
     */
    explicit GameRuleValue(const GameRuleType<T>& type)
        : m_type(&type)
        , m_defaultValue(type.getDefaultValue())
        , m_value(type.getDefaultValue())
    {}

    /**
     * @brief 获取当前值
     */
    [[nodiscard]] T get() const noexcept { return m_value; }

    /**
     * @brief 设置新值
     * @param value 新值
     * @param server Minecraft 服务器实例（用于触发变更监听器）
     */
    void set(T value, server::MinecraftServer* server = nullptr);

    /**
     * @brief 重置为默认值
     * @param server Minecraft 服务器实例
     */
    void reset(server::MinecraftServer* server = nullptr)
    {
        if (m_type) {
            set(m_defaultValue, server);
        }
    }

    /**
     * @brief 检查是否为默认值
     */
    [[nodiscard]] bool isDefault() const noexcept { return m_value == m_defaultValue; }

    /**
     * @brief 获取字符串表示（用于序列化）
     */
    [[nodiscard]] std::string toString() const;

    /**
     * @brief 从字符串解析（用于反序列化）
     * @param value 字符串值
     * @return 是否解析成功
     */
    bool fromString(const std::string& value);

    /**
     * @brief 获取规则类型
     */
    [[nodiscard]] const GameRuleType<T>* getType() const noexcept { return m_type; }

    /**
     * @brief 复制规则值（创建新实例）
     */
    [[nodiscard]] GameRuleValue<T> clone() const noexcept
    {
        GameRuleValue<T> copy;
        copy.m_type = m_type;
        copy.m_defaultValue = m_defaultValue;
        copy.m_value = m_value;
        return copy;
    }

private:
    const GameRuleType<T>* m_type = nullptr;
    T m_defaultValue{}; ///< 默认值副本
    T m_value{};        ///< 当前值
};

// 布尔类型特化
template <>
[[nodiscard]] std::string GameRuleValue<bool>::toString() const;

template <>
bool GameRuleValue<bool>::fromString(const std::string& value);

// 整数类型特化
template <>
[[nodiscard]] std::string GameRuleValue<i32>::toString() const;

template <>
bool GameRuleValue<i32>::fromString(const std::string& value);

// ============================================================================
// 模板实现
// ============================================================================

template <typename T>
GameRuleValue<T> GameRuleType<T>::createValue() const
{
    return GameRuleValue<T>(*this);
}

template <typename T>
void GameRuleValue<T>::set(T value, server::MinecraftServer* server)
{
    m_value = value;
    // 触发变更监听器
    if (m_type && m_type->getChangeListener() && server) {
        m_type->getChangeListener()(server, value);
    }
}

// 类型别名
using BooleanGameRuleKey = GameRuleKey<bool>;
using IntegerGameRuleKey = GameRuleKey<i32>;
using BooleanGameRuleType = GameRuleType<bool>;
using IntegerGameRuleType = GameRuleType<i32>;
using BooleanGameRuleValue = GameRuleValue<bool>;
using IntegerGameRuleValue = GameRuleValue<i32>;

} // namespace mc::world::gamerule

// ============================================================================
// 哈希特化
// ============================================================================

namespace std {

template <typename T>
struct hash<mc::world::gamerule::GameRuleKey<T>> {
    size_t operator()(const mc::world::gamerule::GameRuleKey<T>& key) const { return key.hashCode(); }
};

} // namespace std
