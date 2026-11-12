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
#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include "server/world/gen/structure/structures/IglooStructure.hpp"
#include "server/world/gen/structure/structures/NetherFossilStructure.hpp"
#include "server/world/gen/structure/structures/OceanRuinStructure.hpp"
#include "server/world/gen/structure/structures/RuinedPortalStructure.hpp"
#include "server/world/gen/structure/structures/ShipwreckStructure.hpp"
#include "world/biome/Biome.hpp"
#include <memory>
#include <set>

using namespace mc::world::gen::structure;
using namespace mc::world::gen::feature::template_;

namespace {

// 测试模板名称列表的完整性

TEST(TemplatedStructuresTest, NetherFossilHasCorrectTemplateCount)
{
    // MC 1.16.5: 下界化石有14个模板
    EXPECT_EQ(NetherFossilStructure::s_fossilTemplates.size(), 14u);

    // 验证模板名称格式
    for (const auto& name : NetherFossilStructure::s_fossilTemplates) {
        EXPECT_TRUE(name.find("nether_fossils/fossil_") != std::string::npos) << "Invalid template name: " << name;
    }
}

TEST(TemplatedStructuresTest, ShipwreckHasCorrectTemplateCount)
{
    // MC 1.16.5: 搁浅沉船有11个模板
    EXPECT_EQ(ShipwreckStructure::s_beachedTemplates.size(), 11u);

    // MC 1.16.5: 所有沉船有20个模板
    EXPECT_EQ(ShipwreckStructure::s_allTemplates.size(), 20u);

    // 验证模板名称格式
    for (const auto& name : ShipwreckStructure::s_beachedTemplates) {
        EXPECT_TRUE(name.find("shipwreck/") != std::string::npos) << "Invalid template name: " << name;
    }
}

TEST(TemplatedStructuresTest, OceanRuinHasCorrectTemplateCount)
{
    // MC 1.16.5: 暖海废墟
    EXPECT_EQ(OceanRuinStructure::s_warmTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_warmBigTemplates.size(), 4u);

    // MC 1.16.5: 冷海废墟（砖、裂纹、苔藓）
    EXPECT_EQ(OceanRuinStructure::s_brickTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_brickBigTemplates.size(), 4u);
    EXPECT_EQ(OceanRuinStructure::s_crackedTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_crackedBigTemplates.size(), 4u);
    EXPECT_EQ(OceanRuinStructure::s_mossyTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_mossyBigTemplates.size(), 4u);
}

TEST(TemplatedStructuresTest, IglooHasCorrectTemplateNames)
{
    // MC 1.16.5: 雪屋有3个模板
    EXPECT_EQ(IglooStructure::s_topTemplateName, "igloo/top");
    EXPECT_EQ(IglooStructure::s_middleTemplateName, "igloo/middle");
    EXPECT_EQ(IglooStructure::s_bottomTemplateName, "igloo/bottom");
}

TEST(TemplatedStructuresTest, RuinedPortalHasCorrectTemplateCount)
{
    // MC 1.16.5: 普通传送门10个，巨型传送门3个
    EXPECT_EQ(RuinedPortalStructure::getNormalTemplates().size(), 10u);
    EXPECT_EQ(RuinedPortalStructure::getGiantTemplates().size(), 3u);

    // 验证模板名称格式
    for (const auto& name : RuinedPortalStructure::getNormalTemplates()) {
        EXPECT_TRUE(name.find("ruined_portal/portal_") != std::string::npos) << "Invalid template name: " << name;
    }
    for (const auto& name : RuinedPortalStructure::getGiantTemplates()) {
        EXPECT_TRUE(name.find("ruined_portal/giant_portal_") != std::string::npos) << "Invalid template name: " << name;
    }
}

// 测试结构片段属性

TEST(TemplatedStructuresTest, IglooPieceKeepsTemplatePosition)
{
    IglooPiece top(IglooStructure::s_topTemplateName, mc::BlockPos(0, 64, 0), Rotation::None);
    EXPECT_EQ(top.templateName(), IglooStructure::s_topTemplateName);
    EXPECT_EQ(top.templatePosition(), mc::BlockPos(0, 64, 0));
}

TEST(TemplatedStructuresTest, IglooPieceRotationShrinksBox)
{
    // 90 度旋转会交换包围盒的 X/Z 跨度：用同一模板的两种旋转比较跨度而非具体坐标。
    IglooPiece none(IglooStructure::s_topTemplateName, mc::BlockPos(0, 64, 0), Rotation::None);
    IglooPiece rotated(IglooStructure::s_topTemplateName, mc::BlockPos(0, 64, 0), Rotation::Clockwise90);

    EXPECT_LE(none.getBoundingBox().xSpan(), rotated.getBoundingBox().xSpan());
}

TEST(TemplatedStructuresTest, OceanRuinConfigCarriesPackProbabilities)
{
    OceanRuinConfig config{OceanRuinType::Cold, 0.3f, 0.9f};
    EXPECT_FLOAT_EQ(config.largeProbability, 0.3f);
    EXPECT_FLOAT_EQ(config.clusterProbability, 0.9f);
    EXPECT_EQ(config.biomeType, OceanRuinType::Cold);
}

TEST(TemplatedStructuresTest, ShipwreckPieceProperties)
{
    ShipwreckPiece piece("shipwreck/with_mast", mc::BlockPos(100, 50, 200), Rotation::Clockwise90, true);

    EXPECT_EQ(piece.templateName(), "shipwreck/with_mast");
    EXPECT_TRUE(piece.isBeached());
}

TEST(TemplatedStructuresTest, NetherFossilPieceProperties)
{
    NetherFossilPiece piece("nether_fossils/fossil_5", mc::BlockPos(0, 45, 0), Rotation::Clockwise180);

    EXPECT_EQ(piece.templateName(), "nether_fossils/fossil_5");
}

TEST(TemplatedStructuresTest, RuinedPortalPieceProperties)
{
    RuinedPortalProperties props;
    props.cold = true;
    props.mossiness = 0.8f;
    props.airPocket = true;
    props.vines = true;

    RuinedPortalPiece piece("ruined_portal/portal_1",
        mc::BlockPos(0, 64, 0),
        Rotation::None,
        Mirror::None,
        RuinedPortalLocation::OnLandSurface,
        props,
        mc::BlockPos(0, 64, 0));

    EXPECT_EQ(piece.templateName(), "minecraft:ruined_portal/portal_1");
    EXPECT_EQ(piece.location(), RuinedPortalLocation::OnLandSurface);
    EXPECT_TRUE(piece.properties().cold);
    EXPECT_FLOAT_EQ(piece.properties().mossiness, 0.8f);
    EXPECT_TRUE(piece.properties().airPocket);
    EXPECT_TRUE(piece.properties().vines);
}

TEST(TemplatedStructuresTest, OceanRuinPieceProperties)
{
    OceanRuinPiece piece(
        "underwater_ruin/brick_1", mc::BlockPos(0, 30, 0), Rotation::Clockwise90, 0.8f, OceanRuinType::Cold, false);

    EXPECT_EQ(piece.templateName(), "minecraft:underwater_ruin/brick_1");
    EXPECT_FLOAT_EQ(piece.integrity(), 0.8f);
    EXPECT_EQ(piece.ruinType(), OceanRuinType::Cold);
    EXPECT_FALSE(piece.isLarge());
}

// ============================================================================
// 下界化石的干燥恶魂放置（NetherFossilPiece::placeDriedGhast）
// ============================================================================

namespace {

/// 记录写入的测试世界：未写入的位置一律视为空气，世界种子可配置。
///
/// `MapBackedTestWorld::getBlockState` 对未写入位置返回 nullptr，而干燥恶魂的放置
/// 判定要求「读到的确实是空气」，因此这里把缺失位置补成空气。
class _DriedGhastTestWorld final : public mc::test::MapBackedTestWorld {
public:
    _DriedGhastTestWorld(mc::u64 seed, const mc::BlockState* air)
        : MapBackedTestWorld(mc::Difficulty::Easy)
        , m_seed(seed)
        , m_air(air)
        , m_writeCount(0)
    {}

