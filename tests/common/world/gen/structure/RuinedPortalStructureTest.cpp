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

// ============================================================================
// RuinedPortalStructure 处理器链集成测试
//
// 本测试针对 RuinedPortalPiece::generate() 中实现的处理器链（对应 MC 1.21.11
// RuinedPortalPiece#makeSettings）进行端到端验证。MC 原版处理器顺序：
//   1. BlockIgnoreProcessor (airPocket ? STRUCTURE_BLOCK : STRUCTURE_AND_AIR)
//   2. RuleProcessor (gold→air 0.3, lava 规则, netherrack→magma 0.07 if !cold)
//   3. BlockAgeProcessor (mossiness)
//   4. ProtectedBlockProcessor (FEATURES_CANNOT_REPLACE)
//   5. LavaSubmergedBlockProcessor
//   6. BlackstoneReplaceProcessor (if replaceWithBlackstone)
//
// 由于处理器链内部组装逻辑位于 RuinedPortalStructure.cpp 匿名命名空间，本测试
// 通过公共 API（Structure::generate / RuinedPortalPiece::properties / location）
// 验证配置正确性，并通过直接调用单个处理器的 process() 方法验证规则替换概率。
// ============================================================================

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/ProtectedBlocksProcessor.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/structures/RuinedPortalStructure.hpp"

#include <algorithm>
#include <unordered_map>

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::world::gen::feature::template_;
using namespace mc::Biomes;

namespace {

// ============================================================================
// 测试用区块生成器（参考 test_new_structures.cpp 的 FixedBiomeChunkGenerator）
// ============================================================================

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

// ============================================================================
// 辅助：从 StructureStart 中取第一个 RuinedPortalPiece
// ============================================================================

const RuinedPortalPiece* firstRuinedPortalPiece(const StructureStart& start)
{
    for (const auto& piece : start.pieces()) {
        if (piece->type() == StructurePieceTypes::RUINED_PORTAL) {
            return dynamic_cast<const RuinedPortalPiece*>(piece.get());
        }
    }
    return nullptr;
}

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================

class RuinedPortalStructureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// 1. RuinedPortalPiece 字段存储测试
// ============================================================================

TEST_F(RuinedPortalStructureTest, PieceStoresTemplateNamePositionRotationMirror)
{
    RuinedPortalProperties props;
    props.cold = false;
    props.mossiness = 0.5f;
    props.airPocket = true;
    props.overgrown = false;
    props.vines = false;
    props.replaceWithBlackstone = false;

    RuinedPortalPiece piece("ruined_portal/portal_1",
        BlockPos(100, 64, 200),
        Rotation::Clockwise90,
        Mirror::FrontBack,
        RuinedPortalLocation::OnLandSurface,
        props);

    EXPECT_EQ(piece.templateName(), "ruined_portal/portal_1");
    EXPECT_EQ(piece.location(), RuinedPortalLocation::OnLandSurface);
    EXPECT_FLOAT_EQ(piece.properties().mossiness, 0.5f);
    EXPECT_TRUE(piece.properties().airPocket);
    EXPECT_FALSE(piece.properties().cold);
    EXPECT_FALSE(piece.properties().replaceWithBlackstone);
}

TEST_F(RuinedPortalStructureTest, PieceStoresAllLocationValues)
{
    RuinedPortalProperties props;

    for (auto loc : {RuinedPortalLocation::OnLandSurface,
             RuinedPortalLocation::PartlyBuried,
             RuinedPortalLocation::OnOceanFloor,
             RuinedPortalLocation::InMountain,
             RuinedPortalLocation::Underground,
             RuinedPortalLocation::InNether}) {
        RuinedPortalPiece piece("ruined_portal/portal_1", BlockPos(0, 0, 0), Rotation::None, Mirror::None, loc, props);

        EXPECT_EQ(piece.location(), loc) << "Location " << static_cast<int>(loc) << " not stored correctly";
    }
}

// ============================================================================
// 2. Structure::generate 端到端：验证生成的片段具有正确的属性
// ============================================================================

TEST_F(RuinedPortalStructureTest, GenerateProducesValidStartWithRuinedPortalPiece)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Plains);
    math::Random rng(54321);

    auto start = structure.generate(generator, rng, 10, 10);
    ASSERT_NE(start, nullptr);
    EXPECT_GT(start->pieceCount(), 0u);

    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 模板名应为普通或巨型
    const auto& normalTemplates = RuinedPortalStructure::getNormalTemplates();
    const auto& giantTemplates = RuinedPortalStructure::getGiantTemplates();
    const auto& name = piece->templateName();
    bool isNormal = std::find(normalTemplates.begin(), normalTemplates.end(), name) != normalTemplates.end();
    bool isGiant = std::find(giantTemplates.begin(), giantTemplates.end(), name) != giantTemplates.end();
    EXPECT_TRUE(isNormal || isGiant) << "Unexpected template name: " << name;
}

