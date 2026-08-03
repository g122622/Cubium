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

#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 位置检查条件
 *
 * 检查指定偏移位置是否满足位置谓词（生物群系、维度等）。
 *
 * JSON 格式示例:
 * @code
 * {
 *   "condition": "minecraft:location_check",
 *   "predicate": { "biome": "minecraft:jungle" },
 *   "offsetY": 1
 * }
 * @endcode
 */
class LocationCheckCondition : public LootCondition {
public:
    LocationCheckCondition() = default;

    /**
     * @brief 构造位置检查条件
     * @param predicate 位置谓词
     * @param offsetX X偏移（默认0）
     * @param offsetY Y偏移（默认0）
     * @param offsetZ Z偏移（默认0）
     */
    LocationCheckCondition(advancement::LocationPredicate predicate, i32 offsetX = 0, i32 offsetY = 0, i32 offsetZ = 0);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "location_check"; }

    [[nodiscard]] const advancement::LocationPredicate& getPredicate() const noexcept { return m_predicate; }
    [[nodiscard]] i32 getOffsetX() const noexcept { return m_offsetX; }
    [[nodiscard]] i32 getOffsetY() const noexcept { return m_offsetY; }
    [[nodiscard]] i32 getOffsetZ() const noexcept { return m_offsetZ; }

private:
    advancement::LocationPredicate m_predicate;
    i32 m_offsetX = 0;
    i32 m_offsetY = 0;
    i32 m_offsetZ = 0;
    bool m_isAny = true;
};

} // namespace loot
} // namespace mc
