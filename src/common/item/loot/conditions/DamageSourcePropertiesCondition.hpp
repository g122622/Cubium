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

#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 伤害源属性条件
 *
 * 检查伤害源是否满足指定属性谓词。
 *
 * JSON 格式示例:
 * @code
 * {
 *   "condition": "minecraft:damage_source_properties",
 *   "predicate": { "is_lightning": true }
 * }
 * @endcode
 */
class DamageSourcePropertiesCondition : public LootCondition {
public:
    DamageSourcePropertiesCondition() = default;

    /**
     * @brief 构造伤害源属性条件
     * @param predicate 伤害源谓词
     */
    explicit DamageSourcePropertiesCondition(advancement::DamageSourcePredicate predicate);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "damage_source_properties"; }

    [[nodiscard]] const advancement::DamageSourcePredicate& getPredicate() const noexcept { return m_predicate; }

private:
    advancement::DamageSourcePredicate m_predicate;
    bool m_isAny = true;
};

} // namespace loot
} // namespace mc
