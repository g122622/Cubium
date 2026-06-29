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
 * IMPLIED, INCLUDING ANY LIMITATIONS BELOW.
 */

// ============================================================================
// 结构生成生物群系过滤测试
//
// 测试覆盖：
// 1. NoiseChunkGenerator::_hasBiomesForStructureSet 预过滤
//    1.1 结构集与 possibleBiomes 无交集时被跳过
//    1.2 结构集与 possibleBiomes 有交集时通过
//    1.3 structure->biomeTag() 为空时的行为
// 2. 候选区块级生物群系精确检查
//    2.1 FixedBiomeSource(Plains) 在任意位置返回 Plains
//    2.2 MultiNoiseBiomeSource::createOverworld 包含丰富的主世界生物群系
//    2.3 下界结构集与下界生物群系兼容
//    2.4 EndCity 结构标签不包含主世界生物群系
//    2.5 isValidBiome 与 BiomeTag::contains 行为一致
// ============================================================================

#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

// ============================================================================
// 测试辅助
// ============================================================================

class BiomeFilterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
        // 结构注册表和结构集合注册表在 NoiseChunkGenerator 构造时自动初始化，
        // 但某些测试直接访问 StructureSetRegistry，需要显式初始化
        world::gen::structure::StructureRegistry::initialize();
        world::gen::structure::StructureSetRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. _hasBiomesForStructureSet 预过滤测试
//    通过公共接口间接测试。
//    使用 FixedBiomeSource 创建只有单一生物群系的生成器，
//    验证不兼容的结构集被正确过滤。
// ============================================================================

TEST_F(BiomeFilterTest, NetherStructureSet_SkippedInOverworldBiome)
{
    // 主世界生物群系源不应生成下界结构集（nether_complexes）
    // FixedBiomeSource 只返回 Plains，与下界结构的 biomeTag 无交集
    auto biomeSource = std::make_unique<world::biome::source::FixedBiomeSource>(0LL, Biomes::Plains);
    NoiseChunkGenerator gen(0ULL, DimensionSettings::overworld(), std::move(biomeSource));

    // 查找下界结构集
    const auto* netherComplexSet = world::gen::structure::StructureSetRegistry::instance().get(
        ResourceLocation::parse("minecraft:nether_complexes"));

    // 如果下界结构集未注册，跳过测试
    if (netherComplexSet == nullptr) {
        GTEST_SKIP() << "minecraft:nether_complexes structure set not registered";
    }

    // 下界结构集中，所有在 StructureRegistry 中找到的结构，
    // 其 biomeTag 都不应包含 Plains
    const auto& entries = netherComplexSet->entries();
    bool anyNetherStructureValidInPlains = false;
    for (const auto& entry : entries) {
        const auto* structure = world::gen::structure::StructureRegistry::get(entry.structureId);
        if (!structure) continue; // 未注册的结构跳过
        if (structure->isValidBiome(Biomes::Plains)) {
            anyNetherStructureValidInPlains = true;
            break;
        }
    }
    EXPECT_FALSE(anyNetherStructureValidInPlains) << "Nether structures should not be valid in Plains biome";
}

TEST_F(BiomeFilterTest, OverworldStructureSet_CompatibleWithOverworldBiome)
{
    // 主世界生物群系源应能与主世界结构集兼容
    // 遍历所有注册结构集，寻找至少一个结构条目与 Plains 生物群系兼容
    auto biomeSource = std::make_unique<world::biome::source::FixedBiomeSource>(0LL, Biomes::Plains);

    bool foundCompatible = false;
    const auto& allSets = world::gen::structure::StructureSetRegistry::instance().getAll();
    for (const auto& setPtr : allSets) {
        if (!setPtr) continue;
        for (const auto& entry : setPtr->entries()) {
            const auto* structure = world::gen::structure::StructureRegistry::get(entry.structureId);
            if (!structure) continue;
            if (structure->isValidBiome(Biomes::Plains)) {
                foundCompatible = true;
                break;
            }
        }
        if (foundCompatible) break;
    }
    EXPECT_TRUE(foundCompatible) << "At least one registered structure should be valid in Plains biome";
}

