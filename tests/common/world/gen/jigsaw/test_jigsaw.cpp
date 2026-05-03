/**
 * @file test_jigsaw.cpp
 * @brief Jigsaw拼图系统单元测试
 */

#include <gtest/gtest.h>
#include "common/world/gen/jigsaw/JigsawPattern.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawManager.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/resource/ResourceLocation.hpp"

using namespace mc::world::gen::jigsaw;
using namespace mc::math;
using namespace mc;

/**
 * @brief 测试 JigsawPattern 权重随机选择
 */
TEST(JigsawPatternTest, WeightedRandomSelection) {
    JigsawPattern pattern(
        ResourceLocation("minecraft", "test_pool"),
        ResourceLocation("minecraft", "empty")
    );

    // 添加权重为 1, 2, 3 的拼图块
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_a"), 1);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_b"), 2);
    pattern.addPiece(std::make_unique<SingleJigsawPiece>("piece_c"), 3);

    // 验证总数量 (1 + 2 + 3 = 6)
    EXPECT_EQ(pattern.getNumberOfPieces(), 6u);

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
 * @brief 测试 JigsawPattern::getShuffledPieces
 */
TEST(JigsawPatternTest, GetShuffledPieces) {
    JigsawPattern pattern(
        ResourceLocation("minecraft", "test_pool"),
        ResourceLocation("minecraft", "empty")
    );

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
 * @brief 测试 JigsawPattern 空池
 */
TEST(JigsawPatternTest, EmptyPool) {
    JigsawPattern pattern(
        ResourceLocation("minecraft", "empty"),
        ResourceLocation("minecraft", "empty")
    );

    EXPECT_TRUE(pattern.isEmpty());
    EXPECT_EQ(pattern.getNumberOfPieces(), 0u);

    Random rng(12345);
    EXPECT_EQ(pattern.getRandomPiece(rng), nullptr);

    auto shuffled = pattern.getShuffledPieces(rng);
    EXPECT_TRUE(shuffled.empty());
}

/**
 * @brief 测试 EmptyJigsawPiece 单例
 */
TEST(JigsawPieceTest, EmptyPieceSingleton) {
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
TEST(JigsawPieceTest, SinglePiece) {
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
TEST(JigsawPieceTest, ListPiece) {
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
TEST(JigsawJunctionTest, Equality) {
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
 * @brief 测试 JigsawPatternRegistry
 */
TEST(JigsawPatternRegistryTest, RegisterAndGet) {
    JigsawPatternRegistry& registry = JigsawPatternRegistry::instance();
    registry.clear();

    // 创建并注册模式
    auto pattern = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "test_pattern"),
        ResourceLocation("minecraft", "empty")
    );
    pattern->addPiece(std::make_unique<SingleJigsawPiece>("test_piece"), 1);

    registry.registerPattern(std::move(pattern));

    // 验证注册成功
    const JigsawPattern* retrieved = registry.getPattern(ResourceLocation("minecraft", "test_pattern"));
    EXPECT_TRUE(retrieved != nullptr);
    EXPECT_FALSE(retrieved->isEmpty());
    EXPECT_EQ(retrieved->getNumberOfPieces(), 1u);

    // 验证获取不存在的模式
    const JigsawPattern* notFound = registry.getPattern(ResourceLocation("minecraft", "nonexistent"));
    EXPECT_EQ(notFound, nullptr);

    // 清理
    registry.clear();
}

/**
 * @brief 测试 JigsawManager 坐标旋转
 */
TEST(JigsawManagerTest, CoordinateRotation) {
    // 测试 0 度旋转
    BlockPos pos(10, 5, 20);
    BlockPos rotated0 = JigsawManager::rotatePosition(pos, Rotation::None);
    EXPECT_EQ(rotated0.x, pos.x);
    EXPECT_EQ(rotated0.y, pos.y);
    EXPECT_EQ(rotated0.z, pos.z);

    // 测试 90 度旋转
    BlockPos rotated90 = JigsawManager::rotatePosition(pos, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.x, -pos.z);  // -20
    EXPECT_EQ(rotated90.y, pos.y);   // 5
    EXPECT_EQ(rotated90.z, pos.x);   // 10

    // 测试 180 度旋转
    BlockPos rotated180 = JigsawManager::rotatePosition(pos, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.x, -pos.x);  // -10
    EXPECT_EQ(rotated180.y, pos.y);   // 5
    EXPECT_EQ(rotated180.z, -pos.z);  // -20

    // 测试 270 度旋转
    BlockPos rotated270 = JigsawManager::rotatePosition(pos, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated270.x, pos.z);   // 20
    EXPECT_EQ(rotated270.y, pos.y);   // 5
    EXPECT_EQ(rotated270.z, -pos.x);  // -10
}

/**
 * @brief 测试 JigsawManager 边界框计算
 */
TEST(JigsawManagerTest, BoundingBoxCalculation) {
    SingleJigsawPiece piece("test_piece");
    piece.setSize(BlockPos(10, 8, 12));

    BlockPos pos(100, 50, 200);
    auto box = JigsawManager::calculateBoundingBox(piece, pos, Rotation::None);

    EXPECT_EQ(box.minX(), 100);
    EXPECT_EQ(box.minY(), 50);
    EXPECT_EQ(box.minZ(), 200);
    EXPECT_EQ(box.maxX(), 109);  // 100 + 10 - 1
    EXPECT_EQ(box.maxY(), 57);   // 50 + 8 - 1
    EXPECT_EQ(box.maxZ(), 211);  // 200 + 12 - 1
}

/**
 * @brief 测试 JigsawManager 边界框重叠检测
 */
TEST(JigsawManagerTest, BoundingBoxOverlap) {
    std::vector<PlacedPiece> placedPieces;

    // 创建已放置的块
    auto piece1 = std::make_unique<SingleJigsawPiece>("piece1");
    piece1->setSize(BlockPos(10, 10, 10));

    PlacedPiece placed1;
    placed1.piece = std::move(piece1);
    placed1.position = BlockPos(0, 0, 0);
    placed1.boundingBox = mc::world::gen::structure::StructureBoundingBox(0, 0, 0, 9, 9, 9);
    placedPieces.push_back(std::move(placed1));

    // 测试重叠的边界框
    mc::world::gen::structure::StructureBoundingBox overlapping(5, 5, 5, 15, 15, 15);
    EXPECT_TRUE(JigsawManager::boxesIntersect(placedPieces, overlapping));

    // 测试不重叠的边界框
    mc::world::gen::structure::StructureBoundingBox nonOverlapping(10, 10, 10, 20, 20, 20);
    EXPECT_FALSE(JigsawManager::boxesIntersect(placedPieces, nonOverlapping));

    // 测试相邻但不重叠
    mc::world::gen::structure::StructureBoundingBox adjacent(10, 0, 0, 20, 9, 9);
    EXPECT_FALSE(JigsawManager::boxesIntersect(placedPieces, adjacent));
}

/**
 * @brief 测试 JigsawJunction 的地形平滑功能
 *
 * 验证 JigsawJunction 能够正确存储地形适配信息，
 * 这些信息用于 NoiseChunkGenerator 中的地形平滑计算。
 */
TEST(JigsawJunctionTest, TerrainSmoothingData) {
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
TEST(PlacedPieceTest, JunctionStorage) {
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
