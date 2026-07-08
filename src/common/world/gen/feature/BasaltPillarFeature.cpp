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

#include "BasaltPillarFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

#include <array>

namespace mc::world::gen::feature {

namespace {

/// 判断格子是否为空（空气）
[[nodiscard]] bool isEmptyBlock(WorldGenRegion& region, const BlockPos& pos)
{
    const BlockState* state = region.getBlockState(pos);
    return state == nullptr || state->isAir();
}

/// 在垂挂位置随机放置 BASALT（nextInt(10)!=0 放置并继续，==0 停止该方向垂挂）
[[nodiscard]] bool placeHangOff(WorldGenRegion& region, math::Random& random, const BlockPos& pos)
{
    if (random.nextInt(10) != 0) {
        region.setBlockState(pos, VanillaBlocks::getState(VanillaBlocks::BASALT));
        return true;
    }
    return false;
}

/// 柱底基座垂挂：nextBoolean() 为真则放 BASALT
void placeBaseHangOff(WorldGenRegion& region, math::Random& random, const BlockPos& pos)
{
    if (random.nextBoolean()) {
        region.setBlockState(pos, VanillaBlocks::getState(VanillaBlocks::BASALT));
    }
}

} // namespace

bool ConfiguredBasaltPillarFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    // MC: 仅当 origin 为空且其上方非空时生成
    if (!isEmptyBlock(region, origin) || isEmptyBlock(region, origin.up())) {
        return false;
    }

    const BlockState* basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    BlockPosMutable pillarPos(origin);
    BlockPosMutable hangPos(origin);
    const std::array<Direction, 4> horizontal = {Direction::North, Direction::South, Direction::West, Direction::East};
    std::array<bool, 4> hang = {true, true, true, true};

    // 向下逐格放 BASALT，每格在四方向尝试垂挂
    while (isEmptyBlock(region, pillarPos)) {
        if (!region.isWithinWorldBounds(pillarPos.x, pillarPos.y, pillarPos.z)) {
            return true;
        }
        region.setBlockState(pillarPos, basalt);
        for (size_t d = 0; d < horizontal.size(); ++d) {
            if (hang[d]) {
                hangPos.set(pillarPos).move(horizontal[d]);
                hang[d] = placeHangOff(region, random, hangPos);
            }
        }
        pillarPos.move(Direction::Down);
    }

    // 柱底上方一格周围放基座垂挂
    pillarPos.move(Direction::Up);
    for (Direction dir : horizontal) {
        hangPos.set(pillarPos).move(dir);
        placeBaseHangOff(region, random, hangPos);
    }
    pillarPos.move(Direction::Down);

    // 柱底下方 7x7 区域散落 BASALT
    BlockPosMutable scatterPos;
    BlockPosMutable probePos;
    for (i32 i = -3; i < 4; ++i) {
        for (i32 j = -3; j < 4; ++j) {
            const i32 k = std::abs(i) * std::abs(j);
            if (random.nextInt(10) < 10 - k) {
                scatterPos.set(pillarPos.x + i, pillarPos.y, pillarPos.z + j);
                i32 l = 3;
                // 向下找支撑：只要下方为空就继续下移，最多 3 格
                probePos.set(scatterPos).move(Direction::Down);
                while (isEmptyBlock(region, probePos)) {
                    scatterPos.move(Direction::Down);
                    probePos.set(scatterPos).move(Direction::Down);
                    if (--l <= 0) {
                        break;
                    }
                }
                // 下方非空则放 BASALT
                probePos.set(scatterPos).move(Direction::Down);
                if (!isEmptyBlock(region, probePos)) {
                    region.setBlockState(scatterPos, basalt);
                }
            }
        }
    }

    return true;
}

} // namespace mc::world::gen::feature