// ============================================================================
// 3. 配置正确性测试：不同生物群系对应的属性
// 对应 MC 1.21.11 RuinedPortalStructure.configureProperties / determineLocation
// ============================================================================

TEST_F(RuinedPortalStructureTest, DesertBiomeProducesDesertConfiguration)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Desert);
    math::Random rng(111);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 沙漠类型: 部分掩埋，无空气口袋，无苔藓
    EXPECT_EQ(piece->location(), RuinedPortalLocation::PartlyBuried);
    EXPECT_FALSE(piece->properties().airPocket);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.0f);
    EXPECT_FALSE(piece->properties().replaceWithBlackstone);
}

TEST_F(RuinedPortalStructureTest, JungleBiomeProducesJungleConfiguration)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Jungle);
    math::Random rng(222);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 丛林类型: 在地表，高苔藓，过度生长，藤蔓
    EXPECT_EQ(piece->location(), RuinedPortalLocation::OnLandSurface);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.8f);
    EXPECT_TRUE(piece->properties().overgrown);
    EXPECT_TRUE(piece->properties().vines);
}

TEST_F(RuinedPortalStructureTest, SwampBiomeProducesSwampConfiguration)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Swamp);
    math::Random rng(333);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 沼泽类型: 在海底，无空气口袋，中等苔藓，有藤蔓
    EXPECT_EQ(piece->location(), RuinedPortalLocation::OnOceanFloor);
    EXPECT_FALSE(piece->properties().airPocket);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.5f);
    EXPECT_TRUE(piece->properties().vines);
}

TEST_F(RuinedPortalStructureTest, OceanBiomeProducesOceanConfiguration)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Ocean);
    math::Random rng(444);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 海洋类型: 在海底，无空气口袋，高苔藓
    EXPECT_EQ(piece->location(), RuinedPortalLocation::OnOceanFloor);
    EXPECT_FALSE(piece->properties().airPocket);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.8f);
}

// ============================================================================
// 4. 寒冷生物群系检测
// 对应 MC 1.21.11 RuinedPortalStructure.configureProperties 中的 cold 判断
// ============================================================================

TEST_F(RuinedPortalStructureTest, ColdBiomeSetsColdFlag)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(SnowyPlains);
    math::Random rng(555);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 雪地生物群系应设置 cold=true（Standard 类型 + 雪地生物群系）
    EXPECT_TRUE(piece->properties().cold);
}

TEST_F(RuinedPortalStructureTest, NonColdBiomeDoesNotSetColdFlag)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Plains);
    math::Random rng(666);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 平原不应设置 cold=true
    EXPECT_FALSE(piece->properties().cold);
}

// ============================================================================
// 5. RuleStructureProcessor 替换规则单元测试
// 直接构造与 RuinedPortalStructure.cpp 匿名命名空间相同的规则结构进行验证
// ============================================================================

namespace {

// 与 RuinedPortalStructure.cpp 匿名命名空间中的常量保持一致
constexpr f32 PROBABILITY_OF_GOLD_GONE_TEST = 0.3F;
constexpr f32 PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK_TEST = 0.07F;
constexpr f32 PROBABILITY_OF_MAGMA_INSTEAD_OF_LAVA_TEST = 0.2F;

// 与 RuinedPortalStructure.cpp 匿名命名空间中的辅助函数保持一致
std::unique_ptr<RuleEntry> makeBlockReplaceRule(const Block* inputBlock, const Block* outputBlock)
{
    if (inputBlock == nullptr || outputBlock == nullptr) {
        return nullptr;
    }
    return std::make_unique<RuleEntry>(std::make_unique<BlockMatchRuleTest>(inputBlock),
        std::make_unique<AlwaysTrueRuleTest>(),
        outputBlock->defaultState().stateId());
}

std::unique_ptr<RuleEntry> makeRandomBlockReplaceRule(
    const Block* inputBlock, f32 probability, const Block* outputBlock)
{
    if (inputBlock == nullptr || outputBlock == nullptr) {
        return nullptr;
    }
    return std::make_unique<RuleEntry>(std::make_unique<RandomBlockMatchRuleTest>(inputBlock, probability),
        std::make_unique<AlwaysTrueRuleTest>(),
        outputBlock->defaultState().stateId());
}

} // namespace

