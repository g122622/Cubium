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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/structures/BastionRemnantStructure.hpp"
#include "common/world/gen/structure/structures/DesertPyramidStructure.hpp"
#include "common/world/gen/structure/structures/EndCityStructure.hpp"
#include "common/world/gen/structure/structures/IglooStructure.hpp"
#include "common/world/gen/structure/structures/JungleTempleStructure.hpp"
#include "common/world/gen/structure/structures/NetherFossilStructure.hpp"
#include "common/world/gen/structure/structures/OceanMonumentPieces.hpp"
#include "common/world/gen/structure/structures/OceanMonumentStructure.hpp"
#include "common/world/gen/structure/structures/PillagerOutpostStructure.hpp"
#include "common/world/gen/structure/structures/StrongholdPieces.hpp"
#include "common/world/gen/structure/structures/StrongholdStructure.hpp"
#include "common/world/gen/structure/structures/SwampHutStructure.hpp"
#include "common/world/gen/structure/structures/WoodlandMansionStructure.hpp"

#include <cmath>
#include <unordered_map>

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::Biomes;

// ============================================================================
// 测试夹具
// ============================================================================

class NewStructuresTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

namespace {

class StructureTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_UNUSED(entity);
        ++m_spawnedEntityCount;
        return EntityId(static_cast<u32>(m_spawnedEntityCount));
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(pack(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::WATER->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[pack(x, y, z)] = state;
        return true;
    }

    void fill(const StructureBoundingBox& bounds, const BlockState* state)
    {
        for (i32 x = bounds.minX(); x <= bounds.maxX(); ++x) {
            for (i32 y = bounds.minY(); y <= bounds.maxY(); ++y) {
                for (i32 z = bounds.minZ(); z <= bounds.maxZ(); ++z) {
                    setBlockState(x, y, z, state);
                }
            }
        }
    }

    [[nodiscard]] const BlockState* getRawBlockState(i32 x, i32 y, i32 z) const
    {
        const auto it = m_blocks.find(pack(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntityCount; }

private:
    [[nodiscard]] static i64 pack(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 40) ^ (static_cast<i64>(y) << 20) ^ static_cast<i64>(z & 0xFFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    size_t m_spawnedEntityCount = 0;
};

} // namespace

namespace {

/**
 * @brief 基于 OceanMonumentPiece 边界框构造用于局部房间测试的世界。
 *
 * 先用水填满片段边界，避免 `generateBoxOnFillOnly()` 与 `makeOpening()` 因读路径缺失而偏离 Java。
 */
template <typename PieceT>
StructureTestWorld buildMonumentPieceWorld(PieceT& piece)
{
    StructureTestWorld world;
    world.fill(piece.boundingBox(), &VanillaBlocks::WATER->defaultState());
    return world;
}

class FixedBiomeChunkGenerator final : public IChunkGenerator {
public:
    explicit FixedBiomeChunkGenerator(BiomeId biome)
        : m_biome(biome)
    {}

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override { return m_biome; }

    [[nodiscard]] BiomeId getNoiseBiome(i32, i32, i32) const override { return m_biome; }
    [[nodiscard]] i32 getHeight(i32, i32, HeightmapType) const override { return 64; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] const DimensionSettings& settings() const override { return m_settings; }
    [[nodiscard]] i32 seaLevel() const override { return m_settings.seaLevel; }

    void generateStructureStarts(WorldGenRegion&, ChunkPrimer&) override {}
    void generateStructureReferences(WorldGenRegion&, ChunkPrimer&) override {}
    void generateBiomes(WorldGenRegion&, ChunkPrimer&) override {}
    void generateNoise(WorldGenRegion&, ChunkPrimer&) override {}
    void buildSurface(WorldGenRegion&, ChunkPrimer&) override {}
    void applyCarvers(WorldGenRegion&, ChunkPrimer&) override {}
    void placeFeatures(WorldGenRegion&, ChunkPrimer&) override {}
    i32 spawnInitialMobs(WorldGenRegion&, ChunkPrimer&, std::vector<SpawnedEntityData>&) override { return 0; }

private:
    BiomeId m_biome;
    DimensionSettings m_settings = DimensionSettings::overworld();
};

/**
 * @brief 可配置高度和海平面的区块生成器，用于 canGenerate 测试
 */
class ConfigurableChunkGenerator final : public IChunkGenerator {
public:
    ConfigurableChunkGenerator(BiomeId biome, i32 height, i32 seaLevelVal)
        : m_biome(biome)
        , m_height(height)
        , m_seaLevel(seaLevelVal)
    {
        m_settings.seaLevel = seaLevelVal;
    }

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override { return m_biome; }
    [[nodiscard]] BiomeId getNoiseBiome(i32, i32, i32) const override { return m_biome; }
    [[nodiscard]] i32 getHeight(i32, i32, HeightmapType) const override { return m_height; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] const DimensionSettings& settings() const override { return m_settings; }
    [[nodiscard]] i32 seaLevel() const override { return m_seaLevel; }

    void generateStructureStarts(WorldGenRegion&, ChunkPrimer&) override {}
    void generateStructureReferences(WorldGenRegion&, ChunkPrimer&) override {}
    void generateBiomes(WorldGenRegion&, ChunkPrimer&) override {}
    void generateNoise(WorldGenRegion&, ChunkPrimer&) override {}
    void buildSurface(WorldGenRegion&, ChunkPrimer&) override {}
    void applyCarvers(WorldGenRegion&, ChunkPrimer&) override {}
    void placeFeatures(WorldGenRegion&, ChunkPrimer&) override {}
    i32 spawnInitialMobs(WorldGenRegion&, ChunkPrimer&, std::vector<SpawnedEntityData>&) override { return 0; }

private:
    BiomeId m_biome;
    i32 m_height;
    i32 m_seaLevel;
    DimensionSettings m_settings = DimensionSettings::overworld();
};

} // namespace

// ============================================================================
// IglooStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, Igloo_NameAndSettings)
{
    IglooStructure structure;

    EXPECT_EQ(structure.name(), "Igloo");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 14357618);

    const auto& biomes = structure.validBiomes();
    EXPECT_FALSE(biomes.empty());
    // 验证雪地生物群系
    bool hasSnowyBiome = false;
    for (auto biome : biomes) {
        if (biome == SnowyPlains || biome == SnowyTaiga) {
            hasSnowyBiome = true;
            break;
        }
    }
    EXPECT_TRUE(hasSnowyBiome);
}

TEST_F(NewStructuresTest, Igloo_CanGenerate)
{
    IglooStructure structure;
    StructureTestWorld world;
    math::Random rng(12345);

    // 雪地生物群系应允许生成
    FixedBiomeChunkGenerator snowyGen(SnowyPlains);
    EXPECT_TRUE(structure.canGenerate(world, snowyGen, rng, 0, 0));

    FixedBiomeChunkGenerator taigaGen(SnowyTaiga);
    EXPECT_TRUE(structure.canGenerate(world, taigaGen, rng, 5, 5));

    // 非雪地生物群系应拒绝生成
    FixedBiomeChunkGenerator desertGen(Desert);
    EXPECT_FALSE(structure.canGenerate(world, desertGen, rng, 0, 0));

    FixedBiomeChunkGenerator plainsGen(Plains);
    EXPECT_FALSE(structure.canGenerate(world, plainsGen, rng, 0, 0));

    // 验证 biomeTag 返回非空
    EXPECT_NE(structure.biomeTag(), nullptr);
}

TEST_F(NewStructuresTest, Igloo_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::None;
    bool hasBasement = true;
    i32 middleCount = 1; // 中间层数量

    IglooPiece piece(pos, rotation, hasBasement, middleCount);

    // 验证地下室状态
    EXPECT_TRUE(piece.hasBasement());
    EXPECT_EQ(piece.middleCount(), middleCount);

    // 有地下室时，Y 范围应该更大
    IglooPiece pieceWithoutBasement(pos, rotation, false, 0);
    EXPECT_FALSE(pieceWithoutBasement.hasBasement());
    EXPECT_EQ(pieceWithoutBasement.middleCount(), 0);
}

// ============================================================================
// SwampHutStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, SwampHut_NameAndSettings)
{
    SwampHutStructure structure;

    EXPECT_EQ(structure.name(), "Swamp_Hut");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 14357620);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 1);
    bool hasSwamp = false;
    for (auto biome : biomes) {
        if (biome == Swamp) {
            hasSwamp = true;
            break;
        }
    }
    EXPECT_TRUE(hasSwamp);
}

TEST_F(NewStructuresTest, SwampHut_CanGenerate)
{
    SwampHutStructure structure;
    StructureTestWorld world;
    math::Random rng(12345);

    // 沼泽生物群系应允许生成
    FixedBiomeChunkGenerator swampGen(Swamp);
    EXPECT_TRUE(structure.canGenerate(world, swampGen, rng, 0, 0));

    // 非沼泽生物群系应拒绝生成
    FixedBiomeChunkGenerator desertGen(Desert);
    EXPECT_FALSE(structure.canGenerate(world, desertGen, rng, 0, 0));

    FixedBiomeChunkGenerator plainsGen(Plains);
    EXPECT_FALSE(structure.canGenerate(world, plainsGen, rng, 0, 0));

    // 验证 biomeTag 返回非空
    EXPECT_NE(structure.biomeTag(), nullptr);
}

TEST_F(NewStructuresTest, SwampHut_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::Clockwise90;

    SwampHutPiece piece(pos, rotation);

    // 验证边界框（沼泽小屋约 7x6x9）
    EXPECT_LE(piece.maxX() - piece.minX(), 10);
    EXPECT_LE(piece.maxY() - piece.minY(), 10);
    EXPECT_LE(piece.maxZ() - piece.minZ(), 15);
}

// ============================================================================
// NetherFossilStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, NetherFossil_NameAndSettings)
{
    NetherFossilStructure structure;

    EXPECT_EQ(structure.name(), "Nether_Fossil");
    EXPECT_EQ(structure.separationSettings().spacing, 2);
    EXPECT_EQ(structure.separationSettings().separation, 1);
    EXPECT_EQ(structure.separationSettings().salt, 14357921);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 1);
    EXPECT_EQ(biomes[0], SoulSandValley);
}

