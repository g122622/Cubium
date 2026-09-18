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

#include "common/util/cache/Long2FloatLRUCache.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/biome/climate/SpawnFinder.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::biome;
using namespace mc::world::biome::climate;
using namespace mc::world::biome::source;

// ============================================================================
// Long2FloatLRUCache 测试
// ============================================================================

TEST(Long2FloatLRUCache, EmptyCacheReturnsNaN)
{
    Long2FloatLRUCache cache(1024);
    EXPECT_TRUE(std::isnan(cache.get(0)));
    EXPECT_TRUE(std::isnan(cache.get(12345)));
}

TEST(Long2FloatLRUCache, PutAndGet)
{
    Long2FloatLRUCache cache(1024);
    cache.put(100, 0.5f);
    EXPECT_FLOAT_EQ(cache.get(100), 0.5f);
    cache.put(200, 1.5f);
    EXPECT_FLOAT_EQ(cache.get(200), 1.5f);
    EXPECT_TRUE(std::isnan(cache.get(300)));
}

TEST(Long2FloatLRUCache, UpdateExistingKey)
{
    Long2FloatLRUCache cache(1024);
    cache.put(100, 0.5f);
    EXPECT_FLOAT_EQ(cache.get(100), 0.5f);
    cache.put(100, 0.8f);
    EXPECT_FLOAT_EQ(cache.get(100), 0.8f);
    EXPECT_EQ(cache.size(), 1);
}

TEST(Long2FloatLRUCache, EvictionWhenFull)
{
    Long2FloatLRUCache cache(3);
    cache.put(1, 0.1f);
    cache.put(2, 0.2f);
    cache.put(3, 0.3f);
    EXPECT_EQ(cache.size(), 3);

    // 插入第4个应该淘汰最旧的（key=1）
    cache.put(4, 0.4f);
    EXPECT_EQ(cache.size(), 3);
    EXPECT_TRUE(std::isnan(cache.get(1))); // 被淘汰
    EXPECT_FLOAT_EQ(cache.get(2), 0.2f);   // 仍在
    EXPECT_FLOAT_EQ(cache.get(3), 0.3f);
    EXPECT_FLOAT_EQ(cache.get(4), 0.4f);
}

TEST(Long2FloatLRUCache, Clear)
{
    Long2FloatLRUCache cache(1024);
    cache.put(1, 0.1f);
    cache.put(2, 0.2f);
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
    EXPECT_TRUE(std::isnan(cache.get(1)));
}

TEST(Long2FloatLRUCache, PackBlockPos)
{
    // 测试 BlockPos 打包/解包正确性
    i64 key = Long2FloatLRUCache::packBlockPos(100, 64, -200);
    EXPECT_NE(key, 0);

    // 不同位置应该产生不同的键
    i64 key2 = Long2FloatLRUCache::packBlockPos(101, 64, -200);
    EXPECT_NE(key, key2);

    i64 key3 = Long2FloatLRUCache::packBlockPos(100, 65, -200);
    EXPECT_NE(key, key3);

    i64 key4 = Long2FloatLRUCache::packBlockPos(100, 64, -201);
    EXPECT_NE(key, key4);
}

TEST(Long2FloatLRUCache, MC1024Capacity)
{
    // MC 使用 1024 容量
    Long2FloatLRUCache cache(1024);
    for (i32 i = 0; i < 1024; ++i) {
        cache.put(i, static_cast<f32>(i) * 0.1f);
    }
    EXPECT_EQ(cache.size(), 1024);

    // 添加第 1025 个应该淘汰第 1 个
    cache.put(1024, 99.9f);
    EXPECT_TRUE(std::isnan(cache.get(0)));
    EXPECT_FLOAT_EQ(cache.get(1024), 99.9f);
}

// ============================================================================
// Biome 温度缓存测试
// ============================================================================

TEST(BiomeTemperatureCache, GetTemperatureCachesResult)
{
    // 测试温度缓存：调用两次 getTemperature 应该返回相同的值
    BiomeRegistry::instance().initialize();
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);

    // 清除缓存以确保干净状态
    Biome::clearTemperatureCache();

    constexpr i32 seaLevel = 63;
    const f32 temp1 = plains.getTemperature(100, 64, 200, seaLevel);
    const f32 temp2 = plains.getTemperature(100, 64, 200, seaLevel);
    EXPECT_FLOAT_EQ(temp1, temp2);
}

