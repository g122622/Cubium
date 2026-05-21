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
#include <memory>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 或条件
 *
 * 任一子条件满足即满足。
 * 参考: net.minecraft.loot.conditions.Alternative (使用 OR 逻辑)
 */
class OrCondition : public LootCondition {
public:
    OrCondition() = default;
    explicit OrCondition(std::vector<std::unique_ptr<LootCondition>> conditions);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "or"; }

    void addCondition(std::unique_ptr<LootCondition> condition);
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

private:
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

} // namespace loot
} // namespace mc