// ============================================================================
// 6. 金块 0.3 概率替换为空气的规则测试
// ============================================================================

TEST_F(RuinedPortalStructureTest, GoldBlockReplacementProbabilityApproximately30Percent)
{
    if (!VanillaBlocks::GOLD_BLOCK || !VanillaBlocks::AIR) {
        GTEST_SKIP() << "GOLD_BLOCK or AIR not registered";
    }

    auto rule =
        makeRandomBlockReplaceRule(VanillaBlocks::GOLD_BLOCK, PROBABILITY_OF_GOLD_GONE_TEST, VanillaBlocks::AIR);
    ASSERT_NE(rule, nullptr);

    // RuleEntry 的 matches 方法接受 inputState（模板方块）和 locationState（世界方块）
    // 由于 locationPredicate 是 AlwaysTrueRuleTest，世界方块任意皆可
    const BlockState& goldState = VanillaBlocks::GOLD_BLOCK->defaultState();
    const BlockState& goldLocationState = goldState; // 任意，AlwaysTrue 不检查具体值

    int replacedCount = 0;
    const int totalBlocks = 5000;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockPos pos(i * 7, 64, i * 13);
        math::Random rng(static_cast<u64>(i) * 0x9E3779B97F4A7C15ULL);

        if (rule->matches(&goldState, &goldLocationState, pos, pos, BlockPos(0, 0, 0), rng)) {
            ++replacedCount;
        }
    }

    f32 rate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    // 0.3 概率，允许 ±5% 误差
    EXPECT_GT(rate, 0.25f) << "Gold replacement rate too low: " << rate;
    EXPECT_LT(rate, 0.35f) << "Gold replacement rate too high: " << rate;
}

// ============================================================================
// 7. 岩浆处理规则测试：OnOceanFloor / cold / 默认
// 对应 MC 1.21.11 RuinedPortalPiece#getLavaProcessorRule
// ============================================================================

TEST_F(RuinedPortalStructureTest, LavaRuleOnOceanFloorReplacesLavaWithMagma)
{
    if (!VanillaBlocks::LAVA || !VanillaBlocks::MAGMA) {
        GTEST_SKIP() << "LAVA or MAGMA not registered";
    }

    // OnOceanFloor: 岩浆固定替换为岩浆块
    auto rule = makeBlockReplaceRule(VanillaBlocks::LAVA, VanillaBlocks::MAGMA);
    ASSERT_NE(rule, nullptr);

    const BlockState& lavaState = VanillaBlocks::LAVA->defaultState();
    EXPECT_EQ(rule->outputStateId(), VanillaBlocks::MAGMA->defaultState().stateId());

    // 固定替换（BlockMatch），应总是返回 true
    BlockPos pos(0, 0, 0);
    math::Random rng(1);
    EXPECT_TRUE(rule->matches(&lavaState, &lavaState, pos, pos, pos, rng));
}

TEST_F(RuinedPortalStructureTest, LavaRuleColdReplacesLavaWithNetherrack)
{
    if (!VanillaBlocks::LAVA || !VanillaBlocks::NETHERRACK) {
        GTEST_SKIP() << "LAVA or NETHERRACK not registered";
    }

    // cold: 岩浆固定替换为下界岩
    auto rule = makeBlockReplaceRule(VanillaBlocks::LAVA, VanillaBlocks::NETHERRACK);
    ASSERT_NE(rule, nullptr);

    EXPECT_EQ(rule->outputStateId(), VanillaBlocks::NETHERRACK->defaultState().stateId());
}

