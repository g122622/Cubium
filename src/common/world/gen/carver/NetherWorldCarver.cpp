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

#include "NetherWorldCarver.hpp"
#include "CarvingContext.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/carver/CarverConfiguration.hpp"
#include "common/world/gen/carver/CaveCarver.hpp"

namespace mc {

// 下界熔岩填充高度偏移：Y <= minY + 31 填充熔岩
constexpr i32 NETHER_LAVA_LEVEL_OFFSET = 31;

// ============================================================================
// NetherWorldCarver 实现
// ============================================================================

NetherWorldCarver::NetherWorldCarver()
    : CaveCarver()
{}

f32 NetherWorldCarver::getThickness(math::IRandom& rng) const
{
    return (rng.nextFloat() * 2.0f + rng.nextFloat()) * 2.0f;
}

const BlockState* NetherWorldCarver::getCarveState(
    CarvingContext& context, i32 /*worldX*/, i32 worldY, i32 /*worldZ*/, const CaveCarverConfiguration& config) const
{
    if (worldY <= context.getMinGenY() + NETHER_LAVA_LEVEL_OFFSET) {
        return VanillaBlocks::getState(VanillaBlocks::LAVA);
    }
    return getCaveAirState();
}

} // namespace mc
