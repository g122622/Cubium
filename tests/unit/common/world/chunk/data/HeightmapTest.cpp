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

#include "common/world/chunk/data/Heightmap.hpp"

#include "common/world/block/registry/VanillaBlocks.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <vector>

namespace mc::world::chunk {
namespace {

class HeightmapStorageTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

// ============================================================================
// 位存储布局
// ============================================================================

/**
 * 位宽必须覆盖全部合法高度值。
 *
 * 主世界 MIN_BUILD_HEIGHT=-64、MAX_BUILD_HEIGHT=320：内部存储值（Y+1）范围
 * [-63, 320]，编码后（raw - NO_BLOCK_SENTINEL）范围 [0, 385]，故 BITS=9、
 * 每字 7 个条目、37 个字。
 */
TEST_F(HeightmapStorageTest, BitLayoutMatchesOverworldHeightLimits)
{
    EXPECT_EQ(Heightmap::NO_BLOCK_SENTINEL, mc::world::MIN_BUILD_HEIGHT - 1);
    constexpr i32 maxEncoded = mc::world::MAX_BUILD_HEIGHT - Heightmap::NO_BLOCK_SENTINEL;
    EXPECT_EQ(Heightmap::BITS, 9);
    EXPECT_EQ(Heightmap::VALUES_PER_LONG, 7);
    EXPECT_EQ(Heightmap::WORD_COUNT, 37);
    EXPECT_EQ(Heightmap::VALUE_MASK, 0x1FFULL);
    EXPECT_LE(static_cast<u64>(maxEncoded), Heightmap::VALUE_MASK) << "位宽不足以表示最高方块";
}

/**
 * 单张高度图的字节数必须远小于扁平的 256 × i32。
 *
 * 这是本次压缩的全部意义：7 张高度图合计 2072 B/区块（原 7196 B）。
 */
TEST_F(HeightmapStorageTest, SizeIsCompressedNotFlat)
{
    constexpr size_t FLAT_BYTES = Heightmap::SIZE * sizeof(BlockCoord); // 1028
    const size_t packedBytes = Heightmap::WORD_COUNT * sizeof(u64);     // 296
    EXPECT_LT(packedBytes, FLAT_BYTES);
    EXPECT_EQ(packedBytes, 296u);
    EXPECT_EQ(Heightmap::WORD_COUNT * sizeof(u64) * HEIGHTMAP_TYPE_COUNT, 2072u);
}

// ============================================================================
// 默认值语义
// ============================================================================

/**
 * 默认构造的列全部为"无方块"。
 *
 * 位存储默认全 0，而编码 0 恰好是 NO_BLOCK_SENTINEL —— 这个巧合是刻意的：
 * 它让"零初始化"与"无方块"天然等价，省掉构造期的 256 次写入。
 */
TEST_F(HeightmapStorageTest, DefaultConstructedColumnsAreEmpty)
{
    const Heightmap heightmap(HeightmapType::WorldSurface);
    for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
            EXPECT_EQ(heightmap.getHeight(x, z), Heightmap::NO_BLOCK_SENTINEL)
                << "(" << x << ", " << z << ") 应为无方块";
        }
    }
}

/**
 * 哨兵与"MIN_BUILD_HEIGHT 处有方块"必须可区分。
 *
 * 这是 NO_BLOCK_SENTINEL 取 MIN_BUILD_HEIGHT - 1（而非 0）的全部理由。编码方案
 * 把两者分别映射到 0 与 1，位存储因此不会把二者混为一谈。
 */
