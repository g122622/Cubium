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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MaceItem.hpp"
#include "../../../util/math/MathUtils.hpp"

namespace mc {
namespace item {

MaceItem::MaceItem(const ItemProperties& properties)
    : Item(properties)
{}

f32 MaceItem::calculateSmashAttackDamage(f32 fallDistance)
{
    if (fallDistance <= 0.0f) {
        return 0.0f;
    }
    // 每格下落增加3点伤害，上限40
    f32 extraDamage = fallDistance * DAMAGE_PER_BLOCK_FALLEN;
    return std::min(extraDamage, MAX_EXTRA_DAMAGE);
}

} // namespace item
} // namespace mc
