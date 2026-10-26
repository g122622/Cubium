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

#include "RandomChanceWithEnchantedBonusCondition.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

RandomChanceWithEnchantedBonusCondition::RandomChanceWithEnchantedBonusCondition(
    f32 unenchantedChance, f32 enchantedBase, f32 perLevelAboveFirst, std::string enchantmentId)
    : m_unenchantedChance(unenchantedChance)
    , m_enchantedBase(enchantedBase)
    , m_perLevelAboveFirst(perLevelAboveFirst)
    , m_enchantmentId(std::move(enchantmentId))
{}

bool RandomChanceWithEnchantedBonusCondition::test(LootContext& context) const
{
    // 原版从 ATTACKING_ENTITY 身上取附魔等级（`EnchantmentHelper#getEnchantmentLevel`，
    // 该重载会扫描实体的全部装备槽）。没有攻击者或不是生物时等级为 0。
    i32 level = 0;
    if (auto* entity = context.get<Entity>(LootParams::KILLER_ENTITY); entity != nullptr) {
        if (auto* living = dynamic_cast<LivingEntity*>(entity); living != nullptr) {
            // 原版是「遍历该附魔适用的所有装备槽取最大值」，这里按主手读取。
            // TODO(loot-enchantment-slots): 原版 `Enchantment#getSlotItems` 会扫描全部适用槽位
            //   （头盔/胸甲/护腿/靴子/主手/副手）；当前只读主手，掠夺附魔是主手附魔，暂不受影响。
            level = item::enchant::EnchantmentHelper::getEnchantmentLevel(*living, m_enchantmentId);
        }
    }

    // LevelBasedValue.Linear: base + per_level_above_first * (level - 1)
    const f32 chance =
        level > 0 ? m_enchantedBase + m_perLevelAboveFirst * static_cast<f32>(level - 1) : m_unenchantedChance;
    return context.getRandom().nextFloat() < chance;
}

std::unique_ptr<LootCondition> RandomChanceWithEnchantedBonusCondition::clone() const noexcept
{
    return std::make_unique<RandomChanceWithEnchantedBonusCondition>(
        m_unenchantedChance, m_enchantedBase, m_perLevelAboveFirst, m_enchantmentId);
}

} // namespace loot
} // namespace mc