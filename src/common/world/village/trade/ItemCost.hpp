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

#include <optional>
#include <nlohmann/json.hpp>

namespace mc {
namespace world {
namespace village {
namespace trade {

/**
 * @brief 交易代价的物品模板（vanilla ItemCost）
 *
 * 描述"玩家需要支付什么"：物品模板 + 期望数量 + 精确组件谓词。
 *
 * 之所以不是裸 ItemStack：药水代价需要区分"任意水瓶"与"带特定效果的药水"
 * （vanilla ItemCost 用 DataComponentExactPredicate 精确匹配 potion_contents 组件）。
 * 每级谓词只保存 partial json（comp 名称 → 期望值），匹配时由 ItemStack 的
 * customData / potionId 等承载进行判定；真正用于展示与比较的 ItemStack 由
 * templateStack 给出。
 */
class ItemCost {
public:
    /**
     * @brief 构造无组件谓词的代价
     * @param stack 物品模板（数量会被 count 覆盖）
     * @param count 期望数量（正数）
     */
    ItemCost(ItemStack stack, i32 count);

    /**
     * @brief 以物品模板构造代价（数量取模板自身数量）
     * @param stack 物品模板
     */
    explicit ItemCost(ItemStack stack);

    /**
     * @brief 生成用于展示/匹配的物品堆副本
     * @return 数量为 count 的物品堆副本
     */
    [[nodiscard]] ItemStack toStack() const;

    /**
     * @brief 判断给定物品堆是否满足本代价
     *
     * 只比较物品类型、数量与逐组件谓词；数量门控由调用方按需求修正后的数量另行处理。
     *
     * @param stack 待检查的物品堆
     * @return 满足返回 true
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 只校验物品类型与组件谓词，忽略数量
     * @param stack 待检查的物品堆（可为空）
     * @return 类型与组件匹配返回 true
     */
    [[nodiscard]] bool testComponentwise(const ItemStack& stack) const;

    /**
     * @brief 获取期望数量
     */
    [[nodiscard]] i32 count() const noexcept
    {
        return m_count;
    }

    /**
     * @brief 获取物品模板（数量为 count）
     */
    [[nodiscard]] const ItemStack& templateStack() const noexcept
    {
        return m_stack;
    }

    // ========== 逐组件谓词 ==========

    /**
     * @brief 追加药水内容谓词（potion_contents 组件的 potion 字段）
     * @param potionId 药水资源位置（如 "minecraft:water"）
     * @return 本对象引用（链式调用）
     */
    ItemCost& withPotion(const std::string& potionId);

    /**
     * @brief 追加 custom_data 谓词（根标签必须包含全部给定键值）
     * @param data 期望的 custom_data 片段（必须为对象）
     * @return 本对象引用（链式调用）
     */
    ItemCost& withCustomData(nlohmann::json data);

private:
    /**
     * @brief 检查全部逐组件谓词
     * @param stack 待检查的物品堆
     * @return 全部匹配返回 true
     */
    [[nodiscard]] bool _testComponents(const ItemStack& stack) const;

    /// 物品模板（数量已与 m_count 同步）
    ItemStack m_stack;

    /// 期望数量
    i32 m_count;

    /// potion_contents.potion 期望值（空 = 不约束）
    std::optional<std::string> m_potionId;

    /// custom_data 期望片段（null = 不约束）
    nlohmann::json m_customData;
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc