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

#include "MegaJungleTrunkPlacer.hpp"
#include "GiantTrunkPlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <cmath>
#include <memory>
#include <set>
#include <vector>

namespace mc {

MegaJungleTrunkPlacer::MegaJungleTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{}

std::vector<FoliagePosition> MegaJungleTrunkPlacer::placeTrunk(WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 先调用 GiantTrunkPlacer 放置基础树干和树叶
    foliagePositions = GiantTrunkPlacer(m_baseHeight, m_heightRandA, m_heightRandB)
                           .placeTrunk(world, random, height, startPos, trunkBlocks, trunkBlock);

    // 在树干上生成分支
    // 从 height-2-随机(4) 开始，每隔 2+随机(4) 格，向下到 height/2
    for (i32 branchHeight = height - 2 - random.nextInt(4); branchHeight > height / 2;
        branchHeight -= 2 + random.nextInt(4)) {

        // 随机角度
        f32 angle = random.nextFloat() * 2.0f * math::PI;
        i32 dx = 0;
        i32 dz = 0;

        // 沿着角度方向放置5个方块
        for (i32 step = 0; step < 5; ++step) {
            dx = static_cast<i32>(1.5f + std::cos(angle) * static_cast<f32>(step));
            dz = static_cast<i32>(1.5f + std::sin(angle) * static_cast<f32>(step));

            BlockPos branchPos(startPos.x + dx, startPos.y + branchHeight - 3 + step / 2, startPos.z + dz);

            // 放置分支方块（使用2x2方式）
            placeTrunkLayer2x2(world, branchPos, trunkBlocks, trunkBlock);
        }

        // 添加分支末端的树叶位置
        foliagePositions.emplace_back(BlockPos(startPos.x + dx, startPos.y + branchHeight, startPos.z + dz), -2, false);
    }

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> MegaJungleTrunkPlacer::clone() const
{
    return std::make_unique<MegaJungleTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
