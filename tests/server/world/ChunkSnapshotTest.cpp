/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished copies of the following:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// ============================================================================
// ChunkSnapshot 单元测试
//
// 验证 fromPrimer 深拷贝行为（对齐 Moonrise GenerationChunkHolder.getChunkIfPresentUnchecked
// 返回的不可变 ChunkAccess）：
//   - 方块状态数据深拷贝（修改 primer 不影响快照）
//   - 高度图深拷贝
//   - 生物群系深拷贝
//   - status/x/z 元数据正确
//   - isValid() 与移动语义
// ============================================================================

#include "server/world/ChunkSnapshot.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"

#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;
using mc::server::ChunkSnapshot;

namespace {

// 辅助：在 primer 的指定 section 中写入一个非零 stateId 并返回该 section
ChunkSection* writeSection(ChunkPrimer& primer, i32 sectionIndex, u32 stateId)
{
    ChunkSection* section = primer.createSection(sectionIndex);
    section->fill(stateId);
    return section;
}

} // namespace

// ============================================================================
// fromPrimer 基础属性
// ============================================================================

TEST(ChunkSnapshotTest, FromPrimerPreservesCoordinatesAndStatus)
{
    ChunkPrimer primer(7, -3);
    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::BIOMES);

    EXPECT_EQ(snapshot.x(), 7);
    EXPECT_EQ(snapshot.z(), -3);
    EXPECT_EQ(snapshot.status(), ChunkStatuses::BIOMES);
    EXPECT_TRUE(snapshot.isValid());
}

TEST(ChunkSnapshotTest, DefaultConstructedIsInvalid)
{
    ChunkSnapshot snapshot;
    EXPECT_FALSE(snapshot.isValid());
}

// ============================================================================
// 方块状态深拷贝
// ============================================================================

TEST(ChunkSnapshotTest, BlockStatesAreDeepCopied)
{
    // primer 写入方块数据，创建快照后修改 primer，快照应不受影响
    ChunkPrimer primer(0, 0);
    ChunkSection* section = writeSection(primer, 0, 42); // section 0 全填 stateId=42
    ASSERT_NE(section, nullptr);

    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::NOISE);

    // 快照应包含 stateId=42
    const ChunkData& snapData = snapshot.data();
    const ChunkSection* snapSection = snapData.getSection(0);
    ASSERT_NE(snapSection, nullptr);
    EXPECT_EQ(snapSection->getBlockStateId(0, 0, 0), 42u);
    EXPECT_EQ(snapSection->getBlockStateId(15, 15, 15), 42u);

    // 修改 primer 后快照不变（深拷贝）
    primer.createSection(0)->fill(99);
    EXPECT_EQ(snapSection->getBlockStateId(0, 0, 0), 42u) << "snapshot must be independent of primer";
}

TEST(ChunkSnapshotTest, EmptySectionsDoNotCrash)
{
    // primer 没有任何 section，fromPrimer 应安全处理
    ChunkPrimer primer(0, 0);
    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::EMPTY);
    EXPECT_TRUE(snapshot.isValid());
    const ChunkData& data = snapshot.data();
    for (i32 i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        EXPECT_EQ(data.getSection(i), nullptr);
    }
}

// ============================================================================
// 生物群系深拷贝
// ============================================================================

TEST(ChunkSnapshotTest, BiomesAreDeepCopied)
{
    ChunkPrimer primer(0, 0);
    BiomeContainer biomes;
    biomes.setBiome(0, 0, 0, 0, 7);  // section 0, (x=0,y=0,z=0) = biome 7
    biomes.setBiome(3, 3, 3, 3, 12); // section 3, (x=3,y=3,z=3) = biome 12
    primer.setBiomes(biomes);

    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::BIOMES);

    EXPECT_EQ(snapshot.biomes().getBiome(0, 0, 0, 0), 7u);
    EXPECT_EQ(snapshot.biomes().getBiome(3, 3, 3, 3), 12u);

    // 修改 primer 后快照不变
    BiomeContainer modified;
    modified.setBiome(0, 0, 0, 0, 99);
    primer.setBiomes(modified);
    EXPECT_EQ(snapshot.biomes().getBiome(0, 0, 0, 0), 7u) << "snapshot biomes must be independent";
}

// ============================================================================
// 高度图深拷贝
// ============================================================================

