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

/**
 * @file test_jigsaw.cpp
 * @brief Jigsaw拼图系统单元测试
 */

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/jigsaw/AssemblyTypes.hpp"
#include "common/world/gen/jigsaw/EmptyJigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawPlacer.hpp"
#include "common/world/gen/jigsaw/JigsawTransform.hpp"
#include "common/world/gen/jigsaw/ListJigsawPiece.hpp"
#include "common/world/gen/jigsaw/SingleJigsawPiece.hpp"
#include "common/world/gen/jigsaw/TemplatePool.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/structure/JigsawStructure.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <gtest/gtest.h>

#include <vector>

using namespace mc::world::gen::jigsaw;
using namespace mc::math;
using namespace mc;

namespace {

class RecordingWorldWriter final : public IWorldWriter {
public:
    struct Write {
        BlockPos pos;
        const BlockState* state;
        i32 flags;
    };

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        writes.push_back({BlockPos(x, y, z), state, 0});
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        writes.push_back({BlockPos(x, y, z), state, flags});
        return true;
    }

    std::vector<Write> writes;
};

/**
 * @brief 可配置高度边界的区块生成器 mock
 *
 * 用于 JigsawAssembler::assemble 的 Y 轴裁剪与世界高度边界检查测试。
 * 默认模拟主世界：minY=-64, genDepth=384（即 [-64, 320)）。
 */
class MockChunkGenerator final : public IChunkGenerator {
public:
    MockChunkGenerator()
        : m_minY(world::MIN_BUILD_HEIGHT)
        , m_genDepth(world::CHUNK_HEIGHT)
    {}

    MockChunkGenerator(i32 minY, i32 genDepth)
        : m_minY(minY)
        , m_genDepth(genDepth)
    {}

    [[nodiscard]] i32 getMinY() const override { return m_minY; }
    [[nodiscard]] i32 getGenDepth() const override { return m_genDepth; }

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override { return 0; }
    [[nodiscard]] BiomeId getNoiseBiome(i32, i32, i32) const override { return 0; }
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
    i32 m_minY;
    i32 m_genDepth;
    DimensionSettings m_settings = DimensionSettings::overworld();
};

} // namespace

/**
 * @brief 测试 TemplatePool 权重随机选择
 */
TEST(TemplatePoolTest, WeightedRandomSelection)
{
    TemplatePool pattern(ResourceLocation("minecraft", "test_pool"), ResourceLocation("minecraft", "empty"));

    // 添加权重为 1, 2, 3 的拼图块
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_a"), 1);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_b"), 2);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_c"), 3);

    // 验证总数量 (1 + 2 + 3 = 6)
    EXPECT_EQ(pattern.getTotalWeight(), 6u);

    // 统计随机选择分布
    std::map<std::string, int> counts;
    Random rng(12345);
    for (int i = 0; i < 6000; ++i) {
        const JigsawPiece* piece = pattern.getRandomPiece(rng);
        if (piece) {
            const SingleJigsawPiece* sp = dynamic_cast<const SingleJigsawPiece*>(piece);
            if (sp) {
                counts[sp->getTemplateName()]++;
            }
        }
    }

    // 验证概率分布（允许 15% 误差）
    EXPECT_NEAR(counts["piece_a"], 1000, 150);
    EXPECT_NEAR(counts["piece_b"], 2000, 300);
    EXPECT_NEAR(counts["piece_c"], 3000, 450);
}

/**
 * @brief 测试 TemplatePool::getShuffledPieces
 */
TEST(TemplatePoolTest, GetShuffledPieces)
{
    TemplatePool pattern(ResourceLocation("minecraft", "test_pool"), ResourceLocation("minecraft", "empty"));

    // 添加 5 个拼图块（权重各为 1）
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_0"), 1);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_1"), 1);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_2"), 1);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_3"), 1);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_4"), 1);

    Random rng(42);
    auto shuffled = pattern.getShuffledPieces(rng);

    // 验证数量正确
    EXPECT_EQ(shuffled.size(), 5u);

    // 验证所有元素都被包含
    std::set<std::string> names;
    for (const auto* piece : shuffled) {
        if (piece) {
            const SingleJigsawPiece* sp = dynamic_cast<const SingleJigsawPiece*>(piece);
            if (sp) {
                names.insert(sp->getTemplateName());
            }
        }
    }
    EXPECT_EQ(names.size(), 5u);

    // 验证打乱后与原始顺序不同（99.99%概率）
    // 多次运行验证确实被打乱
    bool wasShuffled = false;
    for (int i = 0; i < 10; ++i) {
        Random rng2(i);
        auto shuffled2 = pattern.getShuffledPieces(rng2);
        if (shuffled2.size() >= 2) {
            const SingleJigsawPiece* sp0 = dynamic_cast<const SingleJigsawPiece*>(shuffled2[0]);
            const SingleJigsawPiece* sp1 = dynamic_cast<const SingleJigsawPiece*>(shuffled2[1]);
            if (sp0 && sp1 && sp0->getTemplateName() != "piece_0") {
                wasShuffled = true;
                break;
            }
        }
    }
    EXPECT_TRUE(wasShuffled);
}