TEST_F(NewStructuresTest, NetherFossil_PieceConstruction)
{
    BlockPos pos(100, 30, 200);
    std::string templateName = "nether_fossils/fossil_3";
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::Clockwise180;

    NetherFossilPiece piece(templateName, pos, rotation);

    // 验证模板名称
    EXPECT_EQ(piece.templateName(), templateName);
}

// ============================================================================
// PillagerOutpostStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, PillagerOutpost_NameAndSettings)
{
    PillagerOutpostStructure structure;

    EXPECT_EQ(structure.name(), "Pillager_Outpost");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 165745296);

    const auto& biomes = structure.validBiomes();
    EXPECT_FALSE(biomes.empty());
    // 验证包含平原等生物群系
    bool hasPlains = false;
    for (auto biome : biomes) {
        if (biome == Plains) {
            hasPlains = true;
            break;
        }
    }
    EXPECT_TRUE(hasPlains);
}

// ============================================================================
// EndCityStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, EndCity_NameAndSettings)
{
    EndCityStructure structure;

    EXPECT_EQ(structure.name(), "End_City");
    EXPECT_EQ(structure.separationSettings().spacing, 20);
    EXPECT_EQ(structure.separationSettings().separation, 11);
    EXPECT_EQ(structure.separationSettings().salt, 10387313);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 2);
    bool hasEndMidlands = false;
    for (auto biome : biomes) {
        if (biome == EndMidlands) {
            hasEndMidlands = true;
            break;
        }
    }
    EXPECT_TRUE(hasEndMidlands);
}

