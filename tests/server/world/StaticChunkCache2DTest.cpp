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
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "server/world/StaticChunkCache2D.hpp"

#include <string>
#include <gtest/gtest.h>

using mc::server::StaticChunkCache2D;

namespace {
// 简单条目类型：记录坐标
struct Entry {
    mc::ChunkCoord x;
    mc::ChunkCoord z;
    std::string label;
};
} // namespace

// ============================================================================
// 构造与填充
// ============================================================================

TEST(StaticChunkCache2DTest, ConstructorFillsAllEntries)
{
    StaticChunkCache2D<Entry> cache(0, 0, 2, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, "entry"}; });

    // 直径 = 2*2+1 = 5，共 25 个条目
    EXPECT_EQ(cache.diameter(), 5);
    EXPECT_EQ(cache.size(), 25u);
    EXPECT_EQ(cache.radius(), 2);
    EXPECT_EQ(cache.centerX(), 0);
    EXPECT_EQ(cache.centerZ(), 0);

    // 每个位置都被填充
    for (mc::ChunkCoord dz = -2; dz <= 2; ++dz) {
        for (mc::ChunkCoord dx = -2; dx <= 2; ++dx) {
            const Entry& e = cache.get(dx, dz);
            EXPECT_EQ(e.x, dx);
            EXPECT_EQ(e.z, dz);
            EXPECT_EQ(e.label, "entry");
        }
    }
}

TEST(StaticChunkCache2DTest, ConstructorWithNonZeroCenter)
{
    StaticChunkCache2D<Entry> cache(10, -5, 1, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, "ok"}; });

    EXPECT_EQ(cache.centerX(), 10);
    EXPECT_EQ(cache.centerZ(), -5);
    EXPECT_EQ(cache.diameter(), 3);
    EXPECT_EQ(cache.size(), 9u);

    // 中心
    EXPECT_EQ(cache.get(10, -5).x, 10);
    // 四角
    EXPECT_EQ(cache.get(9, -6).x, 9);
    EXPECT_EQ(cache.get(11, -4).z, -4);
}

TEST(StaticChunkCache2DTest, RadiusZeroIsSingleChunk)
{
    StaticChunkCache2D<Entry> cache(7, 7, 0, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, "only"}; });

    EXPECT_EQ(cache.diameter(), 1);
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_EQ(cache.get(7, 7).label, "only");
}

// ============================================================================
// inBounds
// ============================================================================

TEST(StaticChunkCache2DTest, InBoundsCheck)
{
    StaticChunkCache2D<Entry> cache(0, 0, 3, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, ""}; });

    EXPECT_TRUE(cache.inBounds(0, 0));
    EXPECT_TRUE(cache.inBounds(3, 3));
    EXPECT_TRUE(cache.inBounds(-3, -3));
    EXPECT_TRUE(cache.inBounds(3, -3));
    EXPECT_TRUE(cache.inBounds(-3, 3));

    // 越界（Chebyshev 距离 > 3）
    EXPECT_FALSE(cache.inBounds(4, 0));
    EXPECT_FALSE(cache.inBounds(0, 4));
    EXPECT_FALSE(cache.inBounds(-4, 0));
    EXPECT_FALSE(cache.inBounds(0, -4));
    EXPECT_FALSE(cache.inBounds(4, 4));
}

TEST(StaticChunkCache2DTest, InBoundsWithNonZeroCenter)
{
    StaticChunkCache2D<Entry> cache(100, 200, 2, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, ""}; });

    EXPECT_TRUE(cache.inBounds(100, 200));
    EXPECT_TRUE(cache.inBounds(102, 202));
    EXPECT_TRUE(cache.inBounds(98, 198));
    EXPECT_FALSE(cache.inBounds(103, 200));
    EXPECT_FALSE(cache.inBounds(100, 203));
}

// ============================================================================
// loader 按坐标返回不同值
// ============================================================================

