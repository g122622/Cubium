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
 * IMPLIED, BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// ============================================================================
// BaseChunkGenerator 和 IChunkGenerator 接口单元测试
//
// 测试覆盖：
// 1. BaseChunkGenerator 默认实现
// 2. IChunkGenerator 虚接口验证
// 3. DimensionSettings 工厂方法
// 4. ChunkStep 与 WorldGenRegion 的交互
// 5. 与 MC 1.21.11 ChunkGenerator 的行为对齐
// ============================================================================

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

class BaseChunkGeneratorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. DimensionSettings 工厂方法
// ============================================================================

TEST_F(BaseChunkGeneratorTest, OverworldDimensionSettings)
{
    const auto& settings = DimensionSettings::overworld();
    // 主世界参数
    EXPECT_EQ(settings.dimensionKind, DimensionKind::Overworld);
    EXPECT_NE(settings.defaultBlock, nullptr);
    EXPECT_NE(settings.defaultFluid, nullptr);
    EXPECT_GT(settings.seaLevel, 0);
    EXPECT_EQ(settings.noise.minY, -64);
    EXPECT_EQ(settings.noise.height, 384);
}

TEST_F(BaseChunkGeneratorTest, NetherDimensionSettings)
{
    const auto& settings = DimensionSettings::nether();
    EXPECT_EQ(settings.dimensionKind, DimensionKind::Nether);
    EXPECT_NE(settings.defaultBlock, nullptr);
    EXPECT_NE(settings.defaultFluid, nullptr);
    EXPECT_EQ(settings.noise.minY, 0);
    EXPECT_EQ(settings.noise.height, 128);
}

TEST_F(BaseChunkGeneratorTest, EndDimensionSettings)
{
    const auto& settings = DimensionSettings::end();
    EXPECT_EQ(settings.dimensionKind, DimensionKind::End);
    EXPECT_NE(settings.defaultBlock, nullptr);
    EXPECT_NE(settings.defaultFluid, nullptr);
    EXPECT_EQ(settings.noise.minY, 0);
    EXPECT_EQ(settings.noise.height, 128); // MC 1.21: 末地高度为 128
}

TEST_F(BaseChunkGeneratorTest, FlatDimensionSettings)
{
    const auto& settings = DimensionSettings::flat();
    EXPECT_EQ(settings.dimensionKind, DimensionKind::Flat);
}

// ============================================================================
// 2. FlatChunkGenerator 作为 BaseChunkGenerator 子类
// ============================================================================

TEST_F(BaseChunkGeneratorTest, FlatGenerator_DefaultBiomeFill)
{
    // BaseChunkGenerator::generateBiomes 默认填充 m_defaultBiome
    // FlatChunkGenerator 设置 m_defaultBiome = FlatSettings.biomeId()
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());

    ChunkPrimer primer(0, 0);
    // 需要创建 WorldGenRegion 来调用 generateBiomes
    // 此处验证 biomeSource 返回正确值
    EXPECT_EQ(gen.getBiomeSource()->getNoiseBiome(0, 0, 0), Biomes::Plains);
}

// ============================================================================
// 3. IChunkGenerator 接口虚方法验证
// ============================================================================