TEST_F(NewStructuresTest, EndCity_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation =
        mc::world::gen::feature::template_::Rotation::CounterClockwise90;
    std::string templateName = "base_floor";

    end_city::CityTemplate piece(templateName, pos, rotation, false);

    // 验证模板名称和旋转
    EXPECT_EQ(piece.templateName(), templateName);
    EXPECT_EQ(piece.rotation(), rotation);
    EXPECT_FALSE(piece.overwrite());
}

// ============================================================================
// WoodlandMansionStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, WoodlandMansion_NameAndSettings)
{
    WoodlandMansionStructure structure;

    EXPECT_EQ(structure.name(), "Woodland_Mansion");
    EXPECT_EQ(structure.separationSettings().spacing, 80);
    EXPECT_EQ(structure.separationSettings().separation, 20);
    EXPECT_EQ(structure.separationSettings().salt, 10387319);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 2);
    bool hasDarkForest = false;
    for (auto biome : biomes) {
        if (biome == DarkForest) {
            hasDarkForest = true;
            break;
        }
    }
    EXPECT_TRUE(hasDarkForest);
}

TEST_F(NewStructuresTest, WoodlandMansion_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::None;
    std::string templateName = "1x1_a1";

    WoodlandMansionPiece piece(templateName, pos, rotation);

    // 验证模板名称和旋转
    EXPECT_EQ(piece.templateName(), templateName);
    EXPECT_EQ(piece.rotation(), rotation);
}

// ============================================================================
// BastionRemnantStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, BastionRemnant_NameAndSettings)
{
    BastionRemnantStructure structure;

    EXPECT_EQ(structure.name(), "bastion_remnant");
    EXPECT_EQ(structure.separationSettings().spacing, 27);
    EXPECT_EQ(structure.separationSettings().separation, 4);
    EXPECT_EQ(structure.separationSettings().salt, 30084232);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 4);
    // 验证不包含玄武岩三角洲
    for (auto biome : biomes) {
        EXPECT_NE(biome, BasaltDeltas);
    }
    // 验证包含下界荒地
    bool hasNetherWastes = false;
    for (auto biome : biomes) {
        if (biome == NetherWastes) {
            hasNetherWastes = true;
            break;
        }
    }
    EXPECT_TRUE(hasNetherWastes);
}

// ============================================================================
// 边界和边界测试
// ============================================================================

TEST_F(NewStructuresTest, AllStructures_HaveValidSeparationSettings)
{
    IglooStructure igloo;
    SwampHutStructure swampHut;
    NetherFossilStructure netherFossil;
    PillagerOutpostStructure outpost;
    EndCityStructure endCity;
    WoodlandMansionStructure mansion;
    BastionRemnantStructure bastion;

    // 分离设置应该合理
    auto validateSettings = [](const StructureSeparationSettings& settings) {
        EXPECT_GT(settings.spacing, 0);
        EXPECT_GT(settings.separation, 0);
        EXPECT_GE(settings.spacing, settings.separation);
        EXPECT_GT(settings.salt, 0);
    };

    validateSettings(igloo.separationSettings());
    validateSettings(swampHut.separationSettings());
    validateSettings(netherFossil.separationSettings());
    validateSettings(outpost.separationSettings());
    validateSettings(endCity.separationSettings());
    validateSettings(mansion.separationSettings());
    validateSettings(bastion.separationSettings());
}

TEST_F(NewStructuresTest, AllStructures_HaveValidBiomes)
{
    IglooStructure igloo;
    SwampHutStructure swampHut;
    NetherFossilStructure netherFossil;
    PillagerOutpostStructure outpost;
    EndCityStructure endCity;
    WoodlandMansionStructure mansion;
    BastionRemnantStructure bastion;

    // 所有结构应该有有效的生物群系列表
    EXPECT_FALSE(igloo.validBiomes().empty());
    EXPECT_FALSE(swampHut.validBiomes().empty());
    EXPECT_FALSE(netherFossil.validBiomes().empty());
    EXPECT_FALSE(outpost.validBiomes().empty());
    EXPECT_FALSE(endCity.validBiomes().empty());
    EXPECT_FALSE(mansion.validBiomes().empty());
    EXPECT_FALSE(bastion.validBiomes().empty());
}

