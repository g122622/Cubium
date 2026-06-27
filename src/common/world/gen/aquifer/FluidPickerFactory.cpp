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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "FluidPickerFactory.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <limits>

namespace mc::world::gen::aquifer {

FluidPicker createOverworldFluidPicker(i32 seaLevel, const BlockState* defaultFluid)
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

FluidPicker createNetherFluidPicker()
{
    const BlockState* lavaState = &VanillaBlocks::LAVA->defaultState();
    return [lavaState](i32, i32, i32) -> FluidStatus { return {32, lavaState}; };
}

FluidPicker createEndFluidPicker()
{
    // MC 1.21: End 没有流体，FluidStatus 返回空气方块状态
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    return [airState](i32, i32, i32) -> FluidStatus { return {std::numeric_limits<i32>::min(), airState}; };
}

} // namespace mc::world::gen::aquifer
