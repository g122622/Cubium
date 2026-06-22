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
 * copies of substantial portions of the Software.
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

namespace mc {
namespace item {
namespace enchant {

bool WindBurstEnchantment::isCompatibleWith(const Enchantment& other) const
{
    // 风爆不与伤害附魔互斥（MC 中风爆不属于 DAMAGE_EXCLUSIVE 组）
    // 但风爆仍然与自身互斥（同种附魔不可叠加）
    if (this == &other) {
        return false;
    }
    // 类型兼容性检查：风爆是武器附魔，与其他武器附魔类型兼容
    // 但按照 MC 规则，风爆可以与致密或破甲共存
    return isTypeCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
