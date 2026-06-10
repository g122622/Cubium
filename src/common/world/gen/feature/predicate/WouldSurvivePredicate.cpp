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

#include "WouldSurvivePredicate.hpp"
#include "common/world/IWorld.hpp"

namespace mc::world::gen::feature::predicate {

bool WouldSurvivePredicate::test(const IWorld& world, const BlockPos& pos) const
{
    if (m_state == nullptr) {
        return false;
    }
    BlockPos checkPos = pos + m_offset;
    // isValidPosition 接收 IBlockReader&（继承自 IWorld），需要 const_cast + static_cast
    auto& blockReader = static_cast<IBlockReader&>(const_cast<IWorld&>(world));
    return m_state->getBlock().isValidPosition(*m_state, blockReader, checkPos);
}

} // namespace mc::world::gen::feature::predicate
