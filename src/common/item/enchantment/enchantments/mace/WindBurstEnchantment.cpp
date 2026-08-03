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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "WindBurstEnchantment.hpp"
#include "../trident/ImpalingEnchantment.hpp"
#include "../weapon/DamageEnchantment.hpp"
#include "BreachEnchantment.hpp"
#include "DensityEnchantment.hpp"
#include "common/item/enchantment/Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool WindBurstEnchantment::isCompatibleWith(const Enchantment& other) const
{
    // 风爆不与任何伤害组附魔互斥，可以与致密、破甲、锋利、亡灵杀手、节肢杀手、穿刺共存
    if (dynamic_cast<const DensityEnchantment*>(&other) != nullptr) {
        return true;
    }
    if (dynamic_cast<const BreachEnchantment*>(&other) != nullptr) {
        return true;
    }
    if (dynamic_cast<const DamageEnchantment*>(&other) != nullptr) {
        return true;
    }
    if (dynamic_cast<const ImpalingEnchantment*>(&other) != nullptr) {
        return true;
    }
    // 同种附魔不可叠加
    if (this == &other) {
        return false;
    }
    return isTypeCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
