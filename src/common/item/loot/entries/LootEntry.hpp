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
    [[nodiscard]] virtual i32 getWeight() const { return m_weight; }

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
    [[nodiscard]] i32 getQuality() const { return m_quality; }

    /**
     * @brief 设置权重
     */
    void setWeight(i32 weight) { m_weight = weight; }

    /**
     * @brief 设置质量
     */
    void setQuality(i32 quality) { m_quality = quality; }

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
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

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
    [[nodiscard]] const std::vector<std::unique_ptr<LootFunction>>& getFunctions() const { return m_functions; }

    /**
     * @brief 应用所有函数到物品堆
     *
     * @param stack 原始物品堆
     * @param context 掉落上下文
     * @return 修改后的物品堆
     */
    [[nodiscard]] ItemStack applyFunctions(ItemStack stack, LootContext& context) const;

    /**
     * @brief 扩展条目（生成候选列表）
     *
     * 将条目添加到候选列表中，用于加权随机选择。
     *
     * @param context 掉落上下文
     * @param consumer 接收候选条目的回调
     */
    virtual void expand(LootContext& context, std::function<void(LootEntry&)> consumer) const = 0;

    /**
     * @brief 生成物品
     *
     * 执行条目的生成逻辑，将生成的物品传递给消费者。
     *
     * @param consumer 接收物品的回调
     * @param context 掉落上下文
     * @return 是否成功生成（用于条件判断）
     */
    virtual bool generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const = 0;

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
