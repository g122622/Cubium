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
 * @brief 按攻击者附魔等级增加掉落数量（minecraft:enchanted_count_increase）
 *
 * 对齐原版 `EnchantedCountIncreaseFunction`：
 * - 从上下文的 `ATTACKING_ENTITY` 读**指定附魔**在该实体装备上的等级；
 * - 等级为 0 时**原样返回、不消耗随机数**（原版在这里就 return 了）；
 * - 否则 `grow(round(level * count.getFloat(ctx)))`，`limit > 0` 时再截断。
 */
class EnchantedCountIncreaseFunction : public LootFunction {
public:
    /**
     * @brief 构造
     * @param enchantmentId 参与计算的附魔 ID（数据包通常是 minecraft:looting）
     * @param count 每级附魔增加的随机数量范围
     * @param limit 掉落数量上限（0 表示不限）
     */
    explicit EnchantedCountIncreaseFunction(
        std::string enchantmentId, const RandomValueRange& count = RandomValueRange(0.0f, 1.0f), i32 limit = 0);

    [[nodiscard]] ItemStack run(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const override
    {
        return "enchanted_count_increase";
    }

    [[nodiscard]] const std::string& getEnchantmentId() const
    {
        return m_enchantmentId;
    }

    [[nodiscard]] const RandomValueRange& getCount() const
    {
        return m_count;
    }

    [[nodiscard]] i32 getLimit() const
    {
        return m_limit;
    }

private:
    std::string m_enchantmentId;
    RandomValueRange m_count;
    i32 m_limit; // 0 means unlimited
};

} // namespace loot
} // namespace mc