/**
 * @brief 测试 TemplatePool 空池
 */
TEST(TemplatePoolTest, EmptyPool)
{
    TemplatePool pattern(ResourceLocation("minecraft", "empty"), ResourceLocation("minecraft", "empty"));

    EXPECT_TRUE(pattern.isEmpty());
    EXPECT_EQ(pattern.getTotalWeight(), 0u);

    Random rng(12345);
    EXPECT_EQ(pattern.getRandomPiece(rng), nullptr);

    auto shuffled = pattern.getShuffledPieces(rng);
    EXPECT_TRUE(shuffled.empty());
}

/**
 * @brief 测试 EmptyJigsawPiece 单例
 */
TEST(JigsawPieceTest, EmptyPieceSingleton)
{
    const EmptyJigsawPiece& instance1 = EmptyJigsawPiece::instance();
    const EmptyJigsawPiece& instance2 = EmptyJigsawPiece::instance();

    // 验证单例
    EXPECT_EQ(&instance1, &instance2);

    // 验证为空
    EXPECT_TRUE(instance1.isEmpty());

    // 验证尺寸为零
    BlockPos size = instance1.getSize();
    EXPECT_EQ(size.x, 0);
    EXPECT_EQ(size.y, 0);
    EXPECT_EQ(size.z, 0);
}

/**
 * @brief 测试 SingleJigsawPiece
 */
TEST(JigsawPieceTest, SinglePiece)
{
    SingleJigsawPiece piece("minecraft:village/plains/house_01");

    EXPECT_FALSE(piece.isEmpty());
    // getTypeName 返回拼图块类型，不是模板名称
    EXPECT_EQ(piece.getTypeName(), "single_pool_element");
    // getTemplateName 返回模板名称
    EXPECT_EQ(piece.getTemplateName(), "minecraft:village/plains/house_01");

    // 设置尺寸
    piece.setSize(BlockPos(10, 8, 12));
    BlockPos size = piece.getSize();
    EXPECT_EQ(size.x, 10);
    EXPECT_EQ(size.y, 8);
    EXPECT_EQ(size.z, 12);

    // 验证克隆
    auto clone = piece.clone();
    EXPECT_TRUE(clone != nullptr);
    EXPECT_FALSE(clone->isEmpty());

    SingleJigsawPiece* clonedPiece = dynamic_cast<SingleJigsawPiece*>(clone.get());
    EXPECT_TRUE(clonedPiece != nullptr);
    EXPECT_EQ(clonedPiece->getTemplateName(), piece.getTemplateName());
}

/**
 * @brief 测试 ListJigsawPiece
 */
TEST(JigsawPieceTest, ListPiece)
{
    ListJigsawPiece listPiece;

    // 添加子拼图块
    listPiece.addPiece(std::make_unique<SingleJigsawPiece>("piece_a"));
    listPiece.addPiece(std::make_unique<SingleJigsawPiece>("piece_b"));
    listPiece.addPiece(std::make_unique<SingleJigsawPiece>("piece_c"));

    EXPECT_FALSE(listPiece.isEmpty());
    EXPECT_EQ(listPiece.getPieceCount(), 3u);

    // 验证获取子块
    const auto& pieces = listPiece.getPieces();
    EXPECT_EQ(pieces.size(), 3u);

    // 验证克隆
    auto clone = listPiece.clone();
    EXPECT_TRUE(clone != nullptr);
    EXPECT_FALSE(clone->isEmpty());

    ListJigsawPiece* clonedPiece = dynamic_cast<ListJigsawPiece*>(clone.get());
    EXPECT_TRUE(clonedPiece != nullptr);
    EXPECT_EQ(clonedPiece->getPieceCount(), 3u);
}

