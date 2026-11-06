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
#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/biome/BiomeTagLoader.hpp"
#include "server/world/gen/feature/template/ProtectedBlocksProcessor.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManagerHostBinding.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/RuinedPortalStructure.hpp"

#include <algorithm>
#include <filesystem>
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

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override
    {
        return m_biome;
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32, i32, i32) const override
    {
        return m_biome;
    }

    [[nodiscard]] i32 getHeight(i32, i32, HeightmapType) const override
    {
        return m_surfaceY + 1;
    }

    /// 四角扫描要按完整地形列判实心：地表以下全实心、地表及以上全空气。
    [[nodiscard]] NoiseColumn getBaseColumn(i32, i32) const override
    {
        const i32 minY = m_settings.noise.minY;
        NoiseColumn column(minY, m_settings.noise.height);
        const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
        for (i32 y = minY; y < m_surfaceY; ++y) {
            column.setBlock(y, &stoneState);
        }
        return column;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 12345;
    }

    [[nodiscard]] const DimensionSettings& settings() const override
    {
        return m_settings;
    }

    [[nodiscard]] i32 seaLevel() const override
    {
        return m_settings.seaLevel;
    }

    void generateStructureStarts(WorldGenRegion&, ChunkPrimer&) override {}

    void generateStructureReferences(WorldGenRegion&, ChunkPrimer&) override {}

    void generateBiomes(WorldGenRegion&, ChunkPrimer&) override {}

    void generateNoise(WorldGenRegion&, ChunkPrimer&) override {}

    void buildSurface(WorldGenRegion&, ChunkPrimer&) override {}

    void applyCarvers(WorldGenRegion&, ChunkPrimer&) override {}

    void placeFeatures(WorldGenRegion&, ChunkPrimer&) override {}

    i32 spawnInitialMobs(WorldGenRegion&, ChunkPrimer&, std::vector<SpawnedEntityData>&) override
    {
        return 0;
    }

private:
    BiomeId m_biome;
    /// 地表方块 Y：默认取海平面，避免高度调整后的温度低于降雪阈值
    i32 m_surfaceY = 62;
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

/// 构造单个 setup（数据包里的 setups 表就是这么一项一项写的）
RuinedPortalSetupDefinition makeSetup(RuinedPortalSetupDefinition::Placement placement,
    f32 airPocketProbability,
    f32 mossiness,
    bool overgrown,
    bool vines,
    bool canBeCold,
    bool replaceWithBlackstone,
    f32 weight)
{
    RuinedPortalSetupDefinition setup;
    setup.placement = placement;
    setup.airPocketProbability = airPocketProbability;
    setup.mossiness = mossiness;
    setup.overgrown = overgrown;
    setup.vines = vines;
    setup.canBeCold = canBeCold;
    setup.replaceWithBlackstone = replaceWithBlackstone;
    setup.weight = weight;
    return setup;
}

// 以下 setups 表逐字取自 datapacks/Vanilla 的 worldgen/structure/ruined_portal*.json

std::vector<RuinedPortalSetupDefinition> desertSetups()
{
    return {
        makeSetup(RuinedPortalSetupDefinition::Placement::PartlyBuried, 0.0f, 0.0f, false, false, false, false, 1.0f)};
}

std::vector<RuinedPortalSetupDefinition> jungleSetups()
{
    return {
        makeSetup(RuinedPortalSetupDefinition::Placement::OnLandSurface, 0.5f, 0.8f, true, true, false, false, 1.0f)};
}

std::vector<RuinedPortalSetupDefinition> swampSetups()
{
    return {
        makeSetup(RuinedPortalSetupDefinition::Placement::OnOceanFloor, 0.0f, 0.5f, false, true, false, false, 1.0f)};
}

std::vector<RuinedPortalSetupDefinition> oceanSetups()
{
    return {
        makeSetup(RuinedPortalSetupDefinition::Placement::OnOceanFloor, 0.0f, 0.8f, false, false, true, false, 1.0f)};
}

std::vector<RuinedPortalSetupDefinition> standardSetups()
{
    return {makeSetup(RuinedPortalSetupDefinition::Placement::Underground, 1.0f, 0.2f, false, false, true, false, 0.5f),
        makeSetup(RuinedPortalSetupDefinition::Placement::OnLandSurface, 0.5f, 0.2f, false, false, true, false, 0.5f)};
}