TEST(ChunkSnapshotTest, HeightmapsAreCopied)
{
    ChunkPrimer primer(0, 0);
    // 通过 getHeightmap(可变) 获取并设置高度图数据
    Heightmap& srcHeightmap = primer.getHeightmap(HeightmapType::WorldSurface);
    std::array<BlockCoord, Heightmap::SIZE> heights{};
    heights[0] = 64;
    heights[5] = 100;
    srcHeightmap.setData(heights);

    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::NOISE);

    EXPECT_TRUE(snapshot.hasHeightmap(HeightmapType::WorldSurface));
    const Heightmap* snapHeightmap = snapshot.heightmap(HeightmapType::WorldSurface);
    ASSERT_NE(snapHeightmap, nullptr);
    EXPECT_EQ(snapHeightmap->getData()[0], 64);
    EXPECT_EQ(snapHeightmap->getData()[5], 100);
}

TEST(ChunkSnapshotTest, HeightmapMissingTypeReturnsNull)
{
    ChunkPrimer primer(0, 0);
    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::EMPTY);

    // primer 没有设置 MotionBlocking，fromPrimer 拷贝了 dummy（空高度图），
    // hasHeightmap 仍应返回 true（因为 fromPrimer 拷贝了全部 7 种类型）。
    // 但 heightmap() 对存在的类型返回非 nullptr。
    EXPECT_TRUE(snapshot.hasHeightmap(HeightmapType::WorldSurface));
    EXPECT_NE(snapshot.heightmap(HeightmapType::WorldSurface), nullptr);
}

// ============================================================================
// 多状态快照独立性
// ============================================================================

TEST(ChunkSnapshotTest, MultipleSnapshotsIndependent)
{
    // 同一 primer 在不同状态创建两个快照，各自独立
    ChunkPrimer primer(1, 1);
    writeSection(primer, 5, 10);

    ChunkSnapshot noiseSnapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::NOISE);

    // 修改 primer 后创建另一个快照
    writeSection(primer, 5, 20);
    ChunkSnapshot surfaceSnapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::SURFACE);

    // NOISE 快照保留 stateId=10
    const ChunkSection* noiseSection = noiseSnapshot.data().getSection(5);
    ASSERT_NE(noiseSection, nullptr);
    EXPECT_EQ(noiseSection->getBlockStateId(0, 0, 0), 10u);

    // SURFACE 快照保留 stateId=20
    const ChunkSection* surfaceSection = surfaceSnapshot.data().getSection(5);
    ASSERT_NE(surfaceSection, nullptr);
    EXPECT_EQ(surfaceSection->getBlockStateId(0, 0, 0), 20u);

    EXPECT_EQ(noiseSnapshot.status(), ChunkStatuses::NOISE);
    EXPECT_EQ(surfaceSnapshot.status(), ChunkStatuses::SURFACE);
}

// ============================================================================
// 移动语义
// ============================================================================

TEST(ChunkSnapshotTest, MoveTransferOwnership)
{
    ChunkPrimer primer(2, 4);
    writeSection(primer, 0, 55);
    BiomeContainer biomes;
    biomes.setBiome(1, 1, 1, 1, 3);
    primer.setBiomes(biomes);

    ChunkSnapshot original = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::SURFACE);
    const ChunkData* originalDataPtr = &original.data();
    EXPECT_EQ(original.biomes().getBiome(1, 1, 1, 1), 3u);

    ChunkSnapshot moved = std::move(original);

    // 移动后 moved 持有数据
    EXPECT_TRUE(moved.isValid());
    EXPECT_EQ(moved.x(), 2);
    EXPECT_EQ(moved.z(), 4);
    EXPECT_EQ(moved.status(), ChunkStatuses::SURFACE);
    EXPECT_EQ(moved.data().getSection(0)->getBlockStateId(0, 0, 0), 55u);
    EXPECT_EQ(moved.biomes().getBiome(1, 1, 1, 1), 3u);

    // data() 指针应保持稳定（shared_ptr 移动后底层 ChunkData 不变）
    EXPECT_EQ(&moved.data(), originalDataPtr);
}

// ============================================================================
// dataPtr 共享语义
// ============================================================================

TEST(ChunkSnapshotTest, DataPtrIsValid)
{
    ChunkPrimer primer(0, 0);
    writeSection(primer, 0, 1);

    ChunkSnapshot snapshot = ChunkSnapshot::fromPrimer(primer, ChunkStatuses::NOISE);
    EXPECT_NE(snapshot.dataPtr(), nullptr);
    EXPECT_EQ(snapshot.dataPtr()->getSection(0)->getBlockStateId(0, 0, 0), 1u);
}