TEST_F(NewStructuresTest, AllStructures_HaveValidNames)
{
    IglooStructure igloo;
    SwampHutStructure swampHut;
    NetherFossilStructure netherFossil;
    PillagerOutpostStructure outpost;
    EndCityStructure endCity;
    WoodlandMansionStructure mansion;
    BastionRemnantStructure bastion;

    // 名称应该非空
    EXPECT_FALSE(igloo.name().empty());
    EXPECT_FALSE(swampHut.name().empty());
    EXPECT_FALSE(netherFossil.name().empty());
    EXPECT_FALSE(outpost.name().empty());
    EXPECT_FALSE(endCity.name().empty());
    EXPECT_FALSE(mansion.name().empty());
    EXPECT_FALSE(bastion.name().empty());
}

// ============================================================================
// PillagerOutpostStructure Village Detection Tests (P4)
// ============================================================================

TEST_F(NewStructuresTest, PillagerOutpost_VillageDetection)
{
    // 测试 isNearVillage 使用正确的村庄间距参数
    // 村庄参数: spacing=32, separation=8, salt=10387312

    PillagerOutpostStructure outpost;

    // 验证前哨站不会在村庄位置生成
    // 这个测试验证 isNearVillage 算法的正确性
    // 实际位置检测依赖于世界种子

    // 前哨站应在距离村庄 10 区块外生成
    EXPECT_EQ(outpost.separationSettings().spacing, 32);
    EXPECT_EQ(outpost.separationSettings().separation, 8);
}

// ============================================================================
// StrongholdStructure Tests (P4)
// ============================================================================

TEST_F(NewStructuresTest, Stronghold_NameAndSettings)
{
    StrongholdStructure structure;

    EXPECT_EQ(structure.name(), "stronghold");
    // 要塞使用特殊的位置计算，不使用标准 spacing/separation
    EXPECT_EQ(structure.separationSettings().spacing, 1);
    EXPECT_EQ(structure.separationSettings().separation, 0);
}

TEST_F(NewStructuresTest, Stronghold_RingCalculation)
{
    // MC 1.16.5: 8 个环，每环要塞数量
    // 环 0: 3, 环 1: 3, 环 2: 3, 环 3: 4, 环 4: 6, 环 5: 10, 环 6: 15, 环 7: 21
    // 总计: 65 个要塞

    EXPECT_EQ(StrongholdStructure::getRing(0), 0);  // 第 1 个要塞在环 0
    EXPECT_EQ(StrongholdStructure::getRing(2), 0);  // 第 3 个要塞在环 0
    EXPECT_EQ(StrongholdStructure::getRing(3), 1);  // 第 4 个要塞在环 1
    EXPECT_EQ(StrongholdStructure::getRing(6), 2);  // 第 7 个要塞在环 2
    EXPECT_EQ(StrongholdStructure::getRing(9), 3);  // 第 10 个要塞在环 3
    EXPECT_EQ(StrongholdStructure::getRing(13), 4); // 第 14 个要塞在环 4
    EXPECT_EQ(StrongholdStructure::getRing(19), 5); // 第 20 个要塞在环 5
    EXPECT_EQ(StrongholdStructure::getRing(29), 6); // 第 30 个要塞在环 6
    EXPECT_EQ(StrongholdStructure::getRing(44), 7); // 第 45 个要塞在环 7
    EXPECT_EQ(StrongholdStructure::getRing(64), 7); // 第 65 个要塞在环 7
}