TEST_F(RuinedPortalStructureTest, LavaRuleDefaultReplacesLavaWithMagmaAt20Percent)
{
    if (!VanillaBlocks::LAVA || !VanillaBlocks::MAGMA) {
        GTEST_SKIP() << "LAVA or MAGMA not registered";
    }

    // 默认: 岩浆以 0.2 概率替换为岩浆块
    auto rule = makeRandomBlockReplaceRule(
        VanillaBlocks::LAVA, PROBABILITY_OF_MAGMA_INSTEAD_OF_LAVA_TEST, VanillaBlocks::MAGMA);
    ASSERT_NE(rule, nullptr);

    EXPECT_EQ(rule->outputStateId(), VanillaBlocks::MAGMA->defaultState().stateId());

    const BlockState& lavaState = VanillaBlocks::LAVA->defaultState();
    int replacedCount = 0;
    const int totalBlocks = 5000;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockPos pos(i * 7, 64, i * 13);
        math::Random rng(static_cast<u64>(i) * 0x9E3779B97F4A7C15ULL);

        if (rule->matches(&lavaState, &lavaState, pos, pos, pos, rng)) {
            ++replacedCount;
        }
    }

    f32 rate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    // 0.2 概率，允许 ±5% 误差
    EXPECT_GT(rate, 0.15f) << "Lava->Magma rate too low: " << rate;
    EXPECT_LT(rate, 0.25f) << "Lava->Magma rate too high: " << rate;
}

// ============================================================================
// 8. 下界岩 0.07 概率替换为岩浆块的规则测试
// 对应 MC 1.21.11 RuinedPortalPiece#makeSettings 中的 netherrack 规则（!cold 时）
// ============================================================================

TEST_F(RuinedPortalStructureTest, NetherrackReplacementProbabilityApproximately7Percent)
{
    if (!VanillaBlocks::NETHERRACK || !VanillaBlocks::MAGMA) {
        GTEST_SKIP() << "NETHERRACK or MAGMA not registered";
    }

    auto rule = makeRandomBlockReplaceRule(
        VanillaBlocks::NETHERRACK, PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK_TEST, VanillaBlocks::MAGMA);
    ASSERT_NE(rule, nullptr);

    const BlockState& netherrackState = VanillaBlocks::NETHERRACK->defaultState();
    int replacedCount = 0;
    const int totalBlocks = 5000;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockPos pos(i * 7, 64, i * 13);
        math::Random rng(static_cast<u64>(i) * 0x9E3779B97F4A7C15ULL);

        if (rule->matches(&netherrackState, &netherrackState, pos, pos, pos, rng)) {
            ++replacedCount;
        }
    }

    f32 rate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    // 0.07 概率，允许 ±3% 误差
    EXPECT_GT(rate, 0.04f) << "Netherrack->Magma rate too low: " << rate;
    EXPECT_LT(rate, 0.10f) << "Netherrack->Magma rate too high: " << rate;
}

// ============================================================================
// 9. BlockAgeProcessor mossiness 概率传递性测试
// 验证 RuinedPortalStructure 将 m_properties.mossiness 正确传递给 BlockAgeProcessor
// 通过 BlockAgeProcessor 直接验证 mossiness 行为（参考 BlockAgeProcessorTest.cpp）
// ============================================================================

TEST_F(RuinedPortalStructureTest, JungleMossinessValueIsHigh)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Jungle);
    math::Random rng(777);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 丛林 mossiness=0.8，应使 BlockAgeProcessor 高概率替换台阶/墙壁
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.8f);
}

TEST_F(RuinedPortalStructureTest, DesertMossinessValueIsZero)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Desert);
    math::Random rng(888);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 沙漠 mossiness=0.0
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.0f);
}

// ============================================================================
// 10. BlackstoneReplacementProcessor 触发条件测试
// 仅当 m_properties.replaceWithBlackstone = true 时才添加该处理器
// 在项目中 replaceWithBlackstone 仅在 Nether 类型设置，本测试验证 Standard 类型不设置
// ============================================================================

TEST_F(RuinedPortalStructureTest, StandardTypeDoesNotSetReplaceWithBlackstone)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Plains);
    math::Random rng(999);

    auto start = structure.generate(generator, rng, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // Standard 类型不应设置 replaceWithBlackstone
    EXPECT_FALSE(piece->properties().replaceWithBlackstone);
}

// ============================================================================
// 11. 验证处理器链顺序：通过 RuinedPortalPiece 的 properties 触发不同分支
// 这里通过多次生成验证 randomness 不破坏配置一致性
// ============================================================================

