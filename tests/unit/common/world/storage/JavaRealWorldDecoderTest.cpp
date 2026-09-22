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

// JavaColumnReader / JavaChunkReader 对真实列的 NBT→ChunkData 解码测试。
//
// 本套用例直接消费 RegionFile 解出的原始列 NBT（不经 JavaWorldReader），因此与区域定位、
// 实体合并等策略无关，只验证列内部的解码：段调色板、方块状态映射、生物群系、高度图、
// 方块实体、以及未完成状态的过滤。
//
// 所有期望值来自对素材的独立解析（见 JavaRealWorldFixture.hpp 的说明）。
// 特别注意高度图：Java 存储值是「最高方块 Y + 1 - 维度最低 Y」，主世界最低 Y 为 -64，
// 因此 getTopBlockY 的期望值是「存储值 - 65」，而不是存储值本身。

#include "common/world/storage/JavaRealWorldFixture.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/storage/reader/java/JavaColumnReader.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace {

using test::JavaRealWorld;
using test::JavaRealWorldReaderFixture;

/// 该测试里所有断言都用到的区块局部坐标 ↔ 素材中的世界区块坐标对应关系：
///   chunk(8, 3)  —— region/r.0.0.mca 的局部坐标 (8, 3)
///   chunk(-4, 7) —— region/r.-1.0.mca 的局部坐标 (28, 7)
///   chunk(-8, 16) —— region/r.-1.0.mca 的局部坐标 (24, 16)
constexpr i32 kChunkX = 8;
constexpr i32 kChunkZ = 3;

/**
 * @brief 取区块内指定世界坐标处的方块状态，断言其存在后返回所属 Block
 *
 * @param chunk 已解码的区块
 * @param x 区块内局部 X
 * @param y 世界 Y
 * @param z 区块内局部 Z
 * @return 该处方块
 */
const Block* blockAt(const ChunkData& chunk, BlockCoord x, BlockCoord y, BlockCoord z)
{
    const BlockState* state = chunk.getBlockState(x, y, z);
    EXPECT_NE(state, nullptr) << "(" << x << ", " << y << ", " << z << ") 处应为非空气方块";
    return state != nullptr ? &state->getBlock() : nullptr;
}

/// 按资源位置取内部方块定义
const Block* blockByName(const char* id)
{
    return BlockRegistry::instance().getBlock(ResourceLocation(id));
}

/// 使用夹具的读取器链解码素材中的某一列
std::optional<ChunkData> decodeColumn(JavaRealWorldReaderFixture& fixture,
    const char* regionFile,
    i32 localX,
    i32 localZ,
    ChunkCoord chunkX,
    ChunkCoord chunkZ)
{
    auto raw = JavaRealWorld::rawColumnNbt(regionFile, localX, localZ);
    EXPECT_TRUE(raw.success()) << raw.error().message();
    if (raw.failed()) {
        return std::nullopt;
    }
    auto result = fixture.columnReader().readColumn(raw.value(), chunkX, chunkZ, 0);
    EXPECT_TRUE(result.success()) << result.error().message();
    if (result.failed() || !result.value().has_value()) {
        return std::nullopt;
    }
    return std::move(result.value().value());
}

// ============================================================================
// 段调色板与方块状态映射
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, DecodesSectionPaletteAndBlocks)
{
    auto chunkOpt = decodeColumn(*this, "r.0.0.mca", kChunkX, kChunkZ, kChunkX, kChunkZ);
    ASSERT_TRUE(chunkOpt.has_value());
    const auto& chunk = *chunkOpt;

    // 主世界段 Y 范围 -4..19，共 24 段
    EXPECT_EQ(chunk.getSections().size(), static_cast<size_t>(world::CHUNK_SECTIONS));
    EXPECT_TRUE(chunk.hasSection(0));  // 段 Y=-4
    EXPECT_TRUE(chunk.hasSection(23)); // 段 Y=19

    // 基岩层与深层石：验证方块状态字符串→内部 stateId 的映射确实生效。
    // 若 JavaBlockStateMapper 未能在 BlockRegistry 中命中，全部方块都会退化为空气，
    // 这些断言会立刻失败。
    if (const Block* bedrock = blockByName("minecraft:bedrock")) {
        EXPECT_EQ(blockAt(chunk, 0, world::MIN_BUILD_HEIGHT, 0), bedrock);
    } else {
        ADD_FAILURE() << "BlockRegistry 中缺少 minecraft:bedrock，方块注册表可能未初始化";
    }
    if (const Block* deepslate = blockByName("minecraft:deepslate")) {
        EXPECT_EQ(blockAt(chunk, 6, world::MIN_BUILD_HEIGHT + 1, 0), deepslate);
    }
    if (const Block* stone = blockByName("minecraft:stone")) {
        EXPECT_EQ(blockAt(chunk, 4, -34, 0), stone);
    }

    // 试炼密室结构方块：该坐标与下方的方块实体断言指向同一处
    if (const Block* trialSpawner = blockByName("minecraft:trial_spawner")) {
        EXPECT_EQ(blockAt(chunk, 5, -9, 4), trialSpawner);
    }
    if (const Block* copper = blockByName("minecraft:waxed_copper_block")) {
        EXPECT_EQ(blockAt(chunk, 8, -20, 7), copper);
    }

    // 高于实际地形的段仍是空气（该段调色板只有空气一项）
    EXPECT_EQ(chunk.getBlockStateId(5, 300, 4), 0u);
    // 超出世界高度上限的坐标按空气处理
    EXPECT_EQ(chunk.getBlockState(5, world::MAX_BUILD_HEIGHT + 1, 4), nullptr);
}