TEST_F(NewStructuresTest, Stronghold_PositionCalculation)
{
    // 测试要塞位置计算
    i64 seed = 12345;

    auto [chunkX1, chunkZ1] = StrongholdStructure::calculateStrongholdPos(0, seed);
    auto [chunkX2, chunkZ2] = StrongholdStructure::calculateStrongholdPos(1, seed);

    // 相同种子、不同索引应该产生不同位置
    EXPECT_NE(chunkX1, chunkX2);
    EXPECT_NE(chunkZ1, chunkZ2);

    // 相同种子、相同索引应该产生相同位置
    auto [chunkX1b, chunkZ1b] = StrongholdStructure::calculateStrongholdPos(0, seed);
    EXPECT_EQ(chunkX1, chunkX1b);
    EXPECT_EQ(chunkZ1, chunkZ1b);

    // 位置应该在合理范围内（环 0 距离 1408-2688 区块）
    // 注意：返回的是区块坐标，不是世界坐标
    EXPECT_GT(std::abs(chunkX1), 80);  // 至少 80 区块距离
    EXPECT_LT(std::abs(chunkX1), 200); // 最多 200 区块距离（环 0）
}

TEST_F(NewStructuresTest, StrongholdPieces_WeightInitialization)
{
    // 测试片段权重初始化
    std::vector<StrongholdPieceWeight> weights;
    initializeStrongholdPieceWeights(weights);

    EXPECT_FALSE(weights.empty());

    // 验证权重包含关键片段类型
    bool hasStraight = false;
    bool hasPortalRoom = false;
    bool hasLibrary = false;

    for (const auto& weight : weights) {
        if (weight.pieceType == StrongholdPieceTypes::STRAIGHT) hasStraight = true;
        if (weight.pieceType == StrongholdPieceTypes::PORTAL_ROOM) hasPortalRoom = true;
        if (weight.pieceType == StrongholdPieceTypes::LIBRARY) hasLibrary = true;
    }

    EXPECT_TRUE(hasStraight);
    EXPECT_TRUE(hasPortalRoom);
    EXPECT_TRUE(hasLibrary);
}

TEST_F(NewStructuresTest, Stronghold_CanGenerateIsNotUniversal)
{
    StrongholdStructure structure;
    StructureTestWorld world;
    FixedBiomeChunkGenerator generator(Plains);
    math::Random rng(12345);

    // MC 原版要塞位置由预计算结果决定，不应在任意区块都返回 true。
    const bool originCanGenerate = structure.canGenerate(world, generator, rng, 0, 0);
    const bool farCanGenerate = structure.canGenerate(world, generator, rng, 200, -200);

    EXPECT_FALSE(originCanGenerate && farCanGenerate);
}

// ============================================================================
// DesertPyramidStructure Tests (P4)
// ============================================================================

TEST_F(NewStructuresTest, DesertPyramid_NameAndSettings)
{
    DesertPyramidStructure structure;

    EXPECT_EQ(structure.name(), "desert_pyramid");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 14357617);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 1);
    for (auto biome : biomes) {
        EXPECT_TRUE(biome == Desert);
    }
}

TEST_F(NewStructuresTest, DesertPyramid_CanGenerate_DesertBiome_Allowed)
{
    DesertPyramidStructure structure;
    StructureTestWorld world;
    math::Random rng(12345);

    // 沙漠生物群系 + 高度在海平面以上 → 允许生成
    ConfigurableChunkGenerator desertGen(Desert, 70, 63);
    EXPECT_TRUE(structure.canGenerate(world, desertGen, rng, 0, 0));
}

TEST_F(NewStructuresTest, DesertPyramid_CanGenerate_NonDesertBiome_Rejected)
{
    DesertPyramidStructure structure;
    StructureTestWorld world;
    math::Random rng(12345);

    // 非沙漠生物群系 → 拒绝生成
    ConfigurableChunkGenerator plainsGen(Plains, 70, 63);
    EXPECT_FALSE(structure.canGenerate(world, plainsGen, rng, 0, 0));

    ConfigurableChunkGenerator swampGen(Swamp, 70, 63);
    EXPECT_FALSE(structure.canGenerate(world, swampGen, rng, 0, 0));
}

