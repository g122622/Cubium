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

// JavaWorldReader 读取真实存档的测试。
//
// 本套用例只关心"世界级"策略：region 文件定位、维度分派、列的存在性、以及 1.17+ 之后
// region/ 与 entities/ 两处数据的合并。列内部如何解码（方块/群系/光照）由
// JavaRealWorldDecoderTest 覆盖，两层解耦后合并路径出错不会伪装成解码错误。
//
// 期望值来自对素材的独立解析，详见 JavaRealWorldFixture.hpp 的说明。

#include "common/world/storage/JavaRealWorldFixture.hpp"

#include "common/TempDirHelper.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "server/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include "server/world/storage/reader/java/JavaChunkReader.hpp"
#include "server/world/storage/reader/java/JavaColumnReader.hpp"
#include "server/world/storage/reader/java/JavaWorldReader.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace {

using test::JavaRealWorld;
using test::JavaRealWorldReaderFixture;
using world::storage::reader::java::JavaBiomeMapper;
using world::storage::reader::java::JavaBlockStateMapper;
using world::storage::reader::java::JavaChunkReader;
using world::storage::reader::java::JavaColumnReader;
using world::storage::reader::java::JavaWorldReader;

/// 在区块坐标列表中查找指定坐标
bool containsChunk(const std::vector<ChunkPos>& chunks, ChunkCoord x, ChunkCoord z)
{
    return std::any_of(chunks.begin(), chunks.end(), [x, z](const ChunkPos& pos) { return pos.x == x && pos.z == z; });
}

// ============================================================================
// 区块枚举
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, ListChunksOverworld)
{
    auto result = worldReader().listChunks(0);
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& chunks = result.value();
    EXPECT_EQ(chunks.size(), static_cast<size_t>(JavaRealWorld::kColumnsTotal));

    // 抽查各区间的已知存在坐标（含跨 region 边界与负坐标）
    EXPECT_TRUE(containsChunk(chunks, 8, 3));
    EXPECT_TRUE(containsChunk(chunks, -4, 7));
    EXPECT_TRUE(containsChunk(chunks, 0, 7));
    EXPECT_TRUE(containsChunk(chunks, -5, -1));
    EXPECT_TRUE(containsChunk(chunks, -28, -20));
    EXPECT_TRUE(containsChunk(chunks, 0, 0));

    // 素材的区块覆盖不是矩形：xPos 的包围盒上界为 20、zPos 为 28，但 (20, 28) 并不存在。
    // 该断言用于防止把"包围盒"误当作"全覆盖"。
    EXPECT_FALSE(containsChunk(chunks, 20, 28));
    EXPECT_FALSE(containsChunk(chunks, 1000, 1000));
}

TEST_F(JavaRealWorldReaderFixture, ListChunksEmptyDimensions)
{
    // 玩家未去过下界与末地：DIM-1/ 与 DIM1/ 只有 data/ 而没有 region/。
    // "没有 region 目录"应表现为空的成功结果，而不是错误。
    for (const DimensionId dimension : {static_cast<DimensionId>(-1), static_cast<DimensionId>(1)}) {
        auto result = worldReader().listChunks(dimension);
        ASSERT_TRUE(result.success()) << "dimension " << dimension << ": " << result.error().message();
        EXPECT_TRUE(result.value().empty()) << "dimension " << dimension;
    }

    // 对照：主世界非空
    auto overworld = worldReader().listChunks(0);
    ASSERT_TRUE(overworld.success());
    EXPECT_FALSE(overworld.value().empty());
}

// ============================================================================
// 列读取：完成状态过滤与越界
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, ReadFullColumn)
{
    auto result = worldReader().readChunk(8, 3, 0);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());

    auto chunk = std::move(result.value().value());
    EXPECT_EQ(chunk.x(), 8);
    EXPECT_EQ(chunk.z(), 3);
    EXPECT_TRUE(chunk.isLoaded());
    EXPECT_EQ(chunk.getSections().size(), static_cast<size_t>(world::CHUNK_SECTIONS));
}

