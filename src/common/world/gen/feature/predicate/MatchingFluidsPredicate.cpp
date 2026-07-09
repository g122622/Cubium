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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MatchingFluidsPredicate.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc::world::gen::feature::predicate {

bool MatchingFluidsPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockPos target = pos + m_offset;
    const BlockState* state = world.getBlockState(target);

    // 取目标位置的流体：空气（nullptr BlockState）或流体状态为空 → 视为 EmptyFluid。
    const fluid::Fluid* fluid = nullptr;
    bool isEmpty = true;
    if (state != nullptr) {
        if (const fluid::FluidState* fluidState = state->getFluidState();
            fluidState != nullptr && !fluidState->isEmpty()) {
            fluid = &fluidState->getFluid();
            isEmpty = false;
        }
    }

    for (const fluid::Fluid* candidate : m_fluids) {
        // candidate 为空流体（isEmpty()=true）时匹配空气/空流体位置。
        if (candidate == nullptr || candidate->isEmpty()) {
            if (isEmpty) {
                return true;
            }
            continue;
        }
        if (!isEmpty && fluid != nullptr && fluid->fluidLocation() == candidate->fluidLocation()) {
            return true;
        }
    }
    if (!isEmpty && fluid != nullptr) {
        for (const fluid::FluidTag* tag : m_tags) {
            if (tag != nullptr && tag->contains(*fluid)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace mc::world::gen::feature::predicate
