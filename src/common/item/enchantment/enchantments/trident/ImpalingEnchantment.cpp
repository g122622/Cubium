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

#include "ImpalingEnchantment.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"

namespace mc {
namespace item {
namespace enchant {

f32 ImpalingEnchantment::getDamageBonus(i32 level, const LivingEntity* target) const noexcept
{
    // 对齐 MC Java 1.21.11 Enchantments.java:989-996：穿刺额外伤害判定目标用
    // EntityTypeTags.SENSITIVE_TO_IMPALING（= AQUATIC）标签，每级 +2.5 伤害。
    //
    // 此前偏差：用 getCreatureAttribute()==CreatureAttribute::Water 判定，而 Cubium 仅
    // GuardianEntity 显式返 Water，导致穿刺只对守卫者/远古守卫者额外伤害，遗漏
    // turtle/axolotl/cod/pufferfish/salmon/tropical_fish/dolphin/squid/glow_squid/
    // tadpole 等 10 个水生生物。vanilla 1.21.11 已全面用 EntityTypeTags 标签替代
    // getMobType 枚举判定附魔目标（Enchantments.java:994 SENSITIVE_TO_IMPALING）。
    //
    // AQUATIC 标签成员（EntityTypeTags.cpp:504-517）与 vanilla 完全一致（12 成员）。
    // SENSITIVE_TO_IMPALING = AQUATIC（EntityTypeTags.cpp:524-525）。
    if (target == nullptr) {
        return 0.0f;
    }
    if (EntityTypeTags::SENSITIVE_TO_IMPALING().contains(target->getTypeId())) {
        return static_cast<f32>(level) * 2.5f;
    }
    return 0.0f;
}

} // namespace enchant
} // namespace item
} // namespace mc