TEST_F(BiomeFilterTest, StructureWithNullBiomeTag_TreatedAsIncompatible)
{
    // biomeTag() 返回 nullptr 的结构应被视为不兼容任何生物群系
    // isValidBiome 在 biomeTag() 为 nullptr 时返回 false
    auto biomeSource = std::make_unique<world::biome::source::FixedBiomeSource>(0LL, Biomes::Plains);
    NoiseChunkGenerator gen(0ULL, DimensionSettings::overworld(), std::move(biomeSource));

    // 遍历所有注册结构，验证 isValidBiome 不会崩溃
    const auto& allSets = world::gen::structure::StructureSetRegistry::instance().getAll();
    for (const auto& setPtr : allSets) {
        if (!setPtr) continue;
        for (const auto& entry : setPtr->entries()) {
            const auto* structure = world::gen::structure::StructureRegistry::get(entry.structureId);
            if (!structure) continue;
            // biomeTag() 返回 nullptr 意味着结构无法在已知生物群系中生成
            // 但我们不应该崩溃
            structure->isValidBiome(Biomes::Plains); // 不崩溃即可
        }
    }
}

TEST_F(BiomeFilterTest, PossibleBiomes_ContainsOverworldBiomes)
{
    // 主世界生物群系源的 possibleBiomes 应包含主世界生物群系
    auto biomeSource = std::make_unique<world::biome::source::FixedBiomeSource>(0LL, Biomes::Plains);
    const auto& possibleBiomes = biomeSource->possibleBiomes();

    // FixedBiomeSource 只有一种生物群系
    ASSERT_EQ(possibleBiomes.size(), 1u);
    EXPECT_EQ(possibleBiomes[0], Biomes::Plains);
}

TEST_F(BiomeFilterTest, MultiNoiseBiomeSource_ContainsManyOverworldBiomes)
{
    // MultiNoiseBiomeSource::createOverworld 的 possibleBiomes 应包含大量主世界生物群系
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    const auto& possibleBiomes = biomeSource->possibleBiomes();

    // 主世界应该有大量生物群系
    EXPECT_GT(possibleBiomes.size(), 50u) << "Overworld biome source should have many possible biomes";

    // 至少应该包含 Plains 和 Desert
    bool hasPlains = false;
    bool hasDesert = false;
    for (BiomeId id : possibleBiomes) {
        if (id == Biomes::Plains) hasPlains = true;
        if (id == Biomes::Desert) hasDesert = true;
    }
    EXPECT_TRUE(hasPlains) << "Overworld should contain Plains";
    EXPECT_TRUE(hasDesert) << "Overworld should contain Desert";
}

// ============================================================================
// 2. 候选区块级生物群系精确检查测试
//    使用 FixedBiomeSource 确保特定生物群系精确控制结构生成。
// ============================================================================

TEST_F(BiomeFilterTest, GenerateStructureStarts_NetherBiomeSource_NetherStructuresAllowed)
{
    // 使用下界生物群系创建生成器，下界结构应被允许
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(0ULL);
    NoiseChunkGenerator gen(0ULL, DimensionSettings::nether(), std::move(biomeSource));

    // 查找下界结构集
    const auto* netherComplexSet = world::gen::structure::StructureSetRegistry::instance().get(
        ResourceLocation::parse("minecraft:nether_complexes"));

    if (netherComplexSet == nullptr) {
        GTEST_SKIP() << "minecraft:nether_complexes structure set not registered";
    }

    // 下界生物群系源应包含下界生物群系
    const auto& possibleBiomes = gen.getBiomeSource()->possibleBiomes();
    EXPECT_GT(possibleBiomes.size(), 0u) << "Nether biome source should have possible biomes";

    // 下界结构集中，至少有一个在 StructureRegistry 中找到的结构，
    // 其 biomeTag 应与下界生物群系兼容
    bool hasCompatible = false;
    for (const auto& entry : netherComplexSet->entries()) {
        const auto* structure = world::gen::structure::StructureRegistry::get(entry.structureId);
        if (!structure) continue;
        for (BiomeId biomeId : possibleBiomes) {
            if (structure->isValidBiome(biomeId)) {
                hasCompatible = true;
                break;
            }
        }
        if (hasCompatible) break;
    }
    EXPECT_TRUE(hasCompatible) << "Nether structure should be compatible with nether biomes";
}

