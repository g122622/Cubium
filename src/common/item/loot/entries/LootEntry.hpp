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
#include "common/item/loot/LootConsumer.hpp"
#include "common/item/loot/conditions/LootConditions.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/item/loot/functions/LootFunctions.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {
namespace loot {

// Forward declarations
class LootPool;

/**
 * @brief 掉落条目类型
 */
enum class LootEntryType : u8 {
    Empty,        // 空条目
    Item,         // 物品条目
    Tag,          // 标签条目
    Table,        // 掉落表引用
    Dynamic,      // 动态条目
    Alternatives, // 替代条目
    Sequence,     // 序列条目
    Group         // 组条目
};

/**
 * @brief 掉落条目基类
 *
 * 定义掉落表中单个条目的基础结构。
 */
class LootEntry {
public:
    virtual ~LootEntry();

    /**
     * @brief 获取条目类型
     */
    [[nodiscard]] virtual LootEntryType getType() const = 0;

    /**
     * @brief 创建副本
     */
    [[nodiscard]] virtual std::unique_ptr<LootEntry> clone() const = 0;

    /**
     * @brief 获取权重
     */
    [[nodiscard]] virtual i32 getWeight() const
    {
        return m_weight;
    }

    /**
     * @brief 获取有效权重（考虑幸运值）
     */
    [[nodiscard]] virtual i32 getEffectiveWeight(f32 luck) const
    {
        return m_weight + static_cast<i32>(luck * m_quality);
    }

    /**
     * @brief 获取质量（幸运值加成系数）
     */
    [[nodiscard]] i32 getQuality() const
    {
        return m_quality;
    }

    /**
     * @brief 设置权重
     */
    void setWeight(i32 weight)
    {
        m_weight = weight;
    }

    /**
     * @brief 设置质量
     */
    void setQuality(i32 quality)
    {
        m_quality = quality;
    }

    // ========== 条件管理 ==========

    /**
     * @brief 添加掉落条件
     *
     * 条件用于控制条目是否生效。所有条件都必须满足才能生成物品。
     *
     * @param condition 条件
     */
    void addCondition(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 获取所有条件
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const
    {
        return m_conditions;
    }

    /**
     * @brief 检查所有条件是否满足
     *
     * @param context 掉落上下文
     * @return 如果所有条件都满足返回true
     */
    [[nodiscard]] bool testConditions(LootContext& context) const;

    // ========== 函数管理 ==========

    /**
     * @brief 添加掉落函数
     *
     * 函数在条件检查之后、物品返回之前执行。
     * 多个函数按顺序执行，前一个函数的输出作为后一个函数的输入。
     *
     * @param function 函数
     */
    void addFunction(std::unique_ptr<LootFunction> function);

    /**
     * @brief 获取所有函数
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootFunction>>& getFunctions() const
    {
        return m_functions;
    }

    /**
     * @brief 用条目级函数**装饰**一个消费回调
     *
     * 对齐原版 `LootItemFunction#decorate`——`LootPoolSingletonContainer$EntryBase#createItemStack`
     * 走的就是它：装饰后的回调每收到一个堆，就用条目自己的函数链变换它，再把结果交给
     * 下游回调。**空堆也照样变换、照样交付**：原版不做任何过滤，空堆要到
     * `createStackSplitter` 才被丢掉。提前过滤会让后续函数少执行一次，消耗随机数的
     * 函数（`enchanted_count_increase`、`apply_bonus`……）因此与原版错位。
     *
     * @param consumer 下游回调
     * @param context 掉落上下文
     */
    [[nodiscard]] ConsumerPtr decorate(ConsumerPtr consumer, LootContext& context) const;

    /**
     * @brief 扩展条目（生成候选列表）
     *
     * 对齐原版 `LootPoolEntryContainer#expand`：**在池的掷骰阶段**就把组合条目
     * （alternatives / group / sequence）展开成候选条目，并用条目自身的条件过滤。
     * 条件不满足的条目不会进入候选，因此既不占权重也不影响随机数消耗。
     *
     * 这与「先按权重选中条目、再判定条件」有本质区别：原版是前者。两种顺序在
     * `alternatives` 上会给出完全不同的结果（前者只让条件满足的分支参与抽签）。
     *
     * 条目一旦成为候选，`generate` 不再重新判定条件——否则带随机数的条件会被
     * 评估两次，随机数序列与原版错位。
     *
     * @param context 掉落上下文
     * @param consumer 接收候选条目的回调
     * @return 是否至少展开出一个候选
     */
    virtual bool expand(LootContext& context, const std::function<void(const LootEntry&)>& consumer) const = 0;

    /**
     * @brief 生成物品
     *
     * 执行条目的生成逻辑，将生成的物品传递给消费者。
     *
     * **不再重新判定条目条件**：原版在 `expand` 阶段已经用条件筛过候选，
     * `createItemStack` 只负责生成物品。重复判定会让带随机数的条件
     * （`random_chance` / `table_bonus`）多消耗一次随机数，使后续抽取错位。
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     */
    virtual void generate(ConsumerPtr consumer, LootContext& context) const = 0;

protected:
    LootEntry() = default;

    explicit LootEntry(i32 weight, i32 quality = 0)
        : m_weight(weight)
        , m_quality(quality)
    {}

    i32 m_weight = 1;
    i32 m_quality = 0;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
    std::vector<std::unique_ptr<LootFunction>> m_functions;
};

} // namespace loot
} // namespace mc