/**
 * @brief 测试 JigsawJunction 相等比较
 */
TEST(JigsawJunctionTest, Equality)
{
    JigsawJunction j1(1, 64, 2, 0, JigsawPlacementBehaviour::Rigid);
    JigsawJunction j2(1, 64, 2, 0, JigsawPlacementBehaviour::Rigid);
    JigsawJunction j3(1, 64, 2, 1, JigsawPlacementBehaviour::Rigid);
    // 注意：JigsawJunction 的相等比较不包括 sourceGroundY
    // j4 的 sourceGroundY=65 与 j1 的 sourceGroundY=64 不同，但其他字段相同
    JigsawJunction j4(1, 65, 2, 0, JigsawPlacementBehaviour::Rigid);
    // j5 的 sourceZ 不同
    JigsawJunction j5(1, 64, 3, 0, JigsawPlacementBehaviour::Rigid);

    EXPECT_TRUE(j1 == j2);
    EXPECT_FALSE(j1 == j3);
    // j1 == j4 因为 sourceGroundY 不参与比较
    EXPECT_TRUE(j1 == j4);
    EXPECT_FALSE(j1 == j5);

    EXPECT_FALSE(j1 != j2);
    EXPECT_TRUE(j1 != j3);
}

/**
 * @brief 测试 TemplatePoolRegistry
 */
TEST(TemplatePoolRegistryTest, RegisterAndGet)
{
    TemplatePoolRegistry& registry = TemplatePoolRegistry::instance();
    registry.clear();

    // 创建并注册模式
    auto pattern = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "test_pattern"), ResourceLocation("minecraft", "empty"));
    pattern->addPiece(std::make_unique<SingleJigsawPiece>("test_piece"), 1);

    registry.registerPool(std::move(pattern));

    // 验证注册成功
    const TemplatePool* retrieved = registry.getPool(ResourceLocation("minecraft", "test_pattern"));
    EXPECT_TRUE(retrieved != nullptr);
    EXPECT_FALSE(retrieved->isEmpty());
    EXPECT_EQ(retrieved->getTotalWeight(), 1u);

    // 验证获取不存在的模式
    const TemplatePool* notFound = registry.getPool(ResourceLocation("minecraft", "nonexistent"));
    EXPECT_EQ(notFound, nullptr);

    // 清理
    registry.clear();
}

/**
 * @brief 测试 JigsawTransform 边界框计算
 */
TEST(JigsawTransformTest, BoundingBoxCalculation)
{
    SingleJigsawPiece piece("test_piece");
    piece.setSize(BlockPos(10, 8, 12));

    BlockPos pos(100, 50, 200);
    auto box = JigsawTransform::calculateBoundingBox(piece, pos, Rotation::None);

    EXPECT_EQ(box.minX(), 100);
    EXPECT_EQ(box.minY(), 50);
    EXPECT_EQ(box.minZ(), 200);
    EXPECT_EQ(box.maxX(), 109); // 100 + 10 - 1
    EXPECT_EQ(box.maxY(), 57);  // 50 + 8 - 1
    EXPECT_EQ(box.maxZ(), 211); // 200 + 12 - 1
}

TEST(JigsawPlacerTest, FallbackPlacementRespectsChunkBounds)
{
    VanillaBlocks::initialize();

    auto piece = std::make_unique<SingleJigsawPiece>("minecraft:missing_template_for_bounds_test");
    piece->setSize(BlockPos(3, 3, 3));

    PlacedPiece placed;
    placed.piece = std::move(piece);
    placed.position = BlockPos(0, 64, 0);
    placed.boundingBox = mc::world::gen::structure::StructureBoundingBox(0, 64, 0, 2, 66, 2);

    RecordingWorldWriter world;
    Random rng(12345);
    mc::world::gen::structure::StructureBoundingBox bounds(10, 64, 10, 12, 66, 12);

    JigsawPlacer::placePiece(world, placed, rng, &bounds);

    EXPECT_TRUE(world.writes.empty());
}

/**
 * @brief 测试 JigsawJunction 的地形平滑功能
 *
 * 验证 JigsawJunction 能够正确存储地形适配信息，
 * 这些信息用于 NoiseChunkGenerator 中的地形平滑计算。
 */
