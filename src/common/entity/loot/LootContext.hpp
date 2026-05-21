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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

// Forward declarations
class ItemStack;
class Entity;
class Player;
class DamageSource;
class ServerWorld;
class IWorld;
class BlockState;
class BlockPos;
class BlockEntity;

namespace loot {

/**
 * @brief 掉落参数
 *
 * 用于 LootContext 中标识不同类型的参数。
 * 参考: net.minecraft.loot.LootParameter
 *
 * @tparam T 参数类型
 */
template <typename T>
class LootParameter {
public:
    explicit LootParameter(const std::string& id)
        : m_id(id)
    {}

    [[nodiscard]] const std::string& getId() const { return m_id; }

    bool operator==(const LootParameter& other) const { return m_id == other.m_id; }

private:
    std::string m_id;
};

/**
 * @brief 掉落参数集合
 *
 * 定义生成掉落物所需的参数类型集合。
 * 参考: net.minecraft.loot.LootParameterSet
 */
class LootParameterSet {
public:
    /**
     * @brief 参数集合类型
     */
    enum class Type {
        Empty,   // 空集合
        Generic, // 通用
        Entity,  // 实体相关
        Block,   // 方块相关
        Chest,   // 容器
        Fishing, // 钓鱼
        Gift,    // 礼物
        Barter   // 以物易物
    };

    LootParameterSet() = default;
    explicit LootParameterSet(Type type)
        : m_type(type)
    {}

    /**
     * @brief 获取参数集合类型
     */
    [[nodiscard]] Type getType() const { return m_type; }

    /**
     * @brief 获取参数集合名称
     */
    [[nodiscard]] std::string getName() const
    {
        switch (m_type) {
            case Type::Empty:
                return "minecraft:empty";
            case Type::Generic:
                return "minecraft:generic";
            case Type::Entity:
                return "minecraft:entity";
            case Type::Block:
                return "minecraft:block";
            case Type::Fishing:
                return "minecraft:fishing";
            case Type::Chest:
                return "minecraft:chest";
            case Type::Gift:
                return "minecraft:gift";
            case Type::Barter:
                return "minecraft:barter";
            default:
                return "minecraft:generic";
        }
    }

    /**
     * @brief 添加必需参数
     */
    template <typename T>
    void addRequired(const LootParameter<T>& param)
    {
        m_requiredParams.push_back(param.getId());
    }

    /**
     * @brief 添加可选参数
     */
    template <typename T>
    void addOptional(const LootParameter<T>& param)
    {
        m_optionalParams.push_back(param.getId());
    }

    /**
     * @brief 检查参数是否在集合中
     */
    [[nodiscard]] bool contains(const std::string& paramId) const;

    /**
     * @brief 验证上下文是否包含所有必需参数
     */
    [[nodiscard]] bool validate(const std::vector<std::string>& providedParams) const;

private:
    Type m_type = Type::Generic;
    std::vector<std::string> m_requiredParams;
    std::vector<std::string> m_optionalParams;
};

// 预定义掉落参数
// 注意：模板参数是值类型，使用时传入指针
namespace LootParams {
extern const LootParameter<Entity> THIS_ENTITY;         // 当前实体
extern const LootParameter<Player> KILLER_PLAYER;       // 击杀玩家
extern const LootParameter<Entity> KILLER_ENTITY;       // 击杀实体
extern const LootParameter<Entity> DIRECT_KILLER;       // 直接击杀者
extern const LootParameter<DamageSource> DAMAGE_SOURCE; // 伤害来源
extern const LootParameter<f32> LUCK;                   // 幸运值

// 方块相关参数
extern const LootParameter<BlockState> BLOCK_STATE;   // 被破坏的方块状态
extern const LootParameter<BlockPos> BLOCK_POS;       // 方块位置
extern const LootParameter<ItemStack> TOOL;           // 使用的工具
extern const LootParameter<BlockEntity> BLOCK_ENTITY; // 方块实体

// 附魔等级参数
extern const LootParameter<i32> FORTUNE_LEVEL;    // 时运附魔等级
extern const LootParameter<i32> SILK_TOUCH_LEVEL; // 精准采集附魔等级
extern const LootParameter<i32> LOOTING_MODIFIER; // 掠夺附魔等级修正

// 爆炸相关参数
extern const LootParameter<f32> EXPLOSION_RADIUS; // 爆炸半径

// 钓鱼相关参数
extern const LootParameter<bool> IS_IN_OPEN_WATER; // 是否在开放水域
} // namespace LootParams

/**
 * @brief 预定义参数集合工厂
 *
 * 提供每种掉落表类型对应的参数集合，包含正确的必需和可选参数。
 * 参考: net.minecraft.loot.LootParameterSets
 */
namespace LootParameterSets {
extern LootParameterSet empty();
extern LootParameterSet block();
extern LootParameterSet chest();
extern LootParameterSet entity();
extern LootParameterSet fishing();
extern LootParameterSet gift();
extern LootParameterSet barter();
extern LootParameterSet generic();
} // namespace LootParameterSets

/**
 * @brief 掉落上下文
 *
 * 包含生成掉落物所需的所有上下文信息。
 * 参考: net.minecraft.loot.LootContext
 */
class LootContext {
public:
    using LootTableResolver = std::function<const class LootTable*(const std::string&)>;