TEST_F(JavaRealWorldReaderFixture, ReadUnfinishedStatusReturnsEmpty)
{
    // 素材中存在大量未完成生成的列。它们不应被当作可加载区块返回。
    struct Case {
        ChunkCoord x;
        ChunkCoord z;
        const char* status;
    };
    const Case cases[] = {
        {-13, 19, "minecraft:noise"},
        {4, 18, "minecraft:features"},
        {-18, -10, "minecraft:initialize_light"},
        {-28, -20, "minecraft:structure_starts"},
    };

    for (const auto& testCase : cases) {
        auto result = worldReader().readChunk(testCase.x, testCase.z, 0);
        ASSERT_TRUE(result.success()) << testCase.status << ": " << result.error().message();
        EXPECT_FALSE(result.value().has_value()) << "Status=" << testCase.status << " 的列不应被加载为区块";
    }

    // 对照：Status=minecraft:full 的列必须有值
    auto full = worldReader().readChunk(8, 3, 0);
    ASSERT_TRUE(full.success());
    EXPECT_TRUE(full.value().has_value());
}

TEST_F(JavaRealWorldReaderFixture, ReadOutOfRangeChunkReturnsEmpty)
{
    // 超出素材覆盖范围的坐标：region 文件不存在，应返回"无此区块"而不是错误
    EXPECT_FALSE(worldReader().readChunk(1000, 1000, 0).value().has_value());
    EXPECT_FALSE(worldReader().readChunk(-1000, -1000, 0).value().has_value());
    EXPECT_FALSE(worldReader().readChunk(1000, 1000, -1).value().has_value());
}

// ============================================================================
// 1.17+ 实体分离：region/ 与 entities/ 的合并
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, MergeEntitiesIntoMainColumn)
{
    // (-4, 7) 同时存在于 region/ 与 entities/：前者提供方块与方块实体，
    // 后者提供实体列表，读取器应把两者合并到同一个 ChunkData 上。
    auto result = worldReader().readChunk(-4, 7, 0);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());

    auto chunk = std::move(result.value().value());
    EXPECT_EQ(chunk.x(), -4);
    EXPECT_EQ(chunk.z(), 7);
    EXPECT_EQ(chunk.blockEntityCount(), 4u);
    EXPECT_TRUE(chunk.hasLoadedEntityNbt());

    // 实体的反序列化是延后的，NBT 只能通过 takeLoadedEntityNbt() 取出，且该调用会搬空内容，
    // 因此整个用例中只调用一次并存进局部变量。
    auto entityNbt = chunk.takeLoadedEntityNbt();
    ASSERT_EQ(entityNbt.size(), 1u);
    ASSERT_NE(entityNbt[0], nullptr);
    ASSERT_EQ(entityNbt[0]->value.count("id"), 1u);
    EXPECT_EQ(entityNbt[0]->get<nbt::tags::string_tag>("id"), "minecraft:creeper");
}

TEST_F(JavaRealWorldReaderFixture, MergeControlWithoutEntitiesRegion)
{
    // 上一条的对照实验：直接读同一列的原始 NBT（不经 JavaWorldReader 的合并），
    // 其结果不应带有实体 NBT。两条用例一起证明合并确实发生，而不是列本身自带实体。
    auto raw = JavaRealWorld::rawColumnNbt("r.-1.0.mca", 28, 7);
    ASSERT_TRUE(raw.success()) << raw.error().message();

    auto columnResult = columnReader().readColumn(raw.value(), -4, 7, 0);
    ASSERT_TRUE(columnResult.success()) << columnResult.error().message();
    ASSERT_TRUE(columnResult.value().has_value());

    auto chunk = std::move(columnResult.value().value());
    EXPECT_EQ(chunk.blockEntityCount(), 4u);
    EXPECT_FALSE(chunk.hasLoadedEntityNbt());
}

TEST_F(JavaRealWorldReaderFixture, MergeColumnWithManyEntities)
{
    // (-5, -1) 的 entities 列含 8 个实体（7 只鱿鱼 + 1 只僵尸）
    auto result = worldReader().readChunk(-5, -1, 0);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());

    auto chunk = std::move(result.value().value());
    EXPECT_TRUE(chunk.hasLoadedEntityNbt());
    EXPECT_EQ(chunk.takeLoadedEntityNbt().size(), 8u);
}