// ============================================================================
// 生物群系
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, DecodesBiomeSections)
{
    // 1.18+ 的生物群系按段存储于 sections[].biomes，需按 4x4x4 采样解包
    auto chunk83 = decodeColumn(*this, "r.0.0.mca", kChunkX, kChunkZ, 8, 3);
    ASSERT_TRUE(chunk83.has_value());
    EXPECT_EQ(chunk83->getBiomeAtBlock(5, 100, 4), Biomes::BirchForest);
    EXPECT_EQ(chunk83->getBiomeAtBlock(0, world::MIN_BUILD_HEIGHT, 0), Biomes::BirchForest);

    auto chunkM47 = decodeColumn(*this, "r.-1.0.mca", 28, 7, -4, 7);
    ASSERT_TRUE(chunkM47.has_value());
    EXPECT_EQ(chunkM47->getBiomeAtBlock(5, -9, 4), Biomes::Forest);

    // 该列地形为草甸（minecraft:meadow），映射表将其归入平原
    auto chunkM816 = decodeColumn(*this, "r.-1.0.mca", 24, 16, -8, 16);
    ASSERT_TRUE(chunkM816.has_value());
    EXPECT_EQ(chunkM816->getBiomeAtBlock(5, -9, 4), Biomes::Plains);
}

// ============================================================================
// 高度图
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, DecodesHeightmapsForFullChunk)
{
    auto chunkOpt = decodeColumn(*this, "r.0.0.mca", kChunkX, kChunkZ, kChunkX, kChunkZ);
    ASSERT_TRUE(chunkOpt.has_value());
    const auto& chunk = *chunkOpt;

    // 素材中 WORLD_SURFACE 的存储值依次为 135 / 128 / 132 / 127，
    // 主世界最低 Y 为 -64，故最高方块 Y = 存储值 - 65
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::WorldSurface, 5, 4), 70);
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::WorldSurface, 11, 6), 63);
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::WorldSurface, 0, 0), 67);
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::WorldSurface, 15, 15), 62);

    // getHeightmapFirstAvailable 返回的是「最高方块 Y + 1」这一原生语义
    EXPECT_EQ(chunk.getHeightmapFirstAvailable(HeightmapType::WorldSurface, 5, 4), 71);

    // 该列上方有树叶：MOTION_BLOCKING_NO_LEAVES 比 WORLD_SURFACE 低，
    // 证明各类型写入的是各自的槽位而非共享同一个数组
    EXPECT_EQ(chunk.getTopBlockY(HeightmapType::MotionBlockingNoLeaves, 5, 4), 62);
    EXPECT_LT(chunk.getTopBlockY(HeightmapType::MotionBlockingNoLeaves, 5, 4),
        chunk.getTopBlockY(HeightmapType::WorldSurface, 5, 4));

    // 该列地表面为固体，故 OCEAN_FLOOR 与 WORLD_SURFACE 一致
    EXPECT_EQ(
        chunk.getTopBlockY(HeightmapType::OceanFloor, 5, 4), chunk.getTopBlockY(HeightmapType::WorldSurface, 5, 4));
}

