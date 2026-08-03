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

#include "VoidStartPlatformFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <cstdlib>

namespace mc::world::gen::feature {

namespace {

// 平台中心固定偏移 (8,3,8)（MC PLATFORM_OFFSET）。
constexpr i32 PLATFORM_CENTER_X = 8;
constexpr i32 PLATFORM_CENTER_Y = 3;
constexpr i32 PLATFORM_CENTER_Z = 8;
// 平台切比雪夫半径（MC PLATFORM_RADIUS=16）。
constexpr i32 PLATFORM_RADIUS = 16;

/// 切比雪夫距离（MC checkerboardDistance）
[[nodiscard]] i32 checkerboardDistance(i32 x1, i32 z1, i32 x2, i32 z2)
{
    return std::max(std::abs(x1 - x2), std::abs(z1 - z2));
}

} // namespace

bool ConfiguredVoidStartPlatformFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& /*random*/,
    const BlockPos& pos) const
{
    // 当前区块坐标
    const ChunkPos chunkPos(pos);
    // 平台中心所在区块坐标（PLATFORM_OFFSET=(8,3,8) → 区块 (0,0)）
    const ChunkPos platformOriginChunk(BlockPos(PLATFORM_CENTER_X, PLATFORM_CENTER_Y, PLATFORM_CENTER_Z));

    // 若当前区块与平台中心区块切比雪夫距离 >1，直接返回 true（不生成）
    if (checkerboardDistance(chunkPos.x, chunkPos.z, platformOriginChunk.x, platformOriginChunk.z) > 1) {
        return true;
    }

    // 平台 Y = origin.y + PLATFORM_OFFSET.y（MC: PLATFORM_OFFSET.atY(origin.y + PLATFORM_OFFSET.y)）
    const i32 platformY = pos.y + PLATFORM_CENTER_Y;
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);

    // 遍历当前区块的 XZ 方块范围
    for (i32 z = pos.z; z < pos.z + world::CHUNK_WIDTH; ++z) {
        for (i32 x = pos.x; x < pos.x + world::CHUNK_WIDTH; ++x) {
            if (checkerboardDistance(PLATFORM_CENTER_X, PLATFORM_CENTER_Z, x, z) <= PLATFORM_RADIUS) {
                const BlockPos target(x, platformY, z);
                // 中心格放圆石，其余放石头
                const BlockState* state = (x == PLATFORM_CENTER_X && z == PLATFORM_CENTER_Z) ? cobblestone : stone;
                region.setBlockState(target, state);
            }
        }
    }
    return true;
}

} // namespace mc::world::gen::feature
