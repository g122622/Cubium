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

#include "LootPool.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootParameterSet.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "context/LootContext.hpp"
#include "functions/LootFunctions.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 掉落表
 *
 * 定义实体死亡、方块破坏等情况下生成的物品。
 * 参考: net.minecraft.loot.LootTable
 *
 * 结构：
 * - 掉落表包含多个池（LootPool）
 * - 每个池定义掷骰次数和多个条目（LootEntry）
 * - 条目按权重随机选择
 * - 条目可以是物品、另一个掉落表引用等
 *
 * 示例用法：
 * @code
 * LootTable table;
 * auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
 * pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 2.0f), 10, 1));
 * table.addPool(std::move(pool));
 *
 * auto context = LootContextBuilder(world).build();
 * auto drops = table.generate(*context);
 * @endcode
 */
class LootTable {
public:
    /**
     * @brief 空掉落表常量
     */
    static const LootTable EMPTY;

    LootTable() = default;
    ~LootTable() = default;

    // 禁止拷贝
    LootTable(const LootTable&) = delete;
    LootTable& operator=(const LootTable&) = delete;

    // 允许移动
    LootTable(LootTable&&) noexcept = default;
    LootTable& operator=(LootTable&&) noexcept = default;

    /**
     * @brief 创建副本
     *
     * 深拷贝所有池、函数及标识信息。用于 TableLootEntry 深拷贝内联表等场景。
     */
    [[nodiscard]] std::unique_ptr<LootTable> clone() const;

    // ========== 池管理 ==========

    /**
     * @brief 添加掉落池
     */
    void addPool(std::unique_ptr<LootPool> pool);

    /**
     * @brief 获取所有池
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootPool>>& getPools() const { return m_pools; }

    /**
     * @brief 根据名称获取池
     */
    [[nodiscard]] LootPool* getPool(const std::string& name);

    /**
     * @brief 移除池
     */
    std::unique_ptr<LootPool> removePool(const std::string& name);

    /**
     * @brief 获取池数量
     */
    [[nodiscard]] size_t poolCount() const { return m_pools.size(); }

    // ========== 表级函数 ==========

    /**
     * @brief 添加表级函数
     *
     * 表级函数应用于此表中所有池生成的物品。
     */
    void addFunction(std::unique_ptr<LootFunction> function);

    /**
     * @brief 获取表级函数
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootFunction>>& getFunctions() const { return m_functions; }

    /**
     * @brief 应用所有表级函数到物品
     */
    [[nodiscard]] ItemStack applyFunctions(ItemStack stack, LootContext& context) const;

    // ========== 参数集 ==========

    /**
     * @brief 获取参数集
     */
    [[nodiscard]] const LootParameterSet& getParameterSet() const { return m_paramSet; }

    /**
     * @brief 设置参数集
     */
    void setParameterSet(const LootParameterSet& paramSet) { m_paramSet = paramSet; }

    // ========== 标识符 ==========

    /**
     * @brief 获取掉落表ID
     */
    [[nodiscard]] const std::string& getId() const { return m_id; }

    /**
     * @brief 设置掉落表ID
     */
    void setId(const std::string& id) { m_id = id; }

    // ========== 生成 ==========

    /**
     * @brief 生成掉落物
     *
     * @param context 掉落上下文
     * @return 生成的物品列表
     */
    [[nodiscard]] std::vector<ItemStack> generate(LootContext& context) const;

    /**
     * @brief 生成掉落物到消费者
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     */
    void generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const;

    /**
     * @brief 递归生成（处理嵌套掉落表）
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     */
    void recursiveGenerate(std::function<void(const ItemStack&)> consumer, LootContext& context) const;

    // ========== 序列化 ==========

    /**
     * @brief 从JSON加载掉落表
     */
    [[nodiscard]] static Result<std::unique_ptr<LootTable>> fromJson(const std::string& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] std::string toJson() const;

private:
    std::string m_id;
    LootParameterSet m_paramSet;
    std::vector<std::unique_ptr<LootPool>> m_pools;
    std::vector<std::unique_ptr<LootFunction>> m_functions;
};

/**
 * @brief 掉落表构建器
 *
 * 参考: net.minecraft.loot.LootTable.Builder
 */
class LootTableBuilder {
public:
    LootTableBuilder() = default;

    /**
     * @brief 设置掉落表ID
     */
    LootTableBuilder& id(const std::string& id)
    {
        m_id = id;
        return *this;
    }

    /**
     * @brief 设置参数集
     */
    LootTableBuilder& paramSet(const LootParameterSet& paramSet)
    {
        m_paramSet = paramSet;
        return *this;
    }

    /**
     * @brief 添加池
     */
    LootTableBuilder& pool(std::unique_ptr<LootPool> pool)
    {
        m_pools.push_back(std::move(pool));
        return *this;
    }

    /**
     * @brief 添加表级函数
     */
    LootTableBuilder& function(std::unique_ptr<LootFunction> func)
    {
        m_functions.push_back(std::move(func));
        return *this;
    }

    /**
     * @brief 构建掉落表
     */
    [[nodiscard]] std::unique_ptr<LootTable> build() const;

private:
    std::string m_id;
    LootParameterSet m_paramSet;
    std::vector<std::unique_ptr<LootPool>> m_pools;
    std::vector<std::unique_ptr<LootFunction>> m_functions;
};

} // namespace loot
} // namespace mc
