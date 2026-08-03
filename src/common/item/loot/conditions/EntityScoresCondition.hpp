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

#include <memory>
#include <string>
#include <unordered_map>

#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/RandomRanges.hpp"

namespace mc {
namespace loot {

/**
 * @brief 实体分数条件
 *
 * 检查指定实体的记分板分数是否在指定范围内。
 * 参考: net.minecraft.loot.conditions.EntityHasScore
 *
 * JSON 格式示例:
 * @code
 * {
 *   "condition": "minecraft:entity_scores",
 *   "entity": "this",
 *   "scores": {
 *     "objective_name": { "min": 1, "max": 10 }
 *   }
 * }
 * @endcode
 */
class EntityScoresCondition : public LootCondition {
public:
    EntityScoresCondition() = default;

    /**
     * @brief 构造实体分数条件
     * @param target 实体目标
     * @param scores 记分板目标名到分数范围的映射
     */
    EntityScoresCondition(
        EntityPropertiesCondition::EntityTarget target, std::unordered_map<std::string, RandomValueRange> scores);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "entity_scores"; }

    [[nodiscard]] EntityPropertiesCondition::EntityTarget getTarget() const { return m_target; }
    [[nodiscard]] const std::unordered_map<std::string, RandomValueRange>& getScores() const { return m_scores; }

private:
    EntityPropertiesCondition::EntityTarget m_target = EntityPropertiesCondition::EntityTarget::This;
    std::unordered_map<std::string, RandomValueRange> m_scores;
};

} // namespace loot
} // namespace mc
