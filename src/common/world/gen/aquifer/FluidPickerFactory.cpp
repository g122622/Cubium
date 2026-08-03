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

#include "FluidPickerFactory.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/aquifer/FluidStatus.hpp"
#include <algorithm>

namespace mc::world::gen::aquifer {

// 对齐 MC 1.21.11 NoiseBasedChunkGenerator.createFluidPicker：所有维度共用同一选择器。
FluidPicker createFluidPicker(i32 seaLevel, const BlockState* defaultFluid)
{
    const i32 lavaLevel = -54;
    const BlockState* lavaState = &VanillaBlocks::LAVA->defaultState();
    const i32 minFluidLevel = std::min(lavaLevel, seaLevel);

    return [seaLevel, lavaLevel, minFluidLevel, defaultFluid, lavaState](i32, i32 y, i32) -> FluidStatus {
        if (y < minFluidLevel) {
            return {lavaLevel, lavaState};
        }
        return {seaLevel, defaultFluid};
    };
}

} // namespace mc::world::gen::aquifer