TEST_F(JavaRealWorldReaderFixture, MissingHeightmapTypesFallBackToWorldSurface)
{
    auto chunkOpt = decodeColumn(*this, "r.0.0.mca", kChunkX, kChunkZ, kChunkX, kChunkZ);
    ASSERT_TRUE(chunkOpt.has_value());
    const auto& chunk = *chunkOpt;

    // 素材 1.21.11 的完整列只写入了 WORLD_SURFACE / OCEAN_FLOOR / MOTION_BLOCKING /
    // MOTION_BLOCKING_NO_LEAVES 四种，没有 *_WG 与 LIGHT_BLOCKING。
    ASSERT_TRUE(chunk.isHeightmapInitialized(HeightmapType::WorldSurface));
    EXPECT_FALSE(chunk.isHeightmapInitialized(HeightmapType::WorldSurfaceWG));
    EXPECT_FALSE(chunk.isHeightmapInitialized(HeightmapType::LightBlocking));

    // 未初始化的类型回退到 WorldSurface 槽位
    EXPECT_EQ(
        chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, 5, 4), chunk.getTopBlockY(HeightmapType::WorldSurface, 5, 4));
    EXPECT_EQ(
        chunk.getTopBlockY(HeightmapType::LightBlocking, 5, 4), chunk.getTopBlockY(HeightmapType::WorldSurface, 5, 4));
}

TEST_F(JavaRealWorldReaderFixture, HeightmapValuesAcrossChunks)
{
    // (-8, 16) 处地形为高地：WORLD_SURFACE 存储 191 而 OCEAN_FLOOR 存储 190，
    // 因为最高处方块（short_grass）不是固体
    auto highland = decodeColumn(*this, "r.-1.0.mca", 24, 16, -8, 16);
    ASSERT_TRUE(highland.has_value());
    EXPECT_EQ(highland->getTopBlockY(HeightmapType::WorldSurface, 5, 4), 126);
    EXPECT_EQ(highland->getTopBlockY(HeightmapType::OceanFloor, 5, 4), 125);

    auto forest = decodeColumn(*this, "r.-1.0.mca", 28, 7, -4, 7);
    ASSERT_TRUE(forest.has_value());
    EXPECT_EQ(forest->getTopBlockY(HeightmapType::WorldSurface, 5, 4), 67);
}

// ============================================================================
// 方块实体
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, DecodesBlockEntities)
{
    auto chunkOpt = decodeColumn(*this, "r.0.0.mca", kChunkX, kChunkZ, kChunkX, kChunkZ);
    ASSERT_TRUE(chunkOpt.has_value());
    const auto& chunk = *chunkOpt;

    // 该区块是试炼密室的一部分，含 19 个方块实体
    ASSERT_EQ(chunk.blockEntityCount(), 19u);

    std::map<BlockEntityType, i32> byType;
    for (const BlockEntity* entity : chunk.getAllBlockEntities()) {
        ASSERT_NE(entity, nullptr);
        ++byType[entity->getType()];
    }
    EXPECT_EQ(byType[BlockEntityType::DecoratedPot], 12);
    EXPECT_EQ(byType[BlockEntityType::TrialSpawner], 3);
    EXPECT_EQ(byType[BlockEntityType::Dispenser], 1);
    EXPECT_EQ(byType[BlockEntityType::Hopper], 1);
    EXPECT_EQ(byType[BlockEntityType::Barrel], 1);
    EXPECT_EQ(byType[BlockEntityType::Vault], 1);

    // 按绝对坐标取方块实体，并与解码出的方块交叉印证
    const BlockPos spawnerPos(133, -9, 52);
    const BlockEntity* spawner = chunk.getBlockEntity(spawnerPos);
    ASSERT_NE(spawner, nullptr);
    EXPECT_EQ(spawner->getType(), BlockEntityType::TrialSpawner);
    EXPECT_EQ(spawner->getPos(), spawnerPos);
    EXPECT_TRUE(chunk.hasBlockEntity(spawnerPos));
}

