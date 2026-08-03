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

#include "MegaPineFoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace mc {

MegaPineFoliagePlacer::MegaPineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{}

i32 MegaPineFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const
{
    return m_height + random.nextInt(4);
}

void MegaPineFoliagePlacer::placeFoliageInternal(WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 巨型松树：更大的锥形
    for (i32 y = 0; y <= foliageHeight; ++y) {
        // 从底部到顶部半径逐渐减小
        i32 layerRadius = std::max(1, radius - y / 3);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool MegaPineFoliagePlacer::shouldSkip(
    math::Random& random, i32 dx, i32 dy, i32 dz, i32 radius, bool /*trunkTop*/) const
{
    // 第一个条件：曼哈顿距离 >= 7 时跳过
    if (std::abs(dx) + std::abs(dz) >= 7) {
        return true;
    }

    // 第二个条件：使用欧几里得距离平方比较
    return dx * dx + dz * dz > radius * radius;
}

std::unique_ptr<FoliagePlacer> MegaPineFoliagePlacer::clone() const
{
    return std::make_unique<MegaPineFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
