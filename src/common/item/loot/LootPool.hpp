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
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include "conditions/LootConditions.hpp"
#include "entries/LootEntry.hpp"
#include "functions/LootFunctions.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 掉落池
 *
 * 包含多个掉落条目，按权重随机选择。
 */
class LootPool {
public:
    /**
     * @brief 构造空掉落池
     */
    LootPool() = default;

    /**
     * @brief 构造掉落池
     * @param rolls 掷骰次数
     * @param bonusRolls 额外掷骰次数（受幸运值影响）
     */
    explicit LootPool(const RandomValueRange& rolls, const RandomValueRange& bonusRolls = RandomValueRange(0.0f, 0.0f));

    ~LootPool() = default;

    // 禁止拷贝
    LootPool(const LootPool&) = delete;
    LootPool& operator=(const LootPool&) = delete;

    // 允许移动
    LootPool(LootPool&&) noexcept = default;
    LootPool& operator=(LootPool&&) noexcept = default;

    /**
     * @brief 创建副本
     */
    [[nodiscard]] std::unique_ptr<LootPool> clone() const;

    // ========== 条目管理 ==========

    /**
     * @brief 添加掉落条目
     */
    void addEntry(std::unique_ptr<LootEntry> entry);

    /**
     * @brief 获取所有条目
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootEntry>>& getEntries() const { return m_entries; }

    // ========== 掷骰配置 ==========

    /**
     * @brief 获取掷骰次数范围
     */
    [[nodiscard]] const RandomValueRange& getRolls() const { return m_rolls; }

    /**
     * @brief 获取额外掷骰次数范围
     */
    [[nodiscard]] const RandomValueRange& getBonusRolls() const { return m_bonusRolls; }

    /**
     * @brief 设置掷骰次数
     */
    void setRolls(const RandomValueRange& rolls) { m_rolls = rolls; }

    /**
     * @brief 设置额外掷骰次数
     */
    void setBonusRolls(const RandomValueRange& bonusRolls) { m_bonusRolls = bonusRolls; }

    // ========== 名称 ==========

    /**
     * @brief 获取名称
     */
    [[nodiscard]] const std::string& getName() const { return m_name; }

    /**
     * @brief 设置名称
     */
    void setName(const std::string& name) { m_name = name; }

    // ========== 条件和函数 ==========

    /**
     * @brief 添加池级条件
     *
     * 条件用于控制整个池是否生成。
     * 所有条件必须通过，池才会执行掷骰。
     */
    void addCondition(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 获取池级条件
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

    /**
     * @brief 测试所有条件
     * @return 所有条件是否通过
     */
    [[nodiscard]] bool testConditions(LootContext& context) const;

    /**
     * @brief 添加池级函数
     *
     * 函数应用于此池中所有条目生成的物品。
     */
    void addFunction(std::unique_ptr<LootFunction> function);

    /**
     * @brief 获取池级函数
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootFunction>>& getFunctions() const { return m_functions; }

    /**
     * @brief 应用所有池级函数到物品
     */
    [[nodiscard]] ItemStack applyFunctions(ItemStack stack, LootContext& context) const;

    // ========== 生成 ==========

    /**
     * @brief 生成掉落物
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     */
    void generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const;

private:
    /**
     * @brief 执行一次掷骰
     */
    void _generateRoll(std::function<void(const ItemStack&)> consumer, LootContext& context) const;

private:
    std::string m_name;
    std::vector<std::unique_ptr<LootEntry>> m_entries;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
    std::vector<std::unique_ptr<LootFunction>> m_functions;
    RandomValueRange m_rolls{1.0f, 1.0f};
    RandomValueRange m_bonusRolls{0.0f, 0.0f};
};

/**
 * @brief 掉落池构建器
 */
class LootPoolBuilder {
public:
    LootPoolBuilder() = default;

    /**
     * @brief 设置掷骰次数
     */
    LootPoolBuilder& rolls(const RandomValueRange& rolls)
    {
        m_rolls = rolls;
        return *this;
    }

    /**
     * @brief 设置掷骰次数（固定值）
     */
    LootPoolBuilder& rolls(i32 value)
    {
        m_rolls = RandomValueRange(static_cast<f32>(value), static_cast<f32>(value));
        return *this;
    }

    /**
     * @brief 设置额外掷骰次数
     */
    LootPoolBuilder& bonusRolls(f32 min, f32 max)
    {
        m_bonusRolls = RandomValueRange(min, max);
        return *this;
    }

    /**
     * @brief 设置名称
     */
    LootPoolBuilder& name(const std::string& name)
    {
        m_name = name;
        return *this;
    }

    /**
     * @brief 添加条目
     */
    LootPoolBuilder& entry(std::unique_ptr<LootEntry> entry)
    {
        m_entries.push_back(std::move(entry));
        return *this;
    }

    /**
     * @brief 添加池级条件
     */
    LootPoolBuilder& condition(std::unique_ptr<LootCondition> cond)
    {
        m_conditions.push_back(std::move(cond));
        return *this;
    }

    /**
     * @brief 添加池级函数
     */
    LootPoolBuilder& function(std::unique_ptr<LootFunction> func)
    {
        m_functions.push_back(std::move(func));
        return *this;
    }

    /**
     * @brief 添加物品条目
     */
    LootPoolBuilder& item(const std::string& itemId, i32 count = 1, i32 weight = 1);

    /**
     * @brief 构建掉落池
     */
    [[nodiscard]] std::unique_ptr<LootPool> build() const;

private:
    std::string m_name;
    RandomValueRange m_rolls{1.0f, 1.0f};
    RandomValueRange m_bonusRolls{0.0f, 0.0f};
    std::vector<std::unique_ptr<LootEntry>> m_entries;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
    std::vector<std::unique_ptr<LootFunction>> m_functions;
};

} // namespace loot
} // namespace mc
