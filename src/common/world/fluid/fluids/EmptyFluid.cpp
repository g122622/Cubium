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

#include "EmptyFluid.hpp"
#include "common/core/Types.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <utility>
#include <vector>

namespace mc {
namespace fluid {

EmptyFluid::EmptyFluid()
{
    // 空流体没有任何属性，创建空的状态容器
    auto container = StateContainer<Fluid, FluidState>::Builder(*this).create(
        [this](const Fluid& fluid,
            auto values,
            const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
            const std::vector<FluidState*>* allStates,
            u32 id) { return std::make_unique<FluidState>(fluid, std::move(values), propertyLayouts, allStates, id); });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

const BlockState* EmptyFluid::getBlockState(const FluidState& state) const
{
    (void)state;
    // 空流体对应的方块是空气
    if (VanillaBlocks::AIR != nullptr) {
        return &VanillaBlocks::AIR->defaultState();
    }
    return nullptr;
}

} // namespace fluid
} // namespace mc