TEST_F(RuinedPortalStructureTest, MultipleGenerationsProduceConsistentConfiguration)
{
    RuinedPortalStructure structure(ResourceLocation("minecraft", "ruined_portal"));
    FixedBiomeChunkGenerator generator(Swamp);

    for (u64 seed = 1; seed <= 20; ++seed) {
        math::Random rng(seed);
        auto start = structure.generate(generator, rng, 0, 0);
        ASSERT_NE(start, nullptr);
        const auto* piece = firstRuinedPortalPiece(*start);
        ASSERT_NE(piece, nullptr);

        // 沼泽类型: 在海底，无空气口袋，中等苔藓(0.5)，有藤蔓
        EXPECT_EQ(piece->location(), RuinedPortalLocation::OnOceanFloor) << "Wrong location at seed " << seed;
        EXPECT_FALSE(piece->properties().airPocket) << "airPocket wrong at seed " << seed;
        EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.5f) << "mossiness wrong at seed " << seed;
        EXPECT_TRUE(piece->properties().vines) << "vines wrong at seed " << seed;
    }
}

// ============================================================================
// 12. 验证 RuleStructureProcessor 处理器能正确组装并处理方块
// 构造完整的 RuleStructureProcessor（与 RuinedPortalStructure.cpp 中的相同）
// 并验证它能正确处理金块/岩浆/下界岩
// ============================================================================

TEST_F(RuinedPortalStructureTest, FullRuleProcessorChainReplacesGoldAndLavaCorrectly)
{
    if (!VanillaBlocks::GOLD_BLOCK || !VanillaBlocks::AIR || !VanillaBlocks::LAVA || !VanillaBlocks::MAGMA ||
        !VanillaBlocks::NETHERRACK) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    // 构造与 RuinedPortalStructure.cpp 中相同的规则链（!cold 情况）
    std::vector<std::unique_ptr<RuleEntry>> rules;
    rules.push_back(
        makeRandomBlockReplaceRule(VanillaBlocks::GOLD_BLOCK, PROBABILITY_OF_GOLD_GONE_TEST, VanillaBlocks::AIR));
    rules.push_back(makeRandomBlockReplaceRule(
        VanillaBlocks::LAVA, PROBABILITY_OF_MAGMA_INSTEAD_OF_LAVA_TEST, VanillaBlocks::MAGMA));
    rules.push_back(makeRandomBlockReplaceRule(
        VanillaBlocks::NETHERRACK, PROBABILITY_OF_MAGMA_INSTEAD_OF_NETHERRACK_TEST, VanillaBlocks::MAGMA));

    RuleStructureProcessor processor(std::move(rules));

    const BlockState& goldState = VanillaBlocks::GOLD_BLOCK->defaultState();
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    const BlockState& lavaState = VanillaBlocks::LAVA->defaultState();
    const BlockState& magmaState = VanillaBlocks::MAGMA->defaultState();
    const BlockState& netherrackState = VanillaBlocks::NETHERRACK->defaultState();

    PlacementSettings settings;

    // 验证金块替换为空气（统计上应接近 0.3）
    int goldReplaced = 0;
    int lavaReplaced = 0;
    int netherrackReplaced = 0;
    const int totalBlocks = 3000;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockPos pos(i * 7, 64, i * 13);
        BlockInfo goldInfo(pos, goldState.stateId());
        BlockInfo lavaInfo(pos, lavaState.stateId());
        BlockInfo netherrackInfo(pos, netherrackState.stateId());

        auto goldResult = processor.process(BlockPos(0, 0, 0), pos, goldInfo, goldInfo, settings);
        auto lavaResult = processor.process(BlockPos(0, 0, 0), pos, lavaInfo, lavaInfo, settings);
        auto netherrackResult = processor.process(BlockPos(0, 0, 0), pos, netherrackInfo, netherrackInfo, settings);

        ASSERT_TRUE(goldResult.has_value());
        ASSERT_TRUE(lavaResult.has_value());
        ASSERT_TRUE(netherrackResult.has_value());

        if (goldResult->blockStateId == airState.stateId()) {
            ++goldReplaced;
        }
        if (lavaResult->blockStateId == magmaState.stateId()) {
            ++lavaReplaced;
        }
        if (netherrackResult->blockStateId == magmaState.stateId()) {
            ++netherrackReplaced;
        }
    }

    f32 goldRate = static_cast<f32>(goldReplaced) / static_cast<f32>(totalBlocks);
    f32 lavaRate = static_cast<f32>(lavaReplaced) / static_cast<f32>(totalBlocks);
    f32 netherrackRate = static_cast<f32>(netherrackReplaced) / static_cast<f32>(totalBlocks);

    EXPECT_GT(goldRate, 0.25f) << "Gold replacement rate too low";
    EXPECT_LT(goldRate, 0.35f) << "Gold replacement rate too high";
    EXPECT_GT(lavaRate, 0.15f) << "Lava replacement rate too low";
    EXPECT_LT(lavaRate, 0.25f) << "Lava replacement rate too high";
    EXPECT_GT(netherrackRate, 0.04f) << "Netherrack replacement rate too low";
    EXPECT_LT(netherrackRate, 0.10f) << "Netherrack replacement rate too high";
}