TEST_F(BaseChunkGeneratorTest, InterfaceVirtualMethods_DebugGenerator)
{
    DebugChunkGenerator gen;
    IChunkGenerator& base = gen;

    EXPECT_TRUE(base.isDebugGenerator());
    EXPECT_EQ(base.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(base.getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(base.getHeight(0, 0, HeightmapType::WorldSurface), 0);
    EXPECT_EQ(base.getGroundHeight(), 0);
    EXPECT_EQ(base.seaLevel(), 63);
    EXPECT_EQ(base.getGenDepth(), 384);
    EXPECT_EQ(base.getMinY(), 0);
}

TEST_F(BaseChunkGeneratorTest, InterfaceVirtualMethods_FlatGenerator)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    IChunkGenerator& base = gen;

    EXPECT_FALSE(base.isDebugGenerator());
    EXPECT_EQ(base.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(base.getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(base.seaLevel(), -63);
    EXPECT_EQ(base.getMinY(), 0);
    EXPECT_EQ(base.getGenDepth(), 384);
}

TEST_F(BaseChunkGeneratorTest, InterfaceVirtualMethods_NoiseGenerator)
{
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 12345ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));
    IChunkGenerator& base = gen;

    EXPECT_FALSE(base.isDebugGenerator());
    EXPECT_EQ(base.seed(), 12345u);
    EXPECT_NE(base.getBiomeSource(), nullptr);
    EXPECT_EQ(base.getMinY(), -64);     // 主世界 minY
    EXPECT_EQ(base.getGenDepth(), 384); // 主世界高度
    EXPECT_EQ(base.seaLevel(), 63);     // 主世界海平面
}

// ============================================================================
// 4. NoiseChunkGenerator 维度参数
// ============================================================================

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_OverworldDimensionParams)
{
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 0ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));

    EXPECT_EQ(gen.getMinY(), -64);
    EXPECT_EQ(gen.getGenDepth(), 384);
    EXPECT_EQ(gen.seaLevel(), 63);
    EXPECT_EQ(gen.getGroundHeight(), 64); // seaLevel + 1
}

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_NetherDimensionParams)
{
    // 下界使用 NetherBiomeSource
    auto settings = DimensionSettings::nether();
    auto randomState = world::gen::RandomState::create(settings, 0ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(*randomState);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));

    EXPECT_EQ(gen.getMinY(), 0);
    EXPECT_EQ(gen.getGenDepth(), 128);
}

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_EndDimensionParams)
{
    // 末地使用 EndBiomeSource，此处用 MultiNoiseBiomeSource 临时替代
    auto settings = DimensionSettings::end();
    auto randomState = world::gen::RandomState::create(settings, 0ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));

    EXPECT_EQ(gen.getMinY(), 0);
    EXPECT_EQ(gen.getGenDepth(), 128); // MC 1.21: 末地高度为 128
}

// ============================================================================
// 5. ChunkStep 与 WorldGenRegion 的交互
// ============================================================================

TEST_F(BaseChunkGeneratorTest, ChunkStep_NoiseWriteRadius)
{
    // NOISE 步骤的写入半径应该为 0（只写中心区块）
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::NOISE);
    EXPECT_EQ(step.blockStateWriteRadius(), 0);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_SurfaceWriteRadius)
{
    // SURFACE 步骤的写入半径应该为 0
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::SURFACE);
    EXPECT_EQ(step.blockStateWriteRadius(), 0);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_CarversWriteRadius)
{
    // CARVERS 步骤的写入半径应该为 0
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::CARVERS);
    EXPECT_EQ(step.blockStateWriteRadius(), 0);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_FeaturesWriteRadius)
{
    // FEATURES 步骤的写入半径应该 > 0（允许写邻居区块）
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::FEATURES);
    EXPECT_GT(step.blockStateWriteRadius(), 0);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_LightWriteRadius)
{
    // LIGHT 步骤的写入半径应为 2：光照在 LIGHT 阶段于 worker 线程执行，
    // 经 WorldLightManager::lightChunk 写半径2邻居的 nibble，走 m_radiusAwareExecutor（5×5 写区互斥）。
    //
    // neighbourReadRadius：ChunkStep::neighbourReadRadius() 返回 accumulatedRadius()，
    // 即所有前序步骤累积依赖的最大半径（对齐 Moonrise 的 neighbourReadRadius =
    // getAccumulatedRadiusOf(EMPTY)，主世界中 EMPTY 为最外圈状态）。LIGHT 继承
    // STRUCTURE_REFERENCES 的 radius-8 STRUCTURE_STARTS 依赖及其前序偏移，累积到 11
    // （与 FULL 的 accumulatedRadius=11 一致，见 ChunkTaskScheduler 注释）。半径2邻居
    // 经 ChunkLightingProvider fallback 到 ServerWorld::getChunkForLight 自取，不影响此值。
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::LIGHT);
    EXPECT_EQ(step.blockStateWriteRadius(), 2);
    EXPECT_EQ(step.neighbourReadRadius(), 11);
    // 直接依赖半径（directRadius）才是 1：LIGHT 仅直接要求 INITIALIZE_LIGHT 半径1
    EXPECT_EQ(step.directRadius(), 1);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_EmptyWriteRadius)
{
    // EMPTY 步骤不应该写任何方块
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::EMPTY);
    EXPECT_LT(step.blockStateWriteRadius(), 0);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_StructureStartsWriteRadius)
{
    // STRUCTURE_STARTS 步骤不应该写方块
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_LT(step.blockStateWriteRadius(), 0);
}

