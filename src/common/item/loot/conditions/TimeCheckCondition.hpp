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
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 时间检查条件
 *
 * 检查当前游戏时间是否在指定范围内，可选取模。
 *
 * JSON 格式示例:
 * @code
 * {
 *   "condition": "minecraft:time_check",
 *   "period": 24000,
 *   "value": { "min": 0, "max": 12000 }
 * }
 * @endcode
 */
class TimeCheckCondition : public LootCondition {
public:
    TimeCheckCondition() = default;

    /**
     * @brief 构造时间检查条件
     * @param period 取模周期（0表示不取模）
     * @param value 时间值范围
     */
    TimeCheckCondition(i64 period, RandomValueRange value);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "time_check"; }

    [[nodiscard]] i64 getPeriod() const noexcept { return m_period; }
    [[nodiscard]] const RandomValueRange& getValue() const noexcept { return m_value; }

private:
    i64 m_period = 0;         // 0 表示不取模
    RandomValueRange m_value; // 时间值范围
    bool m_hasPeriod = false;
};

} // namespace loot
} // namespace mc
