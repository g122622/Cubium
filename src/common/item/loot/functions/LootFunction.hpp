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

#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {

// Forward declarations
class ItemStack;

namespace loot {

/**
 * @brief 掉落函数基类
 *
 * 掉落函数用于修改生成的物品堆，如设置数量、应用附魔加成等。
 * 参考: net.minecraft.loot.functions.ILootFunction
 *
 * 函数在条件检查之后、物品返回之前执行。
 * 多个函数按顺序执行，前一个函数的输出作为后一个函数的输入。
 */
class LootFunction {
public:
    virtual ~LootFunction() = default;

    /**
     * @brief 应用函数到物品堆
     *
     * 修改或替换物品堆。
     *
     * @param stack 原始物品堆（可被修改）
     * @param context 掉落上下文
     * @return 修改后的物品堆（可以返回空堆表示不生成物品）
     */
    virtual ItemStack apply(ItemStack stack, LootContext& context) const = 0;

    /**
     * @brief 创建函数副本
     */
    [[nodiscard]] virtual std::unique_ptr<LootFunction> clone() const noexcept = 0;

    /**
     * @brief 获取函数类型标识
     */
    [[nodiscard]] virtual std::string getType() const = 0;

    // ========== 条件管理 ==========

    /**
     * @brief 添加条件
     *
     * 只有所有条件都满足时，函数才会执行。
     */
    void addCondition(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 获取所有条件
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

    /**
     * @brief 检查所有条件是否满足
     */
    [[nodiscard]] bool testConditions(LootContext& context) const;

protected:
    LootFunction() = default;
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

} // namespace loot
} // namespace mc
