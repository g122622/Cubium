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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 设置数量函数
 *
 * 用于设置物品堆的数量。可以设置固定数量或随机范围。
 * 参考: net.minecraft.loot.functions.SetCount
 *
 * 通过 add 参数控制是替换原数量还是叠加到原数量上。
 */
class SetCountFunction : public LootFunction {
public:
    /**
     * @brief 构造设置数量函数
     * @param count 数量范围
     * @param add 是否叠加到现有数量上（默认为 false，即替换）
     */
    explicit SetCountFunction(const RandomValueRange& count, bool add = false);

    /**
     * @brief 应用函数到物品堆
     * @param stack 原始物品堆
     * @param context 掉落上下文
     * @return 修改后的物品堆
     */
    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;

    /**
     * @brief 创建函数副本
     */
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    /**
     * @brief 获取函数类型标识
     */
    [[nodiscard]] std::string getType() const override { return "set_count"; }

    /**
     * @brief 获取数量范围
     */
    [[nodiscard]] const RandomValueRange& getCount() const { return m_count; }

    /**
     * @brief 是否叠加模式
     */
    [[nodiscard]] bool isAdd() const { return m_add; }

private:
    RandomValueRange m_count; // 数量范围
    bool m_add;               // 是否叠加到现有数量
};

} // namespace loot
} // namespace mc