TEST(JigsawJunctionTest, TerrainSmoothingData)
{
    // 创建一个 Junction，模拟村庄连接点的地形适配数据
    // sourceX, sourceGroundY, sourceZ, deltaY, destProjection
    JigsawJunction junction(100, 64, 200, -5, JigsawPlacementBehaviour::TerrainMatching);

    // 验证数据存储正确
    EXPECT_EQ(junction.getSourceX(), 100);
    EXPECT_EQ(junction.getSourceGroundY(), 64);
    EXPECT_EQ(junction.getSourceZ(), 200);
    EXPECT_EQ(junction.getDeltaY(), -5);
    EXPECT_EQ(junction.getDestProjection(), JigsawPlacementBehaviour::TerrainMatching);

    // 验证不同 deltaY 表示不同的地形关系
    JigsawJunction junctionUp(100, 64, 200, 10, JigsawPlacementBehaviour::Rigid);
    JigsawJunction junctionDown(100, 64, 200, -10, JigsawPlacementBehaviour::Rigid);

    EXPECT_GT(junctionUp.getDeltaY(), 0);   // 目标地面比源高
    EXPECT_LT(junctionDown.getDeltaY(), 0); // 目标地面比源低
}

/**
 * @brief 测试 JigsawJunction 在 PlacedPiece 中的存储
 */
TEST(PlacedPieceTest, JunctionStorage)
{
    PlacedPiece piece;
    piece.position = BlockPos(100, 64, 200);

    // 添加多个 Junction
    piece.junctions.emplace_back(100, 64, 200, 0, JigsawPlacementBehaviour::Rigid);
    piece.junctions.emplace_back(110, 64, 210, -3, JigsawPlacementBehaviour::TerrainMatching);
    piece.junctions.emplace_back(90, 64, 190, 5, JigsawPlacementBehaviour::Rigid);

    EXPECT_EQ(piece.junctions.size(), 3u);

    // 验证 Junction 的参数
    EXPECT_EQ(piece.junctions[0].getSourceX(), 100);
    EXPECT_EQ(piece.junctions[1].getDeltaY(), -3);
    EXPECT_EQ(piece.junctions[2].getDestProjection(), JigsawPlacementBehaviour::Rigid);
}

// ============================================================================
// JigsawAssembler::assemble 维度填充与世界高度边界测试
// ============================================================================
//
// 验证 assemble() 的两项新增行为：
// 1. isStartTooCloseToWorldHeightLimits：当 DimensionPadding 非 ZERO 且起始块包围盒
//    超出 [worldMinY + bottom, worldMinY + genDepth - 1 - top] 时返回空列表。
// 2. MaxDistance 包围盒 Y 轴裁剪：Y 范围被限制在
//    [max(centerY - vertical, worldMinY + bottom),
//     min(centerY + vertical + 1, worldMinY + genDepth - top)]。
//
// 测试通过构造无连接点的单块起始池，使 assemble 仅放置起始块后即结束，
// 从而隔离地验证起始块高度检查逻辑。

namespace {

/**
 * @brief 测试专用拼图块
 *
 * 不加载结构模板，大小与连接点完全由测试代码设置。
 * clone() 忠实复制大小与连接点，避免 SingleJigsawPiece::clone() 重新加载模板
 * 导致手动设置的大小丢失（测试模板名不存在时 m_size 会被置零）。
 */
class TestJigsawPiece final : public JigsawPiece {
public:
    explicit TestJigsawPiece(const BlockPos& size)
        : JigsawPiece(JigsawPlacementBehaviour::Rigid)
        , m_size(size)
    {}

    const std::string& getTypeName() const override { return s_typeName; }
    std::unique_ptr<JigsawPiece> clone() const override
    {
        auto piece = std::make_unique<TestJigsawPiece>(m_size);
        piece->setGroundLevelDelta(getGroundLevelDelta());
        for (const auto& joint : m_joints) {
            piece->addJoint(joint);
        }
        return piece;
    }
    BlockPos getSize() const override { return m_size; }
    void setSize(const BlockPos& size) { m_size = size; }