TEST_F(JavaRealWorldReaderFixture, BlockEntityPositionsMatchDecodedBlocks)
{
    // 自洽断言：不依赖硬编码坐标，遍历所有方块实体，验证其所在位置的方块非空气。
    // 这能在不引入外部期望值的前提下发现"方块实体位置与方块数据错位"这类缺陷。
    struct Case {
        const char* regionFile;
        i32 localX;
        i32 localZ;
        ChunkCoord chunkX;
        ChunkCoord chunkZ;
        size_t expectedBlockEntities;
    };
    const Case cases[] = {
        {"r.0.0.mca", 8, 3, 8, 3, 19},
        {"r.0.0.mca", 8, 2, 8, 2, 18},
        {"r.-1.0.mca", 28, 7, -4, 7, 4},
    };

    for (const auto& testCase : cases) {
        auto chunkOpt = decodeColumn(
            *this, testCase.regionFile, testCase.localX, testCase.localZ, testCase.chunkX, testCase.chunkZ);
        ASSERT_TRUE(chunkOpt.has_value()) << testCase.regionFile;
        const auto& chunk = *chunkOpt;
        ASSERT_EQ(chunk.blockEntityCount(), testCase.expectedBlockEntities) << testCase.regionFile;

        for (const BlockEntity* entity : chunk.getAllBlockEntities()) {
            const BlockPos pos = entity->getPos();
            const BlockCoord localX = pos.x & (world::CHUNK_WIDTH - 1);
            const BlockCoord localZ = pos.z & (world::CHUNK_WIDTH - 1);
            EXPECT_NE(chunk.getBlockStateId(localX, pos.y, localZ), 0u)
                << "方块实体位于 (" << pos.x << ", " << pos.y << ", " << pos.z << ") 但该处方块是空气";
        }
    }
}

TEST_F(JavaRealWorldReaderFixture, DecodesBlockEntitiesForOtherChunks)
{
    auto dungeon = decodeColumn(*this, "r.-1.0.mca", 28, 7, -4, 7);
    ASSERT_TRUE(dungeon.has_value());
    EXPECT_EQ(dungeon->blockEntityCount(), 4u);
    const BlockEntity* spawner = dungeon->getBlockEntity(BlockPos(-50, -30, 122));
    ASSERT_NE(spawner, nullptr);
    EXPECT_EQ(spawner->getType(), BlockEntityType::MobSpawner);

    // 幽匿区块：素材含 3 个 sculk_catalyst 与 4 个 sculk_sensor，但内部
    // BlockEntityType 没有 SculkCatalyst 这一项，未知类型会被跳过并打 warn，
    // 因此只有 4 个感测体进入区块。
    // TODO: 补齐 SculkCatalyst 方块实体类型（枚举、id 映射、注册与实现），
    //       届时本断言的期望值应改为 7。
    auto sculk = decodeColumn(*this, "r.-1.0.mca", 24, 16, -8, 16);
    ASSERT_TRUE(sculk.has_value());
    EXPECT_EQ(sculk->blockEntityCount(), 4u);
    i32 sensorCount = 0;
    for (const BlockEntity* entity : sculk->getAllBlockEntities()) {
        if (entity->getType() == BlockEntityType::SculkSensor) {
            ++sensorCount;
        }
    }
    EXPECT_EQ(sensorCount, 4);
}

// ============================================================================
// 其它列级字段
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, DecodesInhabitedTime)
{
    auto visited = decodeColumn(*this, "r.-1.0.mca", 28, 7, -4, 7);
    ASSERT_TRUE(visited.has_value());
    EXPECT_EQ(visited->inhabitedTime(), 86);

    auto untouched = decodeColumn(*this, "r.0.0.mca", kChunkX, kChunkZ, kChunkX, kChunkZ);
    ASSERT_TRUE(untouched.has_value());
    EXPECT_EQ(untouched->inhabitedTime(), 0);
}

// ============================================================================
// 未完成状态的列在列级同样应被拒绝
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, UnfinishedStatusColumnReturnsEmpty)
{
    struct Case {
        const char* regionFile;
        i32 localX;
        i32 localZ;
        ChunkCoord chunkX;
        ChunkCoord chunkZ;
        const char* status;
    };
    const Case cases[] = {
        {"r.-1.0.mca", 19, 19, -13, 19, "minecraft:noise"},
        {"r.0.0.mca", 4, 18, 4, 18, "minecraft:features"},
        {"r.-1.-1.mca", 14, 22, -18, -10, "minecraft:initialize_light"},
        {"r.-1.-1.mca", 4, 12, -28, -20, "minecraft:structure_starts"},
    };

    for (const auto& testCase : cases) {
        auto raw = JavaRealWorld::rawColumnNbt(testCase.regionFile, testCase.localX, testCase.localZ);
        ASSERT_TRUE(raw.success()) << testCase.status << ": " << raw.error().message();

        auto result = columnReader().readColumn(raw.value(), testCase.chunkX, testCase.chunkZ, 0);
        ASSERT_TRUE(result.success()) << testCase.status << ": " << result.error().message();
        EXPECT_FALSE(result.value().has_value()) << "Status=" << testCase.status << " 的列不应解码为区块";
    }
}

} // namespace
} // namespace mc