TEST(StaticChunkCache2DTest, LoaderReceivesCorrectCoordinates)
{
    StaticChunkCache2D<mc::i32> cache(0, 0, 1, [](mc::ChunkCoord x, mc::ChunkCoord z) {
        // 用 x*100+z 编码坐标，验证 loader 收到的坐标正确
        return static_cast<mc::i32>(x * 100 + z);
    });

    EXPECT_EQ(cache.get(0, 0), 0);
    EXPECT_EQ(cache.get(1, 0), 100);
    EXPECT_EQ(cache.get(0, 1), 1);
    EXPECT_EQ(cache.get(-1, 0), -100);
    EXPECT_EQ(cache.get(0, -1), -1);
    EXPECT_EQ(cache.get(1, 1), 101);
    EXPECT_EQ(cache.get(-1, -1), -101);
}

// ============================================================================
// shared_ptr 条目（模拟 ChunkSnapshot 共享语义）
// ============================================================================

TEST(StaticChunkCache2DTest, SharedPtrEntriesShareUnderlying)
{
    // 模拟 ChunkSnapshot 共享：多个位置返回同一个 shared_ptr
    auto shared = std::make_shared<Entry>(Entry{0, 0, "shared"});
    StaticChunkCache2D<std::shared_ptr<Entry>> cache(0, 0, 1, [&](mc::ChunkCoord, mc::ChunkCoord) { return shared; });

    // 所有位置指向同一对象
    for (mc::ChunkCoord dz = -1; dz <= 1; ++dz) {
        for (mc::ChunkCoord dx = -1; dx <= 1; ++dx) {
            EXPECT_EQ(cache.get(dx, dz).get(), shared.get());
        }
    }
    EXPECT_EQ(shared.use_count(), 10); // 1 (shared) + 9 (cache)
}

// ============================================================================
// Move 语义
// ============================================================================

TEST(StaticChunkCache2DTest, MoveConstruction)
{
    StaticChunkCache2D<Entry> cache0(0, 0, 1, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, "moved"}; });

    StaticChunkCache2D<Entry> cache1(std::move(cache0));

    EXPECT_EQ(cache1.radius(), 1);
    EXPECT_EQ(cache1.size(), 9u);
    EXPECT_EQ(cache1.get(0, 0).label, "moved");
    EXPECT_EQ(cache1.get(1, 1).x, 1);
}

// ============================================================================
// 大半径缓存（验证索引计算正确性）
// ============================================================================

TEST(StaticChunkCache2DTest, LargeRadiusIndexCorrectness)
{
    // 半径 8（对齐 STRUCTURE_REFERENCES 的邻居半径），共 17*17=289 个条目
    StaticChunkCache2D<Entry> cache(-7, -2, 8, [](mc::ChunkCoord x, mc::ChunkCoord z) { return Entry{x, z, ""}; });

    EXPECT_EQ(cache.diameter(), 17);
    EXPECT_EQ(cache.size(), 289u);

    // 验证四角和中心
    EXPECT_EQ(cache.get(-7, -2).x, -7); // 中心
    EXPECT_EQ(cache.get(-7, -2).z, -2);
    EXPECT_EQ(cache.get(-15, -10).x, -15); // 左下角
    EXPECT_EQ(cache.get(-15, -10).z, -10);
    EXPECT_EQ(cache.get(1, 6).x, 1); // 右上角
    EXPECT_EQ(cache.get(1, 6).z, 6);

    // 全量验证：每个位置的坐标与预期一致
    for (mc::ChunkCoord dz = -8; dz <= 8; ++dz) {
        for (mc::ChunkCoord dx = -8; dx <= 8; ++dx) {
            const Entry& e = cache.get(-7 + dx, -2 + dz);
            EXPECT_EQ(e.x, -7 + dx) << "dx=" << dx << " dz=" << dz;
            EXPECT_EQ(e.z, -2 + dz) << "dx=" << dx << " dz=" << dz;
        }
    }
}