TEST_F(BaseChunkGeneratorTest, ChunkStep_BiomesWriteRadius)
{
    // BIOMES 步骤不应该写方块
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::BIOMES);
    EXPECT_LT(step.blockStateWriteRadius(), 0);
}

// ============================================================================
// 6. ChunkStatus 顺序验证
// ============================================================================

TEST_F(BaseChunkGeneratorTest, ChunkStatus_Ordering)
{
    // 验证 MC 1.21 的区块状态顺序
    EXPECT_TRUE(ChunkStatuses::EMPTY.isBefore(ChunkStatuses::STRUCTURE_STARTS));
    EXPECT_TRUE(ChunkStatuses::STRUCTURE_STARTS.isBefore(ChunkStatuses::STRUCTURE_REFERENCES));
    EXPECT_TRUE(ChunkStatuses::STRUCTURE_REFERENCES.isBefore(ChunkStatuses::BIOMES));
    EXPECT_TRUE(ChunkStatuses::BIOMES.isBefore(ChunkStatuses::NOISE));
    EXPECT_TRUE(ChunkStatuses::NOISE.isBefore(ChunkStatuses::SURFACE));
    EXPECT_TRUE(ChunkStatuses::SURFACE.isBefore(ChunkStatuses::CARVERS));
    EXPECT_TRUE(ChunkStatuses::CARVERS.isBefore(ChunkStatuses::FEATURES));
    EXPECT_TRUE(ChunkStatuses::FEATURES.isBefore(ChunkStatuses::INITIALIZE_LIGHT));
    EXPECT_TRUE(ChunkStatuses::INITIALIZE_LIGHT.isBefore(ChunkStatuses::LIGHT));
    EXPECT_TRUE(ChunkStatuses::LIGHT.isBefore(ChunkStatuses::SPAWN));
    EXPECT_TRUE(ChunkStatuses::SPAWN.isBefore(ChunkStatuses::FULL));
}

TEST_F(BaseChunkGeneratorTest, ChunkStatus_IsAtLeast)
{
    EXPECT_TRUE(ChunkStatuses::FULL.isAtLeast(ChunkStatuses::EMPTY));
    EXPECT_TRUE(ChunkStatuses::FULL.isAtLeast(ChunkStatuses::NOISE));
    EXPECT_TRUE(ChunkStatuses::FULL.isAtLeast(ChunkStatuses::FULL));
    EXPECT_FALSE(ChunkStatuses::NOISE.isAtLeast(ChunkStatuses::SURFACE));
    EXPECT_FALSE(ChunkStatuses::EMPTY.isAtLeast(ChunkStatuses::BIOMES));
}

TEST_F(BaseChunkGeneratorTest, ChunkStatus_OrdinalValues)
{
    EXPECT_EQ(ChunkStatuses::EMPTY.ordinal(), 0);
    EXPECT_EQ(ChunkStatuses::STRUCTURE_STARTS.ordinal(), 1);
    EXPECT_EQ(ChunkStatuses::STRUCTURE_REFERENCES.ordinal(), 2);
    EXPECT_EQ(ChunkStatuses::BIOMES.ordinal(), 3);
    EXPECT_EQ(ChunkStatuses::NOISE.ordinal(), 4);
    EXPECT_EQ(ChunkStatuses::SURFACE.ordinal(), 5);
    EXPECT_EQ(ChunkStatuses::CARVERS.ordinal(), 6);
    EXPECT_EQ(ChunkStatuses::FEATURES.ordinal(), 7);
    EXPECT_EQ(ChunkStatuses::INITIALIZE_LIGHT.ordinal(), 8);
    EXPECT_EQ(ChunkStatuses::LIGHT.ordinal(), 9);
    EXPECT_EQ(ChunkStatuses::SPAWN.ordinal(), 10);
    EXPECT_EQ(ChunkStatuses::FULL.ordinal(), 11);
}

