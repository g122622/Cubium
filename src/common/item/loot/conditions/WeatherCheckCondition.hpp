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
#include <memory>
#include <optional>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 天气检查条件
 *
 * 检查当前世界的天气状态（下雨/雷暴）。
 * 参考: net.minecraft.loot.conditions.WeatherCheck
 *
 * JSON 格式示例:
 * @code
 * {
 *   "condition": "minecraft:weather_check",
 *   "raining": true,
 *   "thundering": false
 * }
 * @endcode
 *
 * 可选字段：raining 和 thundering，不设置则不检查对应天气。
 */
class WeatherCheckCondition : public LootCondition {
public:
    WeatherCheckCondition() = default;

    /**
     * @brief 构造天气检查条件
     * @param raining 是否要求下雨（nullopt 表示不检查）
     * @param thundering 是否要求雷暴（nullopt 表示不检查）
     */
    WeatherCheckCondition(std::optional<bool> raining, std::optional<bool> thundering);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "weather_check"; }

    [[nodiscard]] const std::optional<bool>& getRaining() const { return m_raining; }
    [[nodiscard]] const std::optional<bool>& getThundering() const { return m_thundering; }

private:
    std::optional<bool> m_raining;
    std::optional<bool> m_thundering;
};

} // namespace loot
} // namespace mc
