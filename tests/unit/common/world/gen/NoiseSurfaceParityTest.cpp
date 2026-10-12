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

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/SurfaceTestFixture.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {
namespace {

// ============================================================================
// NoiseSurfaceParityTest
//
// 验证主世界草原（Plains）表面规则：顶层 grass_block，其下若干层为 dirt。
//
// 夹具先建立真实 NoiseChunk，再用固定草原与石质平台执行地表构建。
// ============================================================================

class NoiseSurfaceParityTest : public ::testing::Test {};

// 固定草原与石质平台，只验证草方块/泥土地表契约，不把气候采样或海岸位置作为测试前提。
TEST_F(NoiseSurfaceParityTest, PlainsSurfaceUsesDirtUnderTopLayer)
{
    constexpr u64 seed = 42;
    auto result = mc::test::makeSurfaceTestFixture(seed, Biomes::Plains);
    ASSERT_NE(result, nullptr);
    ASSERT_NE(result->center, nullptr);

    const auto& chunk = *result->center;
    WorldGenRegion& region = *result->region;

    i32 checkedColumns = 0;
    i32 plainsColumns = 0;
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 topY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (topY < 1) {
                continue;
            }

            const BlockState* topState = chunk.getBlockState(x, topY, z);
            if (topState == nullptr || !topState->is(VanillaBlocks::GRASS_BLOCK)) {
                continue;
            }

            // 草原表面规则契约：顶层 grass_block 下方应为 dirt（默认 underFloor 兜底）。
            ++checkedColumns;
            const BlockState* belowTop = chunk.getBlockState(x, topY - 1, z);
            ASSERT_NE(belowTop, nullptr);
            EXPECT_TRUE(belowTop->is(VanillaBlocks::DIRT))
                << "grass_block at (" << x << ", " << topY << ", " << z << ") should have dirt below";

            // 统计 Plains 群系列（草原为该默认规则的典型代表）
            const BiomeId biomeId =
                region.getBiome(chunk.x() * world::CHUNK_WIDTH + x, topY, chunk.z() * world::CHUNK_WIDTH + z);
            if (biomeId == Biomes::Plains) {
                ++plainsColumns;
            }
        }
    }

    // 已固定草原与干燥平台，必须实际命中草方块列。
    EXPECT_EQ(checkedColumns, world::CHUNK_WIDTH * world::CHUNK_WIDTH);
    EXPECT_EQ(plainsColumns, checkedColumns);
}

} // namespace
} // namespace mc