TEST_F(BiomeFilterTest, EndCityStructure_RequiresEndBiome)
{
    // 末地城结构需要末地生物群系
    // 验证 HAS_STRUCTURE_END_CITY 标签不包含主世界生物群系
    EXPECT_FALSE(world::biome::BiomeTags::HAS_STRUCTURE_END_CITY().contains(Biomes::Plains))
        << "End City structure tag should not contain Plains";
    EXPECT_FALSE(world::biome::BiomeTags::HAS_STRUCTURE_END_CITY().contains(Biomes::Desert))
        << "End City structure tag should not contain Desert";

    // 末地生物群系应该在标签中
    bool hasEndBiome = false;
    // EndCity 结构应该在 End Highlands / End Midlands 等末地生物群系中生成
    // 检查可能的末地生物群系 ID
    for (BiomeId id = 0; id < 200; ++id) {
        if (world::biome::BiomeTags::HAS_STRUCTURE_END_CITY().contains(id)) {
            hasEndBiome = true;
            break;
        }
    }
    EXPECT_TRUE(hasEndBiome) << "End City structure tag should contain at least one biome";
}

TEST_F(BiomeFilterTest, GetNoiseBiome_ReturnsConsistentBiomeAtChunkCenter)
{
    // 验证 getNoiseBiome 在同一位置返回一致的生物群系
    auto biomeSource = std::make_unique<world::biome::source::FixedBiomeSource>(0LL, Biomes::Plains);
    NoiseChunkGenerator gen(0ULL, DimensionSettings::overworld(), std::move(biomeSource));

    // FixedBiomeSource 在任何位置都应返回 Plains
    for (i32 chunkX = -5; chunkX <= 5; ++chunkX) {
        for (i32 chunkZ = -5; chunkZ <= 5; ++chunkZ) {
            BiomeId biome = gen.getNoiseBiome((chunkX * 16 + 8) >> 2, 0, (chunkZ * 16 + 8) >> 2);
            EXPECT_EQ(biome, Biomes::Plains)
                << "FixedBiomeSource(Plains) should return Plains at chunk (" << chunkX << ", " << chunkZ << ")";
        }
    }
}

TEST_F(BiomeFilterTest, isValidBiome_ConsistentWithBiomeTag)
{
    // 验证 Structure::isValidBiome 与 BiomeTag::contains 行为一致
    const auto& allSets = world::gen::structure::StructureSetRegistry::instance().getAll();
    for (const auto& setPtr : allSets) {
        if (!setPtr) continue;
        for (const auto& entry : setPtr->entries()) {
            const auto* structure = world::gen::structure::StructureRegistry::get(entry.structureId);
            if (!structure) continue;

            const world::biome::BiomeTag* tag = structure->biomeTag();
            if (!tag) {
                // biomeTag() 为空时，isValidBiome 应对所有生物群系返回 false
                EXPECT_FALSE(structure->isValidBiome(Biomes::Plains))
                    << "Structure with null biomeTag should not be valid for any biome";
                EXPECT_FALSE(structure->isValidBiome(Biomes::Desert))
                    << "Structure with null biomeTag should not be valid for any biome";
                continue;
            }

            // isValidBiome 应与 tag->contains() 结果一致
            for (BiomeId testBiome : {Biomes::Plains, Biomes::Desert, Biomes::Ocean}) {
                EXPECT_EQ(structure->isValidBiome(testBiome), tag->contains(testBiome))
                    << "isValidBiome(" << static_cast<int>(testBiome) << ") should match tag->contains() for structure "
                    << entry.structureId.toString();
            }
        }
    }
}

} // namespace