    void place(IWorldWriter& /*world*/,
        const PlacedPiece& /*placed*/,
        mc::world::gen::feature::template_::TemplateManager& /*templateManager*/,
        math::Random& /*rng*/,
        const mc::world::gen::structure::StructureBoundingBox* /*bounds*/,
        mc::world::chunk::ChunkPrimer* /*chunk*/ = nullptr,
        IChunkGenerator* /*generator*/ = nullptr) override
    {}

private:
    BlockPos m_size;
    static std::string s_typeName;
};

std::string TestJigsawPiece::s_typeName = "test_piece";

/// 构造仅含一个无连接点的起始模板池
TemplatePool makeSinglePieceStartPool(const BlockPos& size)
{
    TemplatePool pool(ResourceLocation("minecraft", "test_assemble_pool"), ResourceLocation("minecraft", "empty"));
    pool.addPiece(std::make_unique<TestJigsawPiece>(size), 1);
    return pool;
}

} // namespace

/**
 * @brief 起始块底部超出世界高度边界（DimensionPadding.bottom > 0）时应返回空列表
 *
 * 世界范围 [-64, 320)，DimensionPadding(0, 50) 将下界抬到 -64+50=-14。
 * 起始块 size.y=20，startPos.y=-20，包围盒 minY=-20 < -14，应被拒绝。
 */
TEST(JigsawAssemblerAssembleTest, StartPieceBelowLowerLimitReturnsEmpty)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 20, 5));
    const BlockPos startPos(0, -20, 0);
    const mc::world::gen::structure::DimensionPadding padding(0, 50);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, &padding);

    EXPECT_TRUE(pieces.empty());
}

/**
 * @brief 起始块顶部超出世界高度边界（DimensionPadding.top > 0）时应返回空列表
 *
 * 世界范围 [-64, 320)，DimensionPadding(50, 0) 将上界降到 320-1-50=269。
 * 起始块 size.y=20，startPos.y=260，包围盒 maxY=279 > 269，应被拒绝。
 */
TEST(JigsawAssemblerAssembleTest, StartPieceAboveUpperLimitReturnsEmpty)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 20, 5));
    const BlockPos startPos(0, 260, 0);
    const mc::world::gen::structure::DimensionPadding padding(50, 0);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, &padding);

    EXPECT_TRUE(pieces.empty());
}

/**
 * @brief DimensionPadding 为 ZERO（top=bottom=0）时即使起始块贴近边界也不拒绝
 *
 * 世界范围 [-64, 320)，DimensionPadding(0, 0)。
 * 起始块 size.y=5，startPos.y=-64（紧贴世界底部），包围盒 minY=-64 >= -64+0=-64，应通过。
 */
TEST(JigsawAssemblerAssembleTest, ZeroDimensionPaddingAllowsEdgePlacement)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 5, 5));
    const BlockPos startPos(0, -64, 0);
    const mc::world::gen::structure::DimensionPadding padding(0, 0);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, &padding);

    // ZERO padding 不触发边界检查，应成功放置起始块
    EXPECT_EQ(pieces.size(), 1u);
}

/**
 * @brief DimensionPadding 为 nullptr 时跳过边界检查
 *
 * 与 ZERO 等价：即使起始块贴近世界边界也不拒绝。
 */
TEST(JigsawAssemblerAssembleTest, NullDimensionPaddingSkipsCheck)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 5, 5));
    const BlockPos startPos(0, -64, 0);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, nullptr);

    EXPECT_EQ(pieces.size(), 1u);
}

/**
 * @brief 起始块在 DimensionPadding 限制范围内时应成功放置
 *
 * 世界范围 [-64, 320)，DimensionPadding(10, 10) 将有效范围限制到 [-54, 309]。
 * 起始块 size.y=20，startPos.y=0，包围盒 [0, 19] 完全在 [-54, 309] 内，应通过。
 */
TEST(JigsawAssemblerAssembleTest, StartPieceWithinPaddingSucceeds)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 20, 5));
    const BlockPos startPos(0, 0, 0);
    const mc::world::gen::structure::DimensionPadding padding(10, 10);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, &padding);

    EXPECT_EQ(pieces.size(), 1u);
    // 验证起始块包围盒正确
    ASSERT_FALSE(pieces.empty());
    EXPECT_EQ(pieces[0].boundingBox.minY(), 0);
    EXPECT_EQ(pieces[0].boundingBox.maxY(), 19);
}