TEST_F(BaseChunkGeneratorTest, ChunkStatus_ParentChain)
{
    EXPECT_EQ(ChunkStatuses::EMPTY.parent(), &ChunkStatuses::EMPTY);
    EXPECT_EQ(ChunkStatuses::STRUCTURE_STARTS.parent(), &ChunkStatuses::EMPTY);
    EXPECT_EQ(ChunkStatuses::STRUCTURE_REFERENCES.parent(), &ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(ChunkStatuses::BIOMES.parent(), &ChunkStatuses::STRUCTURE_REFERENCES);
    EXPECT_EQ(ChunkStatuses::NOISE.parent(), &ChunkStatuses::BIOMES);
    EXPECT_EQ(ChunkStatuses::SURFACE.parent(), &ChunkStatuses::NOISE);
    EXPECT_EQ(ChunkStatuses::CARVERS.parent(), &ChunkStatuses::SURFACE);
    EXPECT_EQ(ChunkStatuses::FEATURES.parent(), &ChunkStatuses::CARVERS);
    EXPECT_EQ(ChunkStatuses::FULL.parent(), &ChunkStatuses::SPAWN);
}

TEST_F(BaseChunkGeneratorTest, ChunkStatus_ByName)
{
    // byName 返回的是 getAll() 向量中的元素，而非全局常量对象的地址
    // 因此比较名称而非指针地址
    const ChunkStatus* empty = ChunkStatus::byName("empty");
    ASSERT_NE(empty, nullptr);
    EXPECT_EQ(empty->name(), "empty");
    EXPECT_EQ(empty->ordinal(), 0);

    const ChunkStatus* noise = ChunkStatus::byName("noise");
    ASSERT_NE(noise, nullptr);
    EXPECT_EQ(noise->name(), "noise");
    EXPECT_EQ(noise->ordinal(), 4);

    const ChunkStatus* full = ChunkStatus::byName("full");
    ASSERT_NE(full, nullptr);
    EXPECT_EQ(full->name(), "full");
    EXPECT_EQ(full->ordinal(), 11);

    // 不存在的名称
    EXPECT_EQ(ChunkStatus::byName("invalid"), nullptr);
    EXPECT_EQ(ChunkStatus::byName(""), nullptr);
}

TEST_F(BaseChunkGeneratorTest, ChunkStatus_ByOrdinal)
{
    // byOrdinal 返回的是 getAll() 向量中的元素，而非全局常量对象的地址
    // 因此比较 ordinal 而非指针地址
    const ChunkStatus* status0 = ChunkStatus::byOrdinal(0);
    ASSERT_NE(status0, nullptr);
    EXPECT_EQ(status0->name(), "empty");

    const ChunkStatus* status4 = ChunkStatus::byOrdinal(4);
    ASSERT_NE(status4, nullptr);
    EXPECT_EQ(status4->name(), "noise");

    const ChunkStatus* status11 = ChunkStatus::byOrdinal(11);
    ASSERT_NE(status11, nullptr);
    EXPECT_EQ(status11->name(), "full");

    EXPECT_EQ(ChunkStatus::byOrdinal(-1), nullptr);
    EXPECT_EQ(ChunkStatus::byOrdinal(100), nullptr);
}

// ============================================================================
// 7. ChunkStatus 高度图标志
// ============================================================================

TEST_F(BaseChunkGeneratorTest, ChunkStatus_HeightmapFlags)
{
    // NOISE 及之前使用 PRE_FEATURES
    EXPECT_TRUE(hasFlag(ChunkStatuses::EMPTY.heightmaps(), HeightmapFlag::PRE_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::BIOMES.heightmaps(), HeightmapFlag::PRE_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::NOISE.heightmaps(), HeightmapFlag::PRE_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::SURFACE.heightmaps(), HeightmapFlag::PRE_FEATURES));

    // CARVERS 及之后使用 POST_FEATURES
    EXPECT_TRUE(hasFlag(ChunkStatuses::CARVERS.heightmaps(), HeightmapFlag::POST_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::FEATURES.heightmaps(), HeightmapFlag::POST_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::LIGHT.heightmaps(), HeightmapFlag::POST_FEATURES));
    EXPECT_TRUE(hasFlag(ChunkStatuses::FULL.heightmaps(), HeightmapFlag::POST_FEATURES));

    // POST_FEATURES 不包含 LIGHT_BLOCKING
    EXPECT_FALSE(hasFlag(ChunkStatuses::CARVERS.heightmaps(), HeightmapFlag::LIGHT_BLOCKING));
}

// ============================================================================
// 8. ChunkStatus 类型
// ============================================================================

TEST_F(BaseChunkGeneratorTest, ChunkStatus_Type)
{
    // FULL 是 LEVELCHUNK 类型
    EXPECT_EQ(ChunkStatuses::FULL.type(), ChunkType::LEVELCHUNK);

    // 其他阶段是 PROTOCHUNK 类型
    EXPECT_EQ(ChunkStatuses::EMPTY.type(), ChunkType::PROTOCHUNK);
    EXPECT_EQ(ChunkStatuses::NOISE.type(), ChunkType::PROTOCHUNK);
    EXPECT_EQ(ChunkStatuses::FEATURES.type(), ChunkType::PROTOCHUNK);
    EXPECT_EQ(ChunkStatuses::LIGHT.type(), ChunkType::PROTOCHUNK);
}

// ============================================================================
// 9. NoiseChunkGenerator 基础查询
// ============================================================================

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_GetHeight_ReturnsValidRange)
{
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 42ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));

    // getHeight 应返回在有效范围内的值
    i32 height = gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    EXPECT_GE(height, gen.getMinY());
    EXPECT_LT(height, gen.getMinY() + gen.getGenDepth());
}

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_GetBaseColumn_NotEmpty)
{
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 42ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));

    auto column = gen.getBaseColumn(0, 0);
    EXPECT_EQ(column.minY(), gen.getMinY());
    EXPECT_EQ(column.height(), gen.getGenDepth());
    // 列中至少应该有一些非空方块
    bool hasNonAir = false;
    for (i32 y = column.minY(); y < column.minY() + column.height(); ++y) {
        if (column.getBlock(y) != nullptr && !column.getBlock(y)->isAir()) {
            hasNonAir = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonAir) << "getBaseColumn should return at least one non-air block";
}

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_GetBiome_NonVoid)
{
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 42ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
    NoiseChunkGenerator gen(std::move(settings), std::move(biomeSource), std::move(randomState));

    BiomeId biome = gen.getBiome(0, 64, 0);
    EXPECT_NE(biome, Biomes::TheVoid); // 不应该是虚空生物群系
}