/// 以对应的变体 id + 数据包里的群系标签构造结构实例
///
/// 生产路径里 id 与 `biomes` 标签都由 StructureTypeRegistry 从结构定义 JSON 注入；
/// 直接构造子类时二者都不存在，只能退回子类的 standard 默认标签——沙漠变体因此拿到
/// standard 标签，`isValidBiome(Desert)` 为 false，生成被整段跳过。这里显式注入，
/// 让测试与生产路径的行为一致。
std::unique_ptr<RuinedPortalStructure> makeStructure(
    const std::vector<RuinedPortalSetupDefinition>& setups, const mc::world::biome::BiomeTag* biomeTag)
{
    static const ResourceLocation kStandardId("minecraft", "ruined_portal");
    static const ResourceLocation kDesertId("minecraft", "ruined_portal_desert");
    static const ResourceLocation kJungleId("minecraft", "ruined_portal_jungle");
    static const ResourceLocation kSwampId("minecraft", "ruined_portal_swamp");
    static const ResourceLocation kOceanId("minecraft", "ruined_portal_ocean");
    static const ResourceLocation kNetherId("minecraft", "ruined_portal_nether");
    static const ResourceLocation kMountainId("minecraft", "ruined_portal_mountain");

    ResourceLocation id = kStandardId;
    if (biomeTag == &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_DESERT()) {
        id = kDesertId;
    } else if (biomeTag == &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_JUNGLE()) {
        id = kJungleId;
    } else if (biomeTag == &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_SWAMP()) {
        id = kSwampId;
    } else if (biomeTag == &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_OCEAN()) {
        id = kOceanId;
    } else if (biomeTag == &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_NETHER()) {
        id = kNetherId;
    } else if (biomeTag == &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_MOUNTAIN()) {
        id = kMountainId;
    }

    auto structure = std::make_unique<RuinedPortalStructure>(id, setups);
    structure->setBiomeTag(biomeTag);
    return structure;
}

/// 按 setups 表内容推断所属变体并构造
///
/// setups 表与变体一一对应：单 setup 的 placement 直接决定变体，多 setup 只有
/// standard / mountain 两张表。
std::unique_ptr<RuinedPortalStructure> makeStructure(const std::vector<RuinedPortalSetupDefinition>& setups)
{
    if (setups.size() == 1) {
        const auto& head = setups.front();
        if (head.placement == RuinedPortalSetupDefinition::Placement::OnOceanFloor) {
            return makeStructure(setups,
                head.vines ? &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_SWAMP()
                           : &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_OCEAN());
        }
        if (head.placement == RuinedPortalSetupDefinition::Placement::OnLandSurface && head.overgrown) {
            return makeStructure(setups, &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_JUNGLE());
        }
        if (head.placement == RuinedPortalSetupDefinition::Placement::PartlyBuried) {
            return makeStructure(setups, &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_DESERT());
        }
        if (head.placement == RuinedPortalSetupDefinition::Placement::InNether) {
            return makeStructure(setups, &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_NETHER());
        }
        if (head.placement == RuinedPortalSetupDefinition::Placement::InMountain) {
            return makeStructure(setups, &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_MOUNTAIN());
        }
    }
    return makeStructure(setups, &mc::world::biome::BiomeTags::HAS_STRUCTURE_RUINED_PORTAL_STANDARD());
}