TEST_F(HeightmapStorageTest, SentinelDistinctFromMinBuildHeightBlock)
{
    Heightmap heightmap(HeightmapType::WorldSurface);

    // MIN_BUILD_HEIGHT 处的方块 → 内部值 MIN_BUILD_HEIGHT + 1
    heightmap.setHeight(0, 0, mc::world::MIN_BUILD_HEIGHT + 1);
    EXPECT_EQ(heightmap.getHeight(0, 0), mc::world::MIN_BUILD_HEIGHT + 1);
    EXPECT_NE(heightmap.getHeight(0, 0), Heightmap::NO_BLOCK_SENTINEL);

    // 相邻列未设置 → 哨兵。二者必须不等。
    EXPECT_EQ(heightmap.getHeight(1, 0), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_NE(heightmap.getHeight(0, 0), heightmap.getHeight(1, 0));
}

// ============================================================================
// 读写往返
// ============================================================================

/**
 * 全部 256 列写入互不相同的值后逐列读回，验证位打包不串位。
 *
 * 值集刻意覆盖每个字内的首尾槽位（index 0、6、7、13……）与跨字边界，
 * 以及编码范围的两端（1 与最大值）。
 */
TEST_F(HeightmapStorageTest, SetGetRoundTripAllColumns)
{
    Heightmap heightmap(HeightmapType::WorldSurface);

    constexpr BlockCoord MIN_RAW = mc::world::MIN_BUILD_HEIGHT + 1;
    constexpr BlockCoord MAX_RAW = mc::world::MAX_BUILD_HEIGHT;
    constexpr i32 SPAN = MAX_RAW - MIN_RAW + 1;

    std::vector<BlockCoord> expected(static_cast<size_t>(Heightmap::SIZE));
    for (i32 index = 0; index < Heightmap::SIZE; ++index) {
        const BlockCoord raw = MIN_RAW + (index * 7 + 3) % SPAN;
        expected[static_cast<size_t>(index)] = raw;
        heightmap.setHeight(index % mc::world::CHUNK_WIDTH, index / mc::world::CHUNK_WIDTH, raw);
    }

    for (i32 index = 0; index < Heightmap::SIZE; ++index) {
        EXPECT_EQ(heightmap.getHeight(index % mc::world::CHUNK_WIDTH, index / mc::world::CHUNK_WIDTH),
            expected[static_cast<size_t>(index)])
            << "index " << index << " 串位";
    }
}

/**
 * 边界值：编码范围的两端（1 与 385）必须可无损存取。
 *
 * 385 是 MAX_BUILD_HEIGHT(320) - NO_BLOCK_SENTINEL(-65) = 385，即位宽 9 恰好能
 * 表示的最大值。若 BITS 少一位，最高层的方块会被静默截断成低层高度。
 */
TEST_F(HeightmapStorageTest, BoundaryValuesSurviveRoundTrip)
{
    Heightmap heightmap(HeightmapType::WorldSurface);

    constexpr BlockCoord LOWEST_RAW = mc::world::MIN_BUILD_HEIGHT + 1; // 编码 1
    constexpr BlockCoord HIGHEST_RAW = mc::world::MAX_BUILD_HEIGHT;    // 编码 385

    heightmap.setHeight(0, 0, LOWEST_RAW);
    heightmap.setHeight(15, 15, HIGHEST_RAW);
    heightmap.setHeight(7, 8, Heightmap::NO_BLOCK_SENTINEL);

    EXPECT_EQ(heightmap.getHeight(0, 0), LOWEST_RAW);
    EXPECT_EQ(heightmap.getHeight(15, 15), HIGHEST_RAW);
    EXPECT_EQ(heightmap.getHeight(7, 8), Heightmap::NO_BLOCK_SENTINEL);
}

/**
 * 越界值夹到边界，而不是回绕成另一个合法高度。
 *
 * 高度值可能来自存档/网络的不可信字节，位存储无法表示越界值。回绕会把
 * "不可能的高度"变成"另一个看似合理的高度"，静默污染地形；夹取至少保持在
 * 合法域内且可被上层察觉。
 */
TEST_F(HeightmapStorageTest, OutOfRangeValuesClampInsteadOfWrap)
{
    Heightmap heightmap(HeightmapType::WorldSurface);

    heightmap.setHeight(0, 0, std::numeric_limits<BlockCoord>::max());
    heightmap.setHeight(1, 0, std::numeric_limits<BlockCoord>::min());
    heightmap.setHeight(2, 0, Heightmap::NO_BLOCK_SENTINEL - 1000);
    heightmap.setHeight(3, 0, mc::world::MAX_BUILD_HEIGHT + 1000);

    EXPECT_EQ(heightmap.getHeight(0, 0), mc::world::MAX_BUILD_HEIGHT);
    EXPECT_EQ(heightmap.getHeight(1, 0), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(2, 0), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(3, 0), mc::world::MAX_BUILD_HEIGHT);
}

// ============================================================================
// update 语义
// ============================================================================

/**
 * update 只在"新方块不低于当前高度"时写入。
 *
 * 高度图存的是最高方块，故从下往上放置方块时只有更高的那个生效；先放高的再放低的
 * 不应把高度改低。
 */
TEST_F(HeightmapStorageTest, UpdateKeepsHighestBlock)
{
    Heightmap heightmap(HeightmapType::WorldSurface);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    EXPECT_TRUE(heightmap.update(0, 60, 0, stone));
    EXPECT_EQ(heightmap.getHeight(0, 0), 61);

    // 更低处放方块：不改变高度
    EXPECT_FALSE(heightmap.update(0, 40, 0, stone));
    EXPECT_EQ(heightmap.getHeight(0, 0), 61);

    // 更高处放方块：抬高高度
    EXPECT_TRUE(heightmap.update(0, 90, 0, stone));
    EXPECT_EQ(heightmap.getHeight(0, 0), 91);

    // 同一高度重复放置：当前高度已是 Y+1=91，y=90 不再满足 y >= currentHeight，
    // 返回 false 且高度不变（与 MC Heightmap.update 的 j >= l 判定一致）
    EXPECT_FALSE(heightmap.update(0, 90, 0, stone));
    EXPECT_EQ(heightmap.getHeight(0, 0), 91);

    // 更高一格：91 >= 91 成立，抬到 92
    EXPECT_TRUE(heightmap.update(0, 91, 0, stone));
    EXPECT_EQ(heightmap.getHeight(0, 0), 92);
}

/**
 * 空气/未计入的方块不改变高度。
 */
TEST_F(HeightmapStorageTest, UpdateIgnoresNonOpaqueBlocks)
{
    Heightmap heightmap(HeightmapType::WorldSurface);

    EXPECT_FALSE(heightmap.update(0, 60, 0, nullptr));
    EXPECT_EQ(heightmap.getHeight(0, 0), Heightmap::NO_BLOCK_SENTINEL);

    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    EXPECT_FALSE(heightmap.update(0, 60, 0, air));
    EXPECT_EQ(heightmap.getHeight(0, 0), Heightmap::NO_BLOCK_SENTINEL);
}

// ============================================================================
// 批量接口
// ============================================================================

/**
 * setData / getData 往返必须逐列一致，且哨兵在两侧都保持语义。
 */
TEST_F(HeightmapStorageTest, SetDataGetDataRoundTrip)
{
    std::array<BlockCoord, Heightmap::SIZE> source{};
    for (i32 index = 0; index < Heightmap::SIZE; ++index) {
        // 交替取哨兵与合法值，确保两条分支都被覆盖
        source[static_cast<size_t>(index)] = (index % 3 == 0)
            ? Heightmap::NO_BLOCK_SENTINEL
            : static_cast<BlockCoord>(mc::world::MIN_BUILD_HEIGHT + 1 + index);
    }

    Heightmap heightmap(HeightmapType::OceanFloor);
    heightmap.setData(source);

    const auto roundTripped = heightmap.getData();
    for (i32 index = 0; index < Heightmap::SIZE; ++index) {
        EXPECT_EQ(roundTripped[static_cast<size_t>(index)], source[static_cast<size_t>(index)]) << "index " << index;
    }
}

/**
 * setAll 必须铺满每一个字，包括最后一个字的字尾余位之外的有效槽位。
 *
 * 256 列在 7 条目/字下占 37 个字，最后一个字只用到 4 个槽位（36*7=252，余 4）。
 * 若整字填充时算错槽位数，末尾 4 列会读回 0（= 哨兵）而非设定值。
 */
TEST_F(HeightmapStorageTest, SetAllCoversTrailingPartialWord)
{
    constexpr BlockCoord VALUE = 123;
    Heightmap heightmap(HeightmapType::MotionBlocking);
    heightmap.setAll(VALUE);

    for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
            ASSERT_EQ(heightmap.getHeight(x, z), VALUE) << "(" << x << ", " << z << ") 未被 setAll 覆盖";
        }
    }

    // 末尾 4 列（index 252..255）位于最后一个不完整字，单独确认
    EXPECT_EQ(heightmap.getHeight(252 % 16, 252 / 16), VALUE);
    EXPECT_EQ(heightmap.getHeight(255 % 16, 255 / 16), VALUE);

    // setAll 哨兵同样生效（这是 primeHeightmaps 重置时走的分支）
    heightmap.setAll(Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(255 % 16, 255 / 16), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(0, 0), Heightmap::NO_BLOCK_SENTINEL);
}

/**
 * 越界坐标必须安全返回哨兵/不写入，不得越界访问位存储。
 */
TEST_F(HeightmapStorageTest, OutOfBoundsCoordinatesAreSafe)
{
    Heightmap heightmap(HeightmapType::WorldSurface);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    EXPECT_EQ(heightmap.getHeight(-1, 0), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(0, -1), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(16, 0), Heightmap::NO_BLOCK_SENTINEL);
    EXPECT_EQ(heightmap.getHeight(0, 16), Heightmap::NO_BLOCK_SENTINEL);

    EXPECT_FALSE(heightmap.update(-1, 60, 0, stone));
    EXPECT_FALSE(heightmap.update(16, 60, 0, stone));

    heightmap.setHeight(-1, 0, 60); // 不写入，也不应崩溃
    heightmap.setHeight(16, 0, 60);
    EXPECT_EQ(heightmap.getHeight(0, 0), Heightmap::NO_BLOCK_SENTINEL);
}

} // namespace
} // namespace mc::world::chunk
