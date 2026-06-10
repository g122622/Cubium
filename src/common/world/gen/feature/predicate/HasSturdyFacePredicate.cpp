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
 */

#include "HasSturdyFacePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc::world::gen::feature::predicate {

bool HasSturdyFacePredicate::test(const IWorld& world, const BlockPos& pos) const
{
    BlockPos checkPos = pos + m_offset;
    const BlockState* state = world.getBlockState(checkPos);
    if (state == nullptr) {
        return false;
    }
    // hasSturdyFace 检查方块自身某面是否坚固
    // isSolidSide 检查的是从 side 方向看该面是否坚固，因此需要取反方向
    return state->isSolidSide(const_cast<IWorld&>(world), checkPos, Directions::opposite(m_direction));
}

} // namespace mc::world::gen::feature::predicate