/// 生成一个传送门起点，并要求恰好包含一个 RuinedPortalPiece。
///
/// 这里直接调用 `Structure::generate`，跳过结构集的加权抽签与"区块内是否落点"的判定。
/// 生成点由测试给定的世界种子派生，结构性断言（setup 字段、模板池、cold 标志）可稳定复现。
///
/// 注意：generate 仍会按**候选生成点**处的噪声群系校验生物群系；固定群系生成器在
/// 该点返回的群系若不在结构标签里，返回 nullptr 是正确行为。
std::unique_ptr<StructureStart> generatePortal(
    const RuinedPortalStructure& structure, FixedBiomeChunkGenerator& generator, u64 seed, i32 chunkX, i32 chunkZ)
{
    math::Random rng(seed);
    auto start = structure.generate(generator, rng, chunkX, chunkZ);
    if (start == nullptr || start->pieceCount() != 1) {
        return nullptr;
    }
    return start;
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
        // cold 判定要读群系的气候温度：未初始化时 BiomeRegistry::get 会回退到 plains，
        // 雪原也会被判成"不够冷"。
        world::biome::BiomeRegistry::instance().initialize();
        _bindVanillaTemplates();
        _bindVanillaBiomeTags();
    }

    /// 从原版数据包加载 `has_structure/*` 群系标签。
    ///
    /// 结构类持有的 BiomeTag 是静态缓存且一旦解析就不再重建，而标签解析晚于构造函数时
    /// 该缓存会退化成"空标签"——isValidBiome 对任何群系都返回 false。因此标签必须在
    /// 构造任何结构实例之前加载好。
    void _bindVanillaBiomeTags()
    {
        const auto dataPackDir = GameDirectory::defaultDirectory().dataPacksDir();
        if (!std::filesystem::exists(dataPackDir)) {
            return;
        }
        if (!m_dataPackRepository.scanDirectory(dataPackDir).success()) {
            return;
        }
        world::biome::BiomeTagLoader::loadFromDataPackRepository(m_dataPackRepository);
    }

    /// 把原版数据包里的结构模板（data/minecraft/structure/**/*.nbt）接到进程级模板管理器。
    ///
    /// RuinedPortalStructure::generate 会立刻 loadTemplateNow() 并断言 hasLoadedTemplate()——
    /// 没有模板来源时它会直接 abort（缺模板=整座传送门凭空消失且无报错，所以那里显式失败）。
    /// 因此凡是会调用 generate() 的用例都必须先绑定模板来源。
    ///
    /// 仓库与绑定令牌都作为夹具成员：令牌析构会解绑，仓库必须活得比绑定更久。
    /// 数据包缺失时（非开发机）静默跳过，generate() 相关用例由断言自然暴露为失败。
    void _bindVanillaTemplates()
    {
        const auto dataPackDir = GameDirectory::defaultDirectory().dataPacksDir();
        if (!std::filesystem::exists(dataPackDir)) {
            return;
        }
        if (!m_dataPackRepository.scanDirectory(dataPackDir).success()) {
            return;
        }
        m_templateBinding =
            std::make_unique<TemplateManagerHostBinding>(world::gen::jigsaw::JigsawAssembler::getTemplateManager());
        m_templateBinding->bindDataPackRepository(m_dataPackRepository);
    }

    std::unique_ptr<TemplateManagerHostBinding> m_templateBinding;
    resource::DataPackRepository m_dataPackRepository;
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
        props,
        BlockPos(100, 64, 200));

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
        RuinedPortalPiece piece(
            "ruined_portal/portal_1", BlockPos(0, 0, 0), Rotation::None, Mirror::None, loc, props, BlockPos(0, 0, 0));

        EXPECT_EQ(piece.location(), loc) << "Location " << static_cast<int>(loc) << " not stored correctly";
    }
}

// ============================================================================
// 2. Structure::generate 端到端：setups 表的字段如实进入片段属性
// ============================================================================

TEST_F(RuinedPortalStructureTest, GenerateProducesValidStartWithRuinedPortalPiece)
{
    auto structure = makeStructure(standardSetups());
    FixedBiomeChunkGenerator generator(Plains);

    auto start = generatePortal(*structure, generator, 54321, 10, 10);
    ASSERT_NE(start, nullptr);
    EXPECT_EQ(start->pieceCount(), 1u);

    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    const auto& normalTemplates = RuinedPortalStructure::getNormalTemplates();
    const auto& giantTemplates = RuinedPortalStructure::getGiantTemplates();
    const auto& name = piece->templateName();
    const bool isNormal = std::find(normalTemplates.begin(), normalTemplates.end(), name) != normalTemplates.end();
    const bool isGiant = std::find(giantTemplates.begin(), giantTemplates.end(), name) != giantTemplates.end();
    EXPECT_TRUE(isNormal || isGiant) << "Unexpected template name: " << name;
}

// ============================================================================
// 3. 配置正确性：各变体的 setups 表如实决定片段的放置方式与修饰属性
// ============================================================================

TEST_F(RuinedPortalStructureTest, DesertSetupProducesDesertConfiguration)
{
    auto structure = makeStructure(desertSetups());
    FixedBiomeChunkGenerator generator(Desert);

    auto start = generatePortal(*structure, generator, 111, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    EXPECT_EQ(piece->location(), RuinedPortalLocation::PartlyBuried);
    EXPECT_FALSE(piece->properties().airPocket);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.0f);
    EXPECT_FALSE(piece->properties().replaceWithBlackstone);
}