TEST_F(JavaRealWorldReaderFixture, NoEntityNbtWhenEntitiesRegionLacksColumn)
{
    // (8, 3) 与 (8, 2) 只存在于 region/，entities/ 中没有对应列
    for (const ChunkCoord z : {3, 2}) {
        auto result = worldReader().readChunk(8, z, 0);
        ASSERT_TRUE(result.success()) << result.error().message();
        ASSERT_TRUE(result.value().has_value());
        EXPECT_FALSE(result.value().value().hasLoadedEntityNbt()) << "z=" << z;
    }
}

TEST_F(JavaRealWorldReaderFixture, EntityOnlyColumnSynthesized)
{
    // 素材中 entities/ 的 97 列与 region/ 完全重叠，因此"仅 entities 有数据"这条分支
    // 在真实存档里覆盖不到，必须手工构造：
    //   region/r.0.0.mca  —— 8192 字节的全零头，可正常打开但不含任何列
    //   entities/r.0.0.mca —— 从素材拷贝，含 (0, 7) 列
    // 读取 (0, 7) 时 region 侧无列、entities 侧有列，读取器应合成一个最小列。
    const auto tempDir = test::makeUniqueTestDir("mc_java_real_world");
    std::filesystem::create_directories(tempDir / "region");
    std::filesystem::create_directories(tempDir / "entities");

    std::filesystem::copy_file(JavaRealWorld::worldDir() / "entities" / "r.0.0.mca",
        tempDir / "entities" / "r.0.0.mca",
        std::filesystem::copy_options::overwrite_existing);

    {
        // 8192 字节区域文件头（1024 条偏移 + 1024 条时间戳），全零表示没有任何列
        std::ofstream out(tempDir / "region" / "r.0.0.mca", std::ios::binary);
        ASSERT_TRUE(out.is_open());
        const std::vector<char> emptyHeader(8192, '\0');
        out.write(emptyHeader.data(), static_cast<std::streamsize>(emptyHeader.size()));
    }

    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader chunkReader(blockMapper, biomeMapper);
    JavaColumnReader columnReader(chunkReader);
    JavaWorldReader reader(columnReader);
    ASSERT_TRUE(reader.open(tempDir, JavaRealWorld::formatInfo()).success());

    auto result = reader.readChunk(0, 7, 0);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());

    auto chunk = std::move(result.value().value());
    // 列坐标来自 entities 列的 Position 字段，而非调用参数
    EXPECT_EQ(chunk.x(), 0);
    EXPECT_EQ(chunk.z(), 7);
    EXPECT_TRUE(chunk.hasLoadedEntityNbt());
    EXPECT_EQ(chunk.takeLoadedEntityNbt().size(), 1u);

    // 合成列没有任何方块数据
    EXPECT_EQ(chunk.blockEntityCount(), 0u);

    reader.close();
    test::removeTestDir(tempDir);
}

// ============================================================================
// 生命周期：未打开的读取器必须拒绝读取
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, ReadBeforeOpenFails)
{
    // JavaWorldReader::open 不做任何校验（无条件成功），因此"未打开"是唯一的失败前状态
    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader chunkReader(blockMapper, biomeMapper);
    JavaColumnReader columnReader(chunkReader);
    JavaWorldReader reader(columnReader);

    EXPECT_FALSE(reader.isOpen());
    auto readResult = reader.readChunk(8, 3, 0);
    EXPECT_TRUE(readResult.failed());
    EXPECT_EQ(readResult.error().code(), ErrorCode::InvalidState);

    auto listResult = reader.listChunks(0);
    EXPECT_TRUE(listResult.failed());
    EXPECT_EQ(listResult.error().code(), ErrorCode::InvalidState);

    ASSERT_TRUE(reader.open(m_worldDir, JavaRealWorld::formatInfo()).success());
    EXPECT_TRUE(reader.isOpen());
    EXPECT_TRUE(reader.readChunk(8, 3, 0).success());

    reader.close();
    EXPECT_FALSE(reader.isOpen());
    EXPECT_EQ(reader.readChunk(8, 3, 0).error().code(), ErrorCode::InvalidState);
}

} // namespace
} // namespace mc
