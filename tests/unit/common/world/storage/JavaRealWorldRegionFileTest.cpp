/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/ sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// RegionFile 读取真实 .mca 区域文件的测试。
//
// 这是整条 Java 读取链的最底层：不依赖方块注册表、不依赖任何映射器，因此当上层断言失败时，
// 本套用例可以用来判断"素材本身/区域文件解析出了问题"还是"上层解码出了问题"。
//
// 期望值来自对素材的独立解析（见 JavaRealWorldFixture.hpp 的说明），而非读取器的输出。

#include "common/world/storage/JavaRealWorldFixture.hpp"

#include "common/util/nbt/Nbt.hpp"
#include "server/world/storage/reader/java/RegionFile.hpp"

#include <string>
#include <vector>

namespace mc {
namespace {

using test::JavaRealWorld;
using test::JavaRealWorldFixture;
using world::storage::reader::java::RegionFile;

// ============================================================================
// 区域文件基本结构
// ============================================================================

TEST(JavaRealWorldRegionFileTest, OpenRealRegionAndParseHeader)
{
    RegionFile region(JavaRealWorld::worldDir() / "region" / "r.0.0.mca");

    auto openResult = region.open();
    ASSERT_TRUE(openResult.success()) << openResult.error().message();
    EXPECT_TRUE(region.isOpen());

    // 文件名 r.0.0.mca 中的区域坐标应被正确解析（正数分支）
    EXPECT_EQ(region.regionX(), 0);
    EXPECT_EQ(region.regionZ(), 0);
    EXPECT_EQ(region.path().filename().string(), "r.0.0.mca");

    EXPECT_EQ(region.listChunks().size(), static_cast<size_t>(JavaRealWorld::kColumnsR00));
    EXPECT_TRUE(region.hasChunk(0, 0));

    region.close();
    EXPECT_FALSE(region.isOpen());
}

TEST(JavaRealWorldRegionFileTest, ListChunksCountsPerRegion)
{
    struct Case {
        const char* fileName;
        i32 regionX;
        i32 regionZ;
        i32 columns;
    };
    // 负数区域坐标的文件名形如 r.-1.0.mca，需覆盖负号解析这一分支
    const Case cases[] = {
        {"r.0.0.mca", 0, 0, JavaRealWorld::kColumnsR00},
        {"r.-1.0.mca", -1, 0, JavaRealWorld::kColumnsRm10},
        {"r.0.-1.mca", 0, -1, JavaRealWorld::kColumnsR0m1},
        {"r.-1.-1.mca", -1, -1, JavaRealWorld::kColumnsRm1m1},
    };

    i32 total = 0;
    for (const auto& testCase : cases) {
        RegionFile region(JavaRealWorld::worldDir() / "region" / testCase.fileName);
        auto openResult = region.open();
        ASSERT_TRUE(openResult.success()) << testCase.fileName << ": " << openResult.error().message();

        EXPECT_EQ(region.regionX(), testCase.regionX) << testCase.fileName;
        EXPECT_EQ(region.regionZ(), testCase.regionZ) << testCase.fileName;
        EXPECT_EQ(region.listChunks().size(), static_cast<size_t>(testCase.columns)) << testCase.fileName;
        total += testCase.columns;
    }
    EXPECT_EQ(total, JavaRealWorld::kColumnsTotal);
}

TEST(JavaRealWorldRegionFileTest, ReadChunkDataDecompressesToRealNbt)
{
    RegionFile region(JavaRealWorld::worldDir() / "region" / "r.0.0.mca");
    ASSERT_TRUE(region.open().success());

    // 局部坐标 (8, 3) 即全局区块 (8, 3)，素材中该列为 Status=minecraft:full
    auto dataResult = region.readChunkData(8, 3);
    ASSERT_TRUE(dataResult.success()) << dataResult.error().message();
    const auto& bytes = dataResult.value();
    ASSERT_GT(bytes.size(), 0u);

    // Java 大端 NBT 的根标签类型为 TAG_Compound(0x0A)
    EXPECT_EQ(bytes[0], 0x0A);

    auto root = JavaRealWorld::parseRoot(bytes);
    ASSERT_NE(root, nullptr);

    ASSERT_EQ(root->value.count("xPos"), 1u);
    ASSERT_EQ(root->value.count("zPos"), 1u);
    ASSERT_EQ(root->value.count("Status"), 1u);
    ASSERT_EQ(root->value.count("DataVersion"), 1u);
    EXPECT_EQ(static_cast<i32>(root->get<nbt::tags::int_tag>("xPos")), 8);
    EXPECT_EQ(static_cast<i32>(root->get<nbt::tags::int_tag>("zPos")), 3);
    EXPECT_EQ(static_cast<i32>(root->get<nbt::tags::int_tag>("DataVersion")), JavaRealWorld::kDataVersion);
    EXPECT_EQ(root->get<nbt::tags::string_tag>("Status"), "minecraft:full");

    // 完整列应包含段数据、方块实体与高度图
    EXPECT_EQ(root->value.count("sections"), 1u);
    EXPECT_EQ(root->value.count("block_entities"), 1u);
    EXPECT_EQ(root->value.count("Heightmaps"), 1u);
}

// ============================================================================
// 列的存在性判定与缺失路径
// ============================================================================

TEST(JavaRealWorldRegionFileTest, HasChunkKnownAndUnknownLocals)
{
    {
        // r.0.0 的局部 X 仅覆盖 0..20，局部 (21, 0) 必不存在
        RegionFile region(JavaRealWorld::worldDir() / "region" / "r.0.0.mca");
        ASSERT_TRUE(region.open().success());
        EXPECT_TRUE(region.hasChunk(0, 0));
        EXPECT_FALSE(region.hasChunk(21, 0));
    }
    {
        // r.-1.0 的局部 X 覆盖 4..31，局部 (0, 0) 必不存在
        RegionFile region(JavaRealWorld::worldDir() / "region" / "r.-1.0.mca");
        ASSERT_TRUE(region.open().success());
        EXPECT_TRUE(region.hasChunk(4, 0));
        EXPECT_FALSE(region.hasChunk(0, 0));
    }
    {
        // r.0.-1 的局部 Z 覆盖 12..31，局部 (0, 0) 必不存在
        RegionFile region(JavaRealWorld::worldDir() / "region" / "r.0.-1.mca");
        ASSERT_TRUE(region.open().success());
        EXPECT_TRUE(region.hasChunk(0, 12));
        EXPECT_FALSE(region.hasChunk(0, 0));
    }
}

TEST(JavaRealWorldRegionFileTest, ReadMissingChunkFails)
{
    RegionFile region(JavaRealWorld::worldDir() / "region" / "r.0.0.mca");
    ASSERT_TRUE(region.open().success());

    auto dataResult = region.readChunkData(21, 0);
    ASSERT_TRUE(dataResult.failed());
    EXPECT_EQ(dataResult.error().code(), ErrorCode::ChunkNotFound);

    // 读取缺失列不应导致区域文件失效，后续读取同一文件的其他列仍应成功
    EXPECT_TRUE(region.isOpen());
    EXPECT_TRUE(region.readChunkData(8, 3).success());
}

TEST(JavaRealWorldRegionFileTest, OutOfBoundsLocalCoordsRejected)
{
    RegionFile region(JavaRealWorld::worldDir() / "region" / "r.0.0.mca");
    ASSERT_TRUE(region.open().success());

    // 区域文件每边 32 列，局部坐标合法区间为 [0, 31]
    EXPECT_FALSE(region.hasChunk(32, 0));
    EXPECT_FALSE(region.hasChunk(-1, 0));
    EXPECT_FALSE(region.hasChunk(0, 32));

    EXPECT_EQ(region.readChunkData(32, 0).error().code(), ErrorCode::ChunkNotFound);
    EXPECT_EQ(region.readChunkData(-1, 0).error().code(), ErrorCode::ChunkNotFound);
    EXPECT_EQ(region.readChunkData(0, -1).error().code(), ErrorCode::ChunkNotFound);
}

// ============================================================================
// 0 字节区域文件
// ============================================================================

TEST(JavaRealWorldRegionFileTest, ZeroByteRegionFileOpenFails)
{
    // 素材中 poi/r.0.-1.mca 是 0 字节的真实样本：游戏在尚未写入任何兴趣点数据时
    // 会留下这样一个空文件，读取器必须能安全拒绝而不是崩溃。
    RegionFile region(JavaRealWorld::worldDir() / "poi" / "r.0.-1.mca");
    ASSERT_TRUE(std::filesystem::exists(region.path()));
    EXPECT_EQ(std::filesystem::file_size(region.path()), 0u);

    auto openResult = region.open();
    EXPECT_TRUE(openResult.failed());
    EXPECT_FALSE(region.isOpen());
    EXPECT_FALSE(region.hasChunk(0, 0));
    EXPECT_TRUE(region.listChunks().empty());

    region.close(); // 未成功打开时 close 也必须安全
}

TEST(JavaRealWorldRegionFileTest, PoiRegionFileOpensNormally)
{
    // 与上一条对照：poi/ 目录本身是可读的，失败仅由"0 字节"引起
    RegionFile region(JavaRealWorld::worldDir() / "poi" / "r.0.0.mca");
    auto openResult = region.open();
    ASSERT_TRUE(openResult.success()) << openResult.error().message();
    EXPECT_EQ(region.listChunks().size(), static_cast<size_t>(JavaRealWorld::kPoiColumnsR00));
}

// ============================================================================
// 1.17+ 实体分离：entities/ 区域文件的列结构与 region/ 不同
// ============================================================================

TEST(JavaRealWorldRegionFileTest, EntitiesRegionHasPositionWithoutXPos)
{
    RegionFile region(JavaRealWorld::worldDir() / "entities" / "r.0.0.mca");
    auto openResult = region.open();
    ASSERT_TRUE(openResult.success()) << openResult.error().message();
    EXPECT_EQ(region.listChunks().size(), 15u);

    auto dataResult = region.readChunkData(8, 12);
    ASSERT_TRUE(dataResult.success()) << dataResult.error().message();
    auto root = JavaRealWorld::parseRoot(dataResult.value());
    ASSERT_NE(root, nullptr);

    // 实体列不携带 xPos/zPos，列坐标由 Position 整型数组给出。
    // JavaWorldReader 正是靠这一点区分"完整列"与"仅实体的列"。
    EXPECT_EQ(root->value.count("xPos"), 0u);
    EXPECT_EQ(root->value.count("zPos"), 0u);
    ASSERT_EQ(root->value.count("Position"), 1u);
    ASSERT_EQ(root->value.count("Entities"), 1u);
    EXPECT_EQ(root->value.count("DataVersion"), 1u);

    const auto& position = root->get<nbt::tags::intarray_tag>("Position");
    ASSERT_GE(position.size(), 2u);
    EXPECT_EQ(position[0], 8);
    EXPECT_EQ(position[1], 12);
}

} // namespace
} // namespace mc