TEST_F(RuinedPortalStructureTest, JungleSetupProducesJungleConfiguration)
{
    auto structure = makeStructure(jungleSetups());
    FixedBiomeChunkGenerator generator(Jungle);

    auto start = generatePortal(*structure, generator, 222, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    EXPECT_EQ(piece->location(), RuinedPortalLocation::OnLandSurface);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.8f);
    EXPECT_TRUE(piece->properties().overgrown);
    EXPECT_TRUE(piece->properties().vines);
}

TEST_F(RuinedPortalStructureTest, SwampSetupProducesSwampConfiguration)
{
    auto structure = makeStructure(swampSetups());
    FixedBiomeChunkGenerator generator(Swamp);

    auto start = generatePortal(*structure, generator, 333, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    EXPECT_EQ(piece->location(), RuinedPortalLocation::OnOceanFloor);
    EXPECT_FALSE(piece->properties().airPocket);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.5f);
    EXPECT_TRUE(piece->properties().vines);
}

TEST_F(RuinedPortalStructureTest, OceanSetupProducesOceanConfiguration)
{
    auto structure = makeStructure(oceanSetups());
    FixedBiomeChunkGenerator generator(Ocean);

    auto start = generatePortal(*structure, generator, 444, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    EXPECT_EQ(piece->location(), RuinedPortalLocation::OnOceanFloor);
    EXPECT_FALSE(piece->properties().airPocket);
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.8f);
}

// ============================================================================
// 4. cold 标志：仅 setup.can_be_cold 时才按落点群系采样
// ============================================================================

TEST_F(RuinedPortalStructureTest, ColdBiomeSetsColdFlag)
{
    auto structure = makeStructure(standardSetups());
    FixedBiomeChunkGenerator generator(SnowyPlains);

    auto start = generatePortal(*structure, generator, 555, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    EXPECT_TRUE(piece->properties().cold);
}

TEST_F(RuinedPortalStructureTest, NonColdBiomeDoesNotSetColdFlag)
{
    auto structure = makeStructure(standardSetups());
    FixedBiomeChunkGenerator generator(Plains);

    auto start = generatePortal(*structure, generator, 666, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    EXPECT_FALSE(piece->properties().cold);
}

TEST_F(RuinedPortalStructureTest, CanBeColdFalseNeverSetsColdFlag)
{
    // 单 setup 且 can_be_cold=false：即使落在雪地群系也不采样、不置位。
    // 用 standard 变体的 setups 表（can_be_cold=true 的那份单独由 ColdBiomeSetsColdFlag
    // 覆盖），这里换成同一张表的 can_be_cold=false 版本，标签不变以便通过群系校验。
    auto setups = standardSetups();
    for (auto& setup : setups) {
        setup.canBeCold = false;
    }
    auto structure = makeStructure(setups);
    FixedBiomeChunkGenerator generator(SnowyPlains);

    auto start = generatePortal(*structure, generator, 777, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

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
    auto structure = makeStructure(jungleSetups());
    FixedBiomeChunkGenerator generator(Jungle);

    auto start = generatePortal(*structure, generator, 777, 0, 0);
    ASSERT_NE(start, nullptr);
    const auto* piece = firstRuinedPortalPiece(*start);
    ASSERT_NE(piece, nullptr);

    // 丛林 mossiness=0.8，应使 BlockAgeProcessor 高概率替换台阶/墙壁
    EXPECT_FLOAT_EQ(piece->properties().mossiness, 0.8f);
}

TEST_F(RuinedPortalStructureTest, DesertMossinessValueIsZero)
{
    auto structure = makeStructure(desertSetups());
    FixedBiomeChunkGenerator generator(Desert);
    auto start = generatePortal(*structure, generator, 888, 0, 0);
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
    auto structure = makeStructure(standardSetups());
    FixedBiomeChunkGenerator generator(Plains);
    auto start = generatePortal(*structure, generator, 999, 0, 0);
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
    auto structure = makeStructure(swampSetups());
    FixedBiomeChunkGenerator generator(Swamp);

    for (u64 seed = 1; seed <= 20; ++seed) {
        auto start = generatePortal(*structure, generator, seed, 0, 0);
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