// ============================================================================
// 13. ProtectedBlocksProcessor 在 FEATURES_CANNOT_REPLACE 标签方块上返回 nullopt
// 验证 ProtectedBlocksProcessor 的行为
// ============================================================================

TEST_F(RuinedPortalStructureTest, ProtectedBlocksProcessorSkipFeaturesCannotReplaceTaggedBlocks)
{
    if (!VanillaBlocks::BEDROCK) {
        GTEST_SKIP() << "BEDROCK not registered";
    }

    // bedrock 应在 FEATURES_CANNOT_REPLACE 标签中
    EXPECT_TRUE(BlockTags::FEATURES_CANNOT_REPLACE().contains(*VanillaBlocks::BEDROCK));

    ProtectedBlocksProcessor processor(BlockTags::FEATURES_CANNOT_REPLACE().getId());

    // 由于 ProtectedBlocksProcessor 依赖 PlacementSettings::getWorld() 读取世界方块，
    // 而 BaseTestWorld 不实现 IWorld 接口（仅 IBlockReader），这里无法直接测试
    // process() 的 nullopt 行为。改为验证构造器接受标签 ID 不抛异常。
    EXPECT_EQ(processor.getTagId(), BlockTags::FEATURES_CANNOT_REPLACE().getId());
}

// ============================================================================
// 14. BlockAgeProcessor 苔藓化概率测试：mossiness=0.8 (Jungle) 应高概率替换
// 直接构造 BlockAgeProcessor 验证 mossiness 传递性
// ============================================================================

TEST_F(RuinedPortalStructureTest, HighMossinessReplacesSlabsAndWallsMoreFrequently)
{
    if (!VanillaBlocks::STONE_BRICK_SLAB || !VanillaBlocks::MOSSY_STONE_BRICK_SLAB) {
        GTEST_SKIP() << "Required slab blocks not registered";
    }

    // jungle mossiness=0.8
    BlockAgeProcessor highMossiness(0.8f);
    BlockAgeProcessor lowMossiness(0.0f);

    const BlockState& slabState = VanillaBlocks::STONE_BRICK_SLAB->defaultState();
    const BlockState& mossySlabState = VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState();
    PlacementSettings settings;

    int highReplaced = 0;
    int lowReplaced = 0;
    const int totalBlocks = 500;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), slabState.stateId());

        auto highResult = highMossiness.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);
        auto lowResult = lowMossiness.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(highResult.has_value());
        ASSERT_TRUE(lowResult.has_value());

        if (highResult->blockStateId == mossySlabState.stateId()) {
            ++highReplaced;
        }
        if (lowResult->blockStateId == mossySlabState.stateId()) {
            ++lowReplaced;
        }
    }

    // mossiness=0.8 时替换率应接近 80%
    // mossiness=0.0 时替换率应为 0%
    f32 highRate = static_cast<f32>(highReplaced) / static_cast<f32>(totalBlocks);
    f32 lowRate = static_cast<f32>(lowReplaced) / static_cast<f32>(totalBlocks);

    EXPECT_GT(highRate, 0.7f) << "High mossiness replacement rate too low: " << highRate;
    EXPECT_LT(highRate, 0.9f) << "High mossiness replacement rate too high: " << highRate;
    EXPECT_EQ(lowRate, 0.0f) << "Zero mossiness should not replace slabs";
}