TEST(BiomeTemperatureCache, GetTemperatureMatchesGetHeightAdjustedTemperature)
{
    // getTemperature 应该返回与 getHeightAdjustedTemperature 相同的值
    BiomeRegistry::instance().initialize();
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    Biome::clearTemperatureCache();

    constexpr i32 seaLevel = 63;
    // 在海平面附近（不需要高度调整）
    const f32 cached = plains.getTemperature(100, 64, 200, seaLevel);
    const f32 direct = plains.getHeightAdjustedTemperature(100, 64, 200, seaLevel);
    EXPECT_FLOAT_EQ(cached, direct);
}

TEST(BiomeTemperatureCache, HeightAdjustedTemperatureDecreasesAtHighAltitude)
{
    // 高海拔温度应该低于低海拔温度
    BiomeRegistry::instance().initialize();
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    Biome::clearTemperatureCache();

    constexpr i32 seaLevel = 63;
    const f32 lowTemp = plains.getHeightAdjustedTemperature(100, seaLevel, 200, seaLevel);
    const f32 highTemp = plains.getHeightAdjustedTemperature(100, seaLevel + 100, 200, seaLevel);
    EXPECT_LT(highTemp, lowTemp);
}

TEST(BiomeTemperatureCache, TemperatureBelowSeaLevelUnchanged)
{
    // 海平面以下温度不应改变
    BiomeRegistry::instance().initialize();
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    Biome::clearTemperatureCache();

    constexpr i32 seaLevel = 63;
    const f32 baseTemp = plains.getBaseTemperature();
    const f32 lowTemp = plains.getHeightAdjustedTemperature(100, seaLevel, 200, seaLevel);
    EXPECT_FLOAT_EQ(lowTemp, baseTemp);
}

TEST(BiomeTemperatureCache, ClearCacheWorks)
{
    BiomeRegistry::instance().initialize();
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    Biome::clearTemperatureCache();

    constexpr i32 seaLevel = 63;
    plains.getTemperature(100, 64, 200, seaLevel);
    // 清除后再次调用应该正常工作
    Biome::clearTemperatureCache();
    const f32 temp = plains.getTemperature(100, 64, 200, seaLevel);
    EXPECT_FALSE(std::isnan(temp));
}

TEST(BiomeTemperatureCache, FrozenBiomeTemperatureModifier)
{
    // FrozenOcean 应该使用 TemperatureModifier::Frozen
    BiomeRegistry::instance().initialize();
    const Biome& frozenOcean = BiomeRegistry::instance().get(Biomes::FrozenOcean);
    Biome::clearTemperatureCache();

    constexpr i32 seaLevel = 63;
    // Frozen 修饰符可能返回 0.2 或基础温度，取决于噪声
    const f32 temp = frozenOcean.getTemperature(0, 64, 0, seaLevel);
    // 温度应该在合理范围内（0.0 或 0.2 或基础温度附近）
    EXPECT_GE(temp, -1.0f);
    EXPECT_LE(temp, 2.0f);
}

// ============================================================================
// SpawnFinder 测试
// ============================================================================

TEST(SpawnFinder, EmptySpawnTargetReturnsOrigin)
{
    // 空的 spawn target 应该返回 (0, 0, 0)
    // SpawnFinder::findSpawnPosition 需要 Sampler（无法在此构造），
    // 该路径由 SpawnTargetIntegrationTest 覆盖，此处仅占位。
}

TEST(SpawnFinder, ResultStructInitialization)
{
    SpawnFinder::Result result{0, 0, 0};
    EXPECT_EQ(result.x, 0);
    EXPECT_EQ(result.z, 0);
    EXPECT_EQ(result.fitness, 0);
}

// ============================================================================
// FixedBiomeSource 测试
// ============================================================================