    [[nodiscard]] const mc::BlockState* getBlockState(mc::i32 x, mc::i32 y, mc::i32 z) const override
    {
        const mc::BlockState* stored = MapBackedTestWorld::getBlockState(x, y, z);
        return stored != nullptr ? stored : m_air;
    }

    bool setBlockState(mc::i32 x, mc::i32 y, mc::i32 z, const mc::BlockState* state) override
    {
        ++m_writeCount;
        return MapBackedTestWorld::setBlockState(x, y, z, state);
    }

    [[nodiscard]] mc::u64 seed() const override
    {
        return m_seed;
    }

    [[nodiscard]] mc::i32 writeCount() const
    {
        return m_writeCount;
    }

private:
    mc::u64 m_seed;
    const mc::BlockState* m_air;
    mc::i32 m_writeCount;
};

/// 与被测实现相同的随机源口径：世界种子 → forkPositional → 模板包围盒中心。
std::unique_ptr<mc::math::IRandom> _fossilRandom(mc::u64 seed, const StructureBoundingBox& bounds)
{
    mc::math::JavaLegacyRandom levelRandom(seed);
    mc::math::PositionalRandomFactory factory = levelRandom.forkPositional();
    return factory.at(bounds.centerX(), bounds.centerY(), bounds.centerZ());
}

/// 注册一个 5x3x5 的程序化空模板，供 NetherFossilPiece 载入。
const Template* _installEmptyFossilTemplate()
{
    auto& manager = mc::world::gen::jigsaw::JigsawAssembler::getTemplateManager();
    const mc::resource::ResourceLocation id("minecraft", "nether_fossils/fossil_unit_test");
    manager.addTemplate(id, manager.createProceduralTemplate("fossil_unit_test", 5, 3, 5));
    const Template* templ = manager.getTemplate(id);
    EXPECT_NE(templ, nullptr);
    return templ;
}

} // namespace

TEST(TemplatedStructuresTest, NetherFossilPlacesDriedGhastWithRandomRotation)
{
    mc::VanillaBlocks::initialize();
    ASSERT_NE(_installEmptyFossilTemplate(), nullptr);

    const mc::BlockState* air = mc::VanillaBlocks::getState(mc::VanillaBlocks::AIR);
    ASSERT_NE(air, nullptr);

    NetherFossilPiece probe("nether_fossils/fossil_unit_test", mc::BlockPos(0, 45, 0), Rotation::None);
    const StructureBoundingBox bounds = probe.getBoundingBox();
    ASSERT_TRUE(probe.intersectsChunk(0, 0));

    // 放置分支由世界种子决定：找一个「命中放置且朝向非默认」的种子。
    mc::u64 seed = 1;
    mc::i32 expectedX = 0;
    mc::i32 expectedZ = 0;
    Rotation expectedRotation = Rotation::None;
    bool foundSeed = false;
    for (; seed <= 64; ++seed) {
        auto rng = _fossilRandom(seed, bounds);
        if (rng->nextFloat() >= 0.5f) {
            continue;
        }
        expectedX = bounds.minX() + rng->nextInt(bounds.xSpan());
        expectedZ = bounds.minZ() + rng->nextInt(bounds.zSpan());
        expectedRotation = static_cast<Rotation>(rng->nextInt(4));
        // 只挑「朝向非默认」的种子，否则断言无法区分「应用了旋转」与「直接用默认状态」。
        if (expectedRotation != Rotation::None) {
            foundSeed = true;
            break;
        }
    }
    ASSERT_TRUE(foundSeed) << "no world seed within 1..64 rotates the dried ghast";

    _DriedGhastTestWorld world(seed, air);
    mc::math::Random rng(1);
    NetherFossilPiece piece("nether_fossils/fossil_unit_test", mc::BlockPos(0, 45, 0), Rotation::None);
    const StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(0, 0);
    piece.generate(world, rng, 0, 0, chunkBounds);

    ASSERT_EQ(world.writeCount(), 1) << "the piece should write exactly one block (the dried ghast)";
    const mc::BlockState* placed = world.getBlockState(expectedX, bounds.minY(), expectedZ);
    ASSERT_NE(placed, nullptr);
    EXPECT_TRUE(placed->is(mc::VanillaBlocks::DRIED_GHAST));
    // 朝向由 `Rotation.getRandom` 决定：默认朝北，按旋转映射到对应水平方向。
    const auto defaultFacing =
        mc::VanillaBlocks::DRIED_GHAST->defaultState().get(mc::BlockStateProperties::HORIZONTAL_FACING());
    const auto placedFacing = placed->get(mc::BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_NE(placedFacing, defaultFacing);
    EXPECT_EQ(placedFacing, mc::Directions::rotateDirection(defaultFacing, expectedRotation));
    // 其余属性保持默认（湿润等级 0、不含水）。
    EXPECT_EQ(placed->get(mc::BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS()), 0);
    EXPECT_FALSE(placed->get(mc::BlockStateProperties::WATERLOGGED()));
}

TEST(TemplatedStructuresTest, NetherFossilSkipsDriedGhastWhenTargetIsNotAir)
{
    mc::VanillaBlocks::initialize();
    ASSERT_NE(_installEmptyFossilTemplate(), nullptr);

    const mc::BlockState* air = mc::VanillaBlocks::getState(mc::VanillaBlocks::AIR);
    const mc::BlockState* stone = mc::VanillaBlocks::getState(mc::VanillaBlocks::STONE);
    ASSERT_NE(air, nullptr);
    ASSERT_NE(stone, nullptr);

    NetherFossilPiece probe("nether_fossils/fossil_unit_test", mc::BlockPos(0, 45, 0), Rotation::None);
    const StructureBoundingBox bounds = probe.getBoundingBox();

    // 先把目标格子填成石头，放置分支命中时必须因「非空气」而放弃写入。
    mc::u64 seed = 1;
    for (; seed <= 64; ++seed) {
        if (_fossilRandom(seed, bounds)->nextFloat() < 0.5f) {
            break;
        }
    }
    ASSERT_LE(seed, 64u);

    auto rng = _fossilRandom(seed, bounds);
    (void)rng->nextFloat(); // 消耗放置概率那一次抽取，与实现顺序一致
    const mc::i32 targetX = bounds.minX() + rng->nextInt(bounds.xSpan());
    const mc::i32 targetZ = bounds.minZ() + rng->nextInt(bounds.zSpan());

    _DriedGhastTestWorld world(seed, air);
    ASSERT_TRUE(world.setBlockState(targetX, bounds.minY(), targetZ, stone));
    const mc::i32 writesBefore = world.writeCount();

    mc::math::Random chunkRng(1);
    NetherFossilPiece piece("nether_fossils/fossil_unit_test", mc::BlockPos(0, 45, 0), Rotation::None);
    piece.generate(world, chunkRng, 0, 0, StructureBoundingBox::fromChunk(0, 0));

    EXPECT_EQ(world.writeCount(), writesBefore) << "a non-air target must block the dried ghast placement";
    const mc::BlockState* kept = world.getBlockState(targetX, bounds.minY(), targetZ);
    ASSERT_NE(kept, nullptr);
    EXPECT_TRUE(kept->is(mc::VanillaBlocks::STONE));
}

} // namespace
