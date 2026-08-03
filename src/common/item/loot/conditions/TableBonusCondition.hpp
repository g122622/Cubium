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
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 附魔等级概率表条件（minecraft:table_bonus）
 *
 * 根据工具上指定附魔的等级，以不同的概率满足条件。
 * 概率表中的每个元素对应一个附魔等级（从等级0开始），
 * 如果附魔等级超出概率表长度，则使用最后一个概率值。
 *
 * 参考: net.minecraft.world.level.storage.loot.predicates.BonusLevelTableCondition
 *
 * JSON 格式:
 * @code
 * {
 *   "condition": "minecraft:table_bonus",
 *   "enchantment": "minecraft:fortune",
 *   "chances": [0.1, 0.15, 0.2, 0.25]
 * }
 * @endcode
 *
 * 用法示例:
 * @code
 * // 时运等级0: 10%概率, 等级1: 15%, 等级2: 20%, 等级3: 25%
 * auto cond = std::make_unique<TableBonusCondition>(
 *     "minecraft:fortune", std::vector<f32>{0.1f, 0.15f, 0.2f, 0.25f});
 * @endcode
 */
class TableBonusCondition : public LootCondition {
public:
    /**
     * @brief 构造附魔等级概率表条件
     * @param enchantmentId 附魔ID（如 "minecraft:fortune"）
     * @param chances 概率表，每个元素对应一个附魔等级的概率
     */
    explicit TableBonusCondition(std::string enchantmentId, std::vector<f32> chances);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "table_bonus"; }

    /**
     * @brief 获取附魔ID
     */
    [[nodiscard]] const std::string& getEnchantmentId() const { return m_enchantmentId; }

    /**
     * @brief 获取概率表
     */
    [[nodiscard]] const std::vector<f32>& getChances() const { return m_chances; }

private:
    std::string m_enchantmentId;
    std::vector<f32> m_chances;
};

} // namespace loot
} // namespace mc