TEST(FixedBiomeSource, AlwaysReturnsFixedBiome)
{
    FixedBiomeSource source(12345ULL, Biomes::Plains);
    EXPECT_EQ(source.getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(source.getNoiseBiome(100, -64, 200), Biomes::Plains);
    EXPECT_EQ(source.getNoiseBiome(-1000, 300, 5000), Biomes::Plains);
}

TEST(FixedBiomeSource, FixedBiomeId)
{
    FixedBiomeSource source(0ULL, Biomes::Desert);
    EXPECT_EQ(source.fixedBiomeId(), Biomes::Desert);
}

TEST(FixedBiomeSource, PossibleBiomesContainsOnlyFixedBiome)
{
    FixedBiomeSource source(0ULL, Biomes::Forest);
    const auto& biomes = source.possibleBiomes();
    EXPECT_EQ(biomes.size(), 1u);
    EXPECT_EQ(biomes[0], Biomes::Forest);
}

TEST(FixedBiomeSource, GetBiomesWithinReturnsFixedBiome)
{
    FixedBiomeSource source(0ULL, Biomes::Taiga);
    auto biomes = source.getBiomesWithin(0, 0, 0, 100);
    EXPECT_EQ(biomes.size(), 1u);
    EXPECT_TRUE(biomes.count(Biomes::Taiga));
}

TEST(FixedBiomeSource, FindBiomeWithMatchingPredicate)
{
    FixedBiomeSource source(0ULL, Biomes::Ocean);
    mc::math::Random random(12345ULL);
    auto result = source.findBiome(0, 64, 0, 100, 10, [](BiomeId id) { return id == Biomes::Ocean; }, random, true);
    EXPECT_TRUE(result.has_value());
}

TEST(FixedBiomeSource, FindBiomeWithNonMatchingPredicate)
{
    FixedBiomeSource source(0ULL, Biomes::Ocean);
    mc::math::Random random(12345ULL);
    auto result = source.findBiome(0, 64, 0, 100, 10, [](BiomeId id) { return id == Biomes::Desert; }, random, true);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// BiomeTags 新增标签测试
// ============================================================================

TEST(BiomeTags, DimensionTagsInitialized)
{
    BiomeTags::initialize();

    // IS_OVERWORLD 应该包含 Plains
    EXPECT_TRUE(BiomeTags::IS_OVERWORLD().contains(Biomes::Plains));
    EXPECT_TRUE(BiomeTags::IS_OVERWORLD().contains(Biomes::Ocean));

    // IS_NETHER 应该包含下界生物群系
    EXPECT_TRUE(BiomeTags::IS_NETHER().contains(Biomes::NetherWastes));
    EXPECT_TRUE(BiomeTags::IS_NETHER().contains(Biomes::BasaltDeltas));

    // IS_END 应该包含末地生物群系
    EXPECT_TRUE(BiomeTags::IS_END().contains(Biomes::TheEnd));
    EXPECT_TRUE(BiomeTags::IS_END().contains(Biomes::EndHighlands));
}

TEST(BiomeTags, TerrainTypeTagsInitialized)
{
    BiomeTags::initialize();

    // IS_DEEP_OCEAN
    EXPECT_TRUE(BiomeTags::IS_DEEP_OCEAN().contains(Biomes::DeepOcean));
    EXPECT_TRUE(BiomeTags::IS_DEEP_OCEAN().contains(Biomes::DeepFrozenOcean));
    EXPECT_FALSE(BiomeTags::IS_DEEP_OCEAN().contains(Biomes::Ocean));

    // IS_BEACH
    EXPECT_TRUE(BiomeTags::IS_BEACH().contains(Biomes::Beach));
    EXPECT_TRUE(BiomeTags::IS_BEACH().contains(Biomes::SnowyBeach));
    EXPECT_TRUE(BiomeTags::IS_BEACH().contains(Biomes::StoneShore));

    // IS_MOUNTAIN
    EXPECT_TRUE(BiomeTags::IS_MOUNTAIN().contains(Biomes::JaggedPeaks));
    EXPECT_TRUE(BiomeTags::IS_MOUNTAIN().contains(Biomes::StonyPeaks));

    // IS_TAIGA
    EXPECT_TRUE(BiomeTags::IS_TAIGA().contains(Biomes::Taiga));
    EXPECT_TRUE(BiomeTags::IS_TAIGA().contains(Biomes::SnowyTaiga));

    // IS_JUNGLE
    EXPECT_TRUE(BiomeTags::IS_JUNGLE().contains(Biomes::Jungle));
    EXPECT_TRUE(BiomeTags::IS_JUNGLE().contains(Biomes::BambooJungle));

    // IS_FOREST
    EXPECT_TRUE(BiomeTags::IS_FOREST().contains(Biomes::Forest));
    EXPECT_TRUE(BiomeTags::IS_FOREST().contains(Biomes::DarkForest));
    EXPECT_TRUE(BiomeTags::IS_FOREST().contains(Biomes::CherryGrove));

    // IS_SAVANNA
    EXPECT_TRUE(BiomeTags::IS_SAVANNA().contains(Biomes::Savanna));
    EXPECT_TRUE(BiomeTags::IS_SAVANNA().contains(Biomes::ShatteredSavanna));

    // IS_BADLANDS
    EXPECT_TRUE(BiomeTags::IS_BADLANDS().contains(Biomes::Badlands));
    EXPECT_TRUE(BiomeTags::IS_BADLANDS().contains(Biomes::ErodedBadlands));

    // IS_MUSHROOM
    EXPECT_TRUE(BiomeTags::IS_MUSHROOM().contains(Biomes::MushroomFields));
}

TEST(BiomeTags, GameplayTagsInitialized)
{
    BiomeTags::initialize();

    // 骨粉珊瑚
    EXPECT_TRUE(BiomeTags::PRODUCES_CORALS_FROM_BONEMEAL().contains(Biomes::WarmOcean));

    // 地图水标记
    EXPECT_TRUE(BiomeTags::WATER_ON_MAP_OUTLINES().contains(Biomes::Ocean));
    EXPECT_TRUE(BiomeTags::WATER_ON_MAP_OUTLINES().contains(Biomes::Swamp));

    // 虚空
    EXPECT_TRUE(BiomeTags::IS_VOID().contains(Biomes::TheVoid));
}

TEST(BiomeTags, CrossDimensionExclusivity)
{
    BiomeTags::initialize();

    // Plains 不应在 IS_NETHER 或 IS_END 中
    EXPECT_FALSE(BiomeTags::IS_NETHER().contains(Biomes::Plains));
    EXPECT_FALSE(BiomeTags::IS_END().contains(Biomes::Plains));

    // NetherWastes 不应在 IS_OVERWORLD 或 IS_END 中
    EXPECT_FALSE(BiomeTags::IS_OVERWORLD().contains(Biomes::NetherWastes));
    EXPECT_FALSE(BiomeTags::IS_END().contains(Biomes::NetherWastes));

    // TheEnd 不应在 IS_OVERWORLD 或 IS_NETHER 中
    EXPECT_FALSE(BiomeTags::IS_OVERWORLD().contains(Biomes::TheEnd));
    EXPECT_FALSE(BiomeTags::IS_NETHER().contains(Biomes::TheEnd));
}

// ============================================================================
// TheVoid 生物群系测试
// ============================================================================

TEST(BiomeTheVoid, BasicProperties)
{
    BiomeRegistry::instance().initialize();
    const Biome& voidBiome = BiomeRegistry::instance().get(Biomes::TheVoid);
    EXPECT_EQ(voidBiome.id(), Biomes::TheVoid);
    EXPECT_EQ(voidBiome.name(), "the_void");
    EXPECT_FALSE(voidBiome.hasPrecipitation());
    EXPECT_FLOAT_EQ(voidBiome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(voidBiome.climate().downfall, 0.5f);
}

TEST(BiomeTheVoid, NoPrecipitation)
{
    BiomeRegistry::instance().initialize();
    const Biome& voidBiome = BiomeRegistry::instance().get(Biomes::TheVoid);
    EXPECT_FALSE(voidBiome.hasPrecipitation());

    // 虚空不应该有雪生成
    constexpr i32 seaLevel = 63;
    EXPECT_FALSE(voidBiome.doesSnowGenerate(0, 64, 0, seaLevel));
}
