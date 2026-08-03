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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "DisabledAquifer.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include "common/world/gen/aquifer/FluidStatus.hpp"

namespace mc::world::gen::aquifer {

const BlockState* DisabledAquifer::computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue)
{
    if (densityValue > 0.0) {
        return nullptr; // 固体方块
    }

    FluidStatus fluid = m_globalFluidPicker(blockX, blockY, blockZ);
    return fluid.at(blockY);
}

} // namespace mc::world::gen::aquifer