    LootContext(IWorld& world, math::Random& random);
    ~LootContext() = default;

    // 禁止拷贝
    LootContext(const LootContext&) = delete;
    LootContext& operator=(const LootContext&) = delete;

    // 允许移动
    LootContext(LootContext&&) = default;
    LootContext& operator=(LootContext&&) = default;

    // ========== 参数访问 ==========

    /**
     * @brief 检查是否有指定参数
     */
    template <typename T>
    [[nodiscard]] bool has(const LootParameter<T>& param) const
    {
        return m_params.find(param.getId()) != m_params.end();
    }

    /**
     * @brief 获取所有已设置的参数ID列表
     */
    [[nodiscard]] std::vector<std::string> getParamIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(m_params.size());
        for (const auto& [id, _] : m_params) {
            ids.push_back(id);
        }
        return ids;
    }

    /**
     * @brief 获取参数值
     * @tparam T 参数类型
     * @param param 参数
     * @return 参数值指针，不存在返回nullptr
     */
    template <typename T>
    [[nodiscard]] T* get(const LootParameter<T>& param) const
    {
        auto it = m_params.find(param.getId());
        if (it != m_params.end()) {
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }

    /**
     * @brief 设置参数
     *
     * 存储参数指针到内部映射。
     * 调用方需要确保参数在 LootContext 生命周期内有效。
     *
     * @tparam T 参数值的类型
     * @param param 参数标识符
     * @param value 参数值指针
     */
    template <typename T>
    void set(const LootParameter<T>& param, T* value)
    {
        m_params[param.getId()] = static_cast<void*>(value);
    }

    /**
     * @brief 设置拥有所有权的参数值
     *
     * 存储值的副本到内部容器，由 LootContext 管理生命周期。
     * 适用于简单的值类型（如 i32, f32 等）。
     *
     * @tparam T 参数值的类型
     * @param param 参数标识符
     * @param value 参数值
     */
    template <typename T>
    void setOwnedValue(const LootParameter<T>& param, T value)
    {
        auto ownedPtr = std::make_shared<T>(std::move(value));
        m_ownedValues.push_back(ownedPtr);
        m_params[param.getId()] = static_cast<void*>(ownedPtr.get());
    }

    // ========== 基本属性 ==========

    /**
     * @brief 获取世界
     */
    [[nodiscard]] IWorld& getWorld() const { return m_world; }

    /**
     * @brief 获取随机数生成器
     */
    [[nodiscard]] math::Random& getRandom() const { return m_random; }

    /**
     * @brief 获取幸运值
     */
    [[nodiscard]] f32 getLuck() const { return m_luck; }

    /**
     * @brief 设置幸运值
     */
    void setLuck(f32 luck) { m_luck = luck; }

    /**
     * @brief 获取掠夺附魔等级
     */
    [[nodiscard]] i32 getLootingModifier() const { return m_lootingModifier; }

    /**
     * @brief 设置掠夺附魔等级
     */
    void setLootingModifier(i32 level) { m_lootingModifier = level; }

    // ========== 掉落表访问 ==========

    /**
     * @brief 设置掉落表解析器
     */
    void setLootTableResolver(LootTableResolver resolver) { m_lootTableResolver = std::move(resolver); }

    /**
     * @brief 获取掉落表
     */
    [[nodiscard]] const LootTable* getLootTable(const std::string& id) const;

    // ========== 循环检测 ==========

    /**
     * @brief 添加掉落表到访问栈（用于检测循环引用）
     * @return 如果已经访问过此表，返回false
     */
    bool pushLootTable(const LootTable* table);

    /**
     * @brief 从访问栈移除掉落表
     */
    void popLootTable(const LootTable* table);

    friend class LootContextBuilder;

private:
    IWorld& m_world;
    math::Random& m_random;
    f32 m_luck = 0.0f;
    i32 m_lootingModifier = 0;
    std::unordered_map<std::string, void*> m_params;
    std::vector<std::shared_ptr<void>> m_ownedValues; // 拥有所有权的值存储
    LootTableResolver m_lootTableResolver;
    std::vector<const LootTable*> m_visitedTables; // 用于检测循环引用
};

/**
 * @brief 掉落上下文构建器
 *
 * 参考: net.minecraft.loot.LootContext.Builder
 */
class LootContextBuilder {
public:
    explicit LootContextBuilder(IWorld& world);

    /**
     * @brief 设置随机数生成器
     */
    LootContextBuilder& withRandom(math::Random& random);

    /**
     * @brief 设置随机种子
     */
    LootContextBuilder& withSeed(u64 seed);

    /**
     * @brief 设置幸运值
     */
    LootContextBuilder& withLuck(f32 luck);

    /**
     * @brief 设置掠夺附魔等级
     */
    LootContextBuilder& withLootingModifier(i32 level);

    /**
     * @brief 设置参数
     */
    template <typename T>
    LootContextBuilder& withParameter(const LootParameter<T>& param, T* value)
    {
        m_params[param.getId()] = static_cast<void*>(value);
        return *this;
    }

    /**
     * @brief 设置可空参数
     */
    template <typename T>
    LootContextBuilder& withNullableParameter(const LootParameter<T>& param, T* value)
    {
        if (value) {
            m_params[param.getId()] = static_cast<void*>(value);
        } else {
            m_params.erase(param.getId());
        }
        return *this;
    }

    /**
     * @brief 设置拥有所有权的参数值
     *
     * 存储值的副本到构建器，在构建时传递给 LootContext。
     * 适用于简单的值类型（如 i32, f32 等）。
     *
     * @tparam T 参数值的类型
     * @param param 参数标识符
     * @param value 参数值
     */
    template <typename T>
    LootContextBuilder& withOwnedValue(const LootParameter<T>& param, T value)
    {
        auto ownedPtr = std::make_shared<T>(std::move(value));
        m_ownedValues.push_back(ownedPtr);
        m_params[param.getId()] = static_cast<void*>(ownedPtr.get());
        return *this;
    }

    /**
     * @brief 设置掉落表解析器
     */
    LootContextBuilder& withLootTableResolver(LootContext::LootTableResolver resolver)
    {
        m_lootTableResolver = std::move(resolver);
        return *this;
    }

    /**
     * @brief 构建掉落上下文
     */
    [[nodiscard]] std::unique_ptr<LootContext> build(const LootParameterSet& paramSet = LootParameterSet());

private:
    IWorld& m_world;
    math::Random* m_random = nullptr;
    u64 m_seed = 0;
    bool m_hasSeed = false;
    f32 m_luck = 0.0f;
    i32 m_lootingModifier = 0;
    std::unordered_map<std::string, void*> m_params;
    std::vector<std::shared_ptr<void>> m_ownedValues; // 拥有所有权的值存储
    LootContext::LootTableResolver m_lootTableResolver;
};

} // namespace loot
} // namespace mc