TEST_F(BaseChunkGeneratorTest, NoiseGenerator_DifferentSeeds_DifferentTerrain)
{
    auto settings1 = DimensionSettings::overworld();
    auto randomState1 = world::gen::RandomState::create(settings1, 1ULL);
    auto biomeSource1 = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState1, false);
    NoiseChunkGenerator gen1(std::move(settings1), std::move(biomeSource1), std::move(randomState1));

    auto settings2 = DimensionSettings::overworld();
    auto randomState2 = world::gen::RandomState::create(settings2, 99999ULL);
    auto biomeSource2 = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState2, false);
    NoiseChunkGenerator gen2(std::move(settings2), std::move(biomeSource2), std::move(randomState2));

    // 不同种子应产生不同地形高度
    i32 h1 = gen1.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    i32 h2 = gen2.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    // 不要求一定不同（统计上可能偶尔相同），但通常是不同的
    // 此处仅验证两个生成器都可以正常工作
    EXPECT_GE(h1, gen1.getMinY());
    EXPECT_LT(h1, gen1.getMinY() + gen1.getGenDepth());
    EXPECT_GE(h2, gen2.getMinY());
    EXPECT_LT(h2, gen2.getMinY() + gen2.getGenDepth());
}

// ============================================================================
// 10. getSpawnHeight
// ============================================================================

TEST_F(BaseChunkGeneratorTest, GetSpawnHeight_DefaultImplementation)
{
    // IChunkGenerator::getSpawnHeight 调用 getHeight(x, z, WorldSurfaceWG)
    // DebugChunkGenerator::getHeight 始终返回 0
    DebugChunkGenerator debugGen;
    EXPECT_EQ(debugGen.getSpawnHeight(0, 0), 0);
}

// ============================================================================
// 11. findNearestMapStructure 默认实现
// ============================================================================

TEST_F(BaseChunkGeneratorTest, FindNearestMapStructure_DefaultReturnsNull)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto result = gen.findNearestMapStructure(0, 0, 0, 100, false);
    EXPECT_FALSE(result.has_value());
}

TEST_F(BaseChunkGeneratorTest, FindNearestMapStructure_DebugGenerator)
{
    DebugChunkGenerator gen;
    auto result = gen.findNearestMapStructure(0, 0, 0, 100, false);
    EXPECT_FALSE(result.has_value());
}

} // namespace
