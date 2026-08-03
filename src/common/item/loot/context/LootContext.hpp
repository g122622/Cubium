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

// Convenience header - includes all individual context headers
#include "LootParameter.hpp"
#include "LootParameterSet.hpp"
#include "LootParameterSets.hpp"
#include "LootParams.hpp"

// LootContext class definition (also requires these directly)
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {

// Forward declarations
class IWorld;

namespace loot {

// Forward declarations
class LootCondition;
class LootTable;

/**
 * @brief 掉落上下文
 *
 * 包含生成掉落物所需的所有上下文信息。
 */
class LootContext {
public:
    using LootTableResolver = std::function<const LootTable*(const std::string&)>;
    using PredicateResolver = std::function<const LootCondition*(const std::string&)>;

    LootContext(IWorld& world, math::Random& random);
    ~LootContext() = default;

    // 禁止拷贝
    LootContext(const LootContext&) = delete;
    LootContext& operator=(const LootContext&) = delete;

    // 允许移动构造，禁止移动赋值（有引用成员）
    LootContext(LootContext&&) = default;
    LootContext& operator=(LootContext&&) = delete;

    // ========== 参数访问 ==========

    /**
     * @brief 检查是否有指定参数
     */
    template <typename T>
    [[nodiscard]] bool has(const LootParameter<T>& param) const noexcept
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
    [[nodiscard]] T* get(const LootParameter<T>& param) const noexcept
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
    [[nodiscard]] IWorld& getWorld() const noexcept { return m_world; }

    /**
     * @brief 获取随机数生成器
     */
    [[nodiscard]] math::Random& getRandom() const noexcept { return m_random; }

    /**
     * @brief 获取幸运值
     */
    [[nodiscard]] f32 getLuck() const noexcept { return m_luck; }

    /**
     * @brief 设置幸运值
     */
    void setLuck(f32 luck) noexcept { m_luck = luck; }

    /**
     * @brief 获取掠夺附魔等级
     */
    [[nodiscard]] i32 getLootingModifier() const noexcept { return m_lootingModifier; }

    /**
     * @brief 设置掠夺附魔等级
     */
    void setLootingModifier(i32 level) noexcept { m_lootingModifier = level; }

    // ========== 谓词访问 ==========

    /**
     * @brief 设置谓词解析器
     *
     * 用于 ReferenceCondition 查找命名谓词。
     *
     * @param resolver 谓词解析器函数
     */
    void setPredicateResolver(PredicateResolver resolver) noexcept { m_predicateResolver = std::move(resolver); }

    /**
     * @brief 获取命名谓词
     *
     * @param id 谓词ID
     * @return 谓词条件指针，不存在返回nullptr
     */
    [[nodiscard]] const LootCondition* getPredicate(const std::string& id) const noexcept;

    // ========== 掉落表访问 ==========

    /**
     * @brief 设置掉落表解析器
     */
    void setLootTableResolver(LootTableResolver resolver) noexcept { m_lootTableResolver = std::move(resolver); }

    /**
     * @brief 获取掉落表
     */
    [[nodiscard]] const LootTable* getLootTable(const std::string& id) const noexcept;

    // ========== 循环检测 ==========

    /**
     * @brief 添加掉落表到访问栈（用于检测循环引用）
     * @return 如果已经访问过此表，返回false
     */
    bool pushLootTable(const LootTable* table) noexcept;

    /**
     * @brief 从访问栈移除掉落表
     */
    void popLootTable(const LootTable* table) noexcept;

    /**
     * @brief 添加谓词到访问栈（用于检测循环引用）
     * @return 如果已经访问过此谓词，返回false
     */
    bool pushPredicate(const LootCondition* predicate) noexcept;

    /**
     * @brief 从访问栈移除谓词
     */
    void popPredicate(const LootCondition* predicate) noexcept;

    friend class LootContextBuilder;

private:
    IWorld& m_world;
    math::Random& m_random;
    f32 m_luck = 0.0f;
    i32 m_lootingModifier = 0;
    std::unordered_map<std::string, void*> m_params;
    std::vector<std::shared_ptr<void>> m_ownedValues; // 拥有所有权的值存储
    LootTableResolver m_lootTableResolver;
    PredicateResolver m_predicateResolver;
    std::vector<const LootTable*> m_visitedTables;         // 用于检测掉落表循环引用
    std::vector<const LootCondition*> m_visitedPredicates; // 用于检测谓词循环引用
};

} // namespace loot
} // namespace mc

// Include LootContextBuilder after LootContext definition (for convenience)
#include "LootContextBuilder.hpp"
