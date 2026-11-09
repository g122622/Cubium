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
    f32 unenchantedChance, std::unique_ptr<LevelBasedValue> enchantedChance, std::string enchantmentId)
    : m_unenchantedChance(unenchantedChance)
    , m_enchantedChance(std::move(enchantedChance))
    , m_enchantmentId(std::move(enchantmentId))
{
    MC_ASSERT_RELEASE(m_enchantedChance != nullptr);
}

bool RandomChanceWithEnchantedBonusCondition::test(LootContext& context) const
{
    // 原版从 ATTACKING_ENTITY 身上取附魔等级（`EnchantmentHelper#getEnchantmentLevel`，
    // 该重载按附魔的适用槽位扫描装备）。没有攻击者或不是生物时等级为 0。
    i32 level = 0;
    if (auto* entity = context.get<Entity>(LootParams::KILLER_ENTITY); entity != nullptr) {
        if (auto* living = dynamic_cast<LivingEntity*>(entity); living != nullptr) {
            // 走实体重载：它会按该附魔自身的 `slots` 声明筛选槽位后取最大等级，
            // 与 `Enchantment#getSlotItems` 一致（掠夺只认主手，保护认四个护甲槽）。
            level = item::enchant::EnchantmentHelper::getEnchantmentLevel(*living, m_enchantmentId);
        }
    }

    // 等级为 0 时用固定概率；否则用 enchanted_chance 表达式求值。
    const f32 chance = level > 0 ? m_enchantedChance->calculate(level) : m_unenchantedChance;
    return context.getRandom().nextFloat() < chance;
}

std::unique_ptr<LootCondition> RandomChanceWithEnchantedBonusCondition::clone() const noexcept
{
    return std::make_unique<RandomChanceWithEnchantedBonusCondition>(
        m_unenchantedChance, m_enchantedChance->clone(), m_enchantmentId);
}

} // namespace loot
} // namespace mc