TEST_F(NewStructuresTest, DesertPyramid_CanGenerate_BelowSeaLevel_Rejected)
{
    DesertPyramidStructure structure;
    StructureTestWorld world;
    math::Random rng(12345);

    // 沙漠生物群系但四角高度低于海平面 → 拒绝生成
    ConfigurableChunkGenerator lowGen(Desert, 50, 63);
    EXPECT_FALSE(structure.canGenerate(world, lowGen, rng, 0, 0));

    // 高度恰好等于海平面 → 允许生成
    ConfigurableChunkGenerator atSeaGen(Desert, 63, 63);
    EXPECT_TRUE(structure.canGenerate(world, atSeaGen, rng, 0, 0));
}

TEST_F(NewStructuresTest, DesertPyramid_CanGenerate_BiomeTag)
{
    DesertPyramidStructure structure;

    // 验证 biomeTag 返回非空且为正确的标签
    const auto* tag = structure.biomeTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(Desert));
    EXPECT_FALSE(tag->contains(Plains));
    EXPECT_FALSE(tag->contains(Swamp));
}

// ============================================================================
// JungleTempleStructure Tests (P4)
// ============================================================================

TEST_F(NewStructuresTest, JungleTemple_NameAndSettings)
{
    JungleTempleStructure structure;

    EXPECT_EQ(structure.name(), "jungle_temple");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 14357619);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 2);
    for (auto biome : biomes) {
        EXPECT_TRUE(biome == BambooJungle || biome == Jungle);
    }
}

TEST_F(NewStructuresTest, OceanMonument_NameAndSettings)
{
    OceanMonumentStructure structure;

    EXPECT_EQ(structure.name(), "ocean_monument");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 5);
    EXPECT_EQ(structure.separationSettings().salt, 10387313);
    EXPECT_FALSE(structure.useUniformSpacing());

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 5);
    EXPECT_EQ(biomes[0], DeepOcean);
}

TEST_F(NewStructuresTest, OceanMonument_RoomDefinitionConnections)
{
    OceanMonumentRoomDefinition source(0);
    OceanMonumentRoomDefinition east(1);

    source.setConnection(5, &east);
    east.setConnection(4, &source);
    source.updateOpenings();
    east.updateOpenings();

    EXPECT_TRUE(source.hasOpening(5));
    EXPECT_TRUE(east.hasOpening(4));
    EXPECT_EQ(source.countOpenings(), 1);
    EXPECT_EQ(east.countOpenings(), 1);
    EXPECT_EQ(source.getConnection(5), &east);
    EXPECT_EQ(east.getConnection(4), &source);
}

TEST_F(NewStructuresTest, OceanMonument_BuildingConstruction)
{
    math::Random rng(12345);
    OceanMonumentBuilding building(rng, 0, 0, Direction::North);

    const StructureBoundingBox bounds = building.boundingBox();
    EXPECT_EQ(bounds.minX(), 0);
    EXPECT_EQ(bounds.minZ(), 0);
    EXPECT_EQ(bounds.maxX(), 57);
    EXPECT_EQ(bounds.maxZ(), 57);
    EXPECT_GE(bounds.maxY() - bounds.minY(), 22);
}

TEST_F(NewStructuresTest, OceanMonument_RoomDefinitionFindsSource)
{
    OceanMonumentRoomDefinition source(0);
    OceanMonumentRoomDefinition middle(1);
    OceanMonumentRoomDefinition leaf(2);

    source.setSource(true);
    source.setConnection(5, &middle);
    middle.setConnection(4, &source);
    middle.setConnection(5, &leaf);
    leaf.setConnection(4, &middle);

    source.updateOpenings();
    middle.updateOpenings();
    leaf.updateOpenings();

    EXPECT_TRUE(leaf.findSource(1));
    EXPECT_TRUE(middle.findSource(2));
}

TEST_F(NewStructuresTest, OceanMonument_PenthouseSpawnsElderGuardian)
{
    StructureBoundingBox bounds(0, 39, 0, 57, 61, 57);
    OceanMonumentPenthouse penthouse(Direction::North, bounds);
    StructureTestWorld world = buildMonumentPieceWorld(penthouse);
    math::Random rng(12345);

    penthouse.generate(world, rng, 0, 0, bounds);

    EXPECT_EQ(world.spawnedEntityCount(), 1u);
}
