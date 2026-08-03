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

#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"

namespace mc {
namespace loot {

/**
 * @brief 实体属性条件
 *
 * 检查指定实体（this/killer/direct_killer/killer_player）是否满足属性谓词。
 * 使用 advancement 命名空间中的 EntityPredicate 进行匹配。
 *
 * 参考: net.minecraft.loot.conditions.EntityHasProperty
 *
 * JSON 格式示例:
 * @code
 * {
 *   "condition": "minecraft:entity_properties",
 *   "entity": "this",
 *   "predicate": {
 *     "flags": { "is_on_fire": true }
 *   }
 * }
 * @endcode
 */
class EntityPropertiesCondition : public LootCondition {
public:
    /**
     * @brief 实体目标类型
     */
    enum class EntityTarget {
        This,         // 当前实体
        Killer,       // 击杀者
        DirectKiller, // 直接击杀者
        KillerPlayer  // 击杀玩家
    };

    EntityPropertiesCondition() = default;

    /**
     * @brief 构造实体属性条件
     * @param target 实体目标
     * @param predicate 实体谓词（空谓词表示只检查实体是否存在）
     */
    EntityPropertiesCondition(EntityTarget target, advancement::EntityPredicate predicate);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "entity_properties"; }

    [[nodiscard]] EntityTarget getTarget() const noexcept { return m_target; }
    [[nodiscard]] const advancement::EntityPredicate& getPredicate() const noexcept { return m_predicate; }

    /**
     * @brief 将字符串转换为 EntityTarget
     */
    [[nodiscard]] static EntityTarget parseEntityTarget(const std::string& str);

    /**
     * @brief 将 EntityTarget 转换为字符串
     */
    [[nodiscard]] static std::string entityTargetToString(EntityTarget target);

private:
    EntityTarget m_target = EntityTarget::This; ///< 实体目标类型
    advancement::EntityPredicate m_predicate;   ///< 实体谓词
    bool m_isAny = true;                        ///< 是否为空谓词（只检查实体是否存在）
};

} // namespace loot
} // namespace mc