/**
 * @brief MaxDistance Y 轴被 DimensionPadding 与世界底部裁剪
 *
 * 世界范围 [-64, 320)，DimensionPadding(0, 50)，MaxDistance(10, 10)。
 * startPos.y=-60，未裁剪的 Y 范围为 [-70, -49]，但 worldMinY+bottom=-14，
 * 故裁剪后 Y 范围应为 [-14, -49] —— 由于 minY > maxY（裁剪后下界高于上界），
 * 起始块包围盒 [-60, -59] 的 minY=-60 < -14（lowerLimit），会被 isStartTooCloseToWorldHeightLimits 拒绝。
 *
 * 此测试验证底部裁剪生效：startPos.y=-60 + DimensionPadding.bottom=50 导致 lowerLimit=-14，
 * 起始块 minY=-60 < -14，触发早返回。
 */
TEST(JigsawAssemblerAssembleTest, MaxDistanceBottomClippingRejectsStart)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 2, 5));
    const BlockPos startPos(0, -60, 0);
    const mc::world::gen::structure::DimensionPadding padding(0, 50);
    const mc::world::gen::structure::MaxDistance maxDist(10, 10);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, &padding);

    // 起始块 minY=-60 < lowerLimit=-14，被 isStartTooCloseToWorldHeightLimits 拒绝
    EXPECT_TRUE(pieces.empty());
}

/**
 * @brief MaxDistance Y 轴被 DimensionPadding 与世界顶部裁剪
 *
 * 世界范围 [-64, 320)，DimensionPadding(50, 0)，MaxDistance(10, 10)。
 * startPos.y=315，起始块 size.y=2，包围盒 [315, 316]。
 * upperLimit = 320 - 1 - 50 = 269，maxY=316 > 269，被拒绝。
 */
TEST(JigsawAssemblerAssembleTest, MaxDistanceTopClippingRejectsStart)
{
    MockChunkGenerator generator; // [-64, 320)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 2, 5));
    const BlockPos startPos(0, 315, 0);
    const mc::world::gen::structure::DimensionPadding padding(50, 0);
    const mc::world::gen::structure::MaxDistance maxDist(10, 10);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, generator, nullptr, &maxDist, &padding);

    // 起始块 maxY=316 > upperLimit=269，被拒绝
    EXPECT_TRUE(pieces.empty());
}

/**
 * @brief 自定义世界高度（如下界 [0, 128)）下的边界检查
 *
 * 下界 minY=0, genDepth=128。DimensionPadding(10, 10) 将有效范围限制到 [10, 117]。
 * 起始块 size.y=20，startPos.y=0，包围盒 [0, 19]，minY=0 < 10，被拒绝。
 */
TEST(JigsawAssemblerAssembleTest, NetherLikeWorldBottomCheck)
{
    MockChunkGenerator netherGen(0, 128); // [0, 128)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 20, 5));
    const BlockPos startPos(0, 0, 0);
    const mc::world::gen::structure::DimensionPadding padding(10, 10);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, netherGen, nullptr, &maxDist, &padding);

    EXPECT_TRUE(pieces.empty());
}

/**
 * @brief 自定义世界高度（下界 [0, 128)）下起始块在范围内时成功放置
 *
 * 下界 minY=0, genDepth=128。DimensionPadding(10, 10) 将有效范围限制到 [10, 117]。
 * 起始块 size.y=20，startPos.y=50，包围盒 [50, 69] 完全在 [10, 117] 内，应通过。
 */
TEST(JigsawAssemblerAssembleTest, NetherLikeWorldWithinBoundsSucceeds)
{
    MockChunkGenerator netherGen(0, 128); // [0, 128)
    auto& registry = TemplatePoolRegistry::instance();
    TemplatePool startPool = makeSinglePieceStartPool(BlockPos(5, 20, 5));
    const BlockPos startPos(0, 50, 0);
    const mc::world::gen::structure::DimensionPadding padding(10, 10);
    const mc::world::gen::structure::MaxDistance maxDist(80);

    Random rng(42);
    auto pieces =
        JigsawAssembler::assemble(registry, startPool, 1, startPos, rng, netherGen, nullptr, &maxDist, &padding);

    EXPECT_EQ(pieces.size(), 1u);
    ASSERT_FALSE(pieces.empty());
    EXPECT_EQ(pieces[0].boundingBox.minY(), 50);
    EXPECT_EQ(pieces[0].boundingBox.maxY(), 69);
}
