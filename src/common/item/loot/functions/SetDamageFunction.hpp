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
 * @brief 设置损坏程度函数
 *
 * 用于设置物品的耐久度损坏程度。适用于工具、武器等可损坏物品的掉落。
 */
class SetDamageFunction : public LootFunction {
public:
    /**
     * @brief 构造设置损坏程度函数
     * @param durability 损坏程度范围（0.0 = 完好，1.0 = 完全损坏）
     * @param add 是否叠加到现有损坏程度上（默认为 false，即替换）
     */
    explicit SetDamageFunction(const RandomValueRange& durability, bool add = false);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "set_damage"; }

    [[nodiscard]] const RandomValueRange& getDurability() const { return m_durability; }
    [[nodiscard]] bool isAdd() const { return m_add; }

private:
    RandomValueRange m_durability; // 损坏程度范围（0.0 = 完好，1.0 = 完全损坏）
    bool m_add;                    // 是否叠加到现有损坏程度上
};

} // namespace loot
} // namespace mc
