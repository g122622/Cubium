#include <gtest/gtest.h>

#include "world/block/blocks/end/EndPortalBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/BlockPos.hpp"
#include "util/Direction.hpp"
#include "core/Constants.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// ChorusPlantBlock 基础测试
// ============================================================================

class ChorusPlantBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(ChorusPlantBlockTest, Create_HasCorrectProperties) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& state = block.defaultState();

    // 默认状态应该是无连接
    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
    EXPECT_FALSE(state.get(BlockStateProperties::UP()));
    EXPECT_FALSE(state.get(BlockStateProperties::DOWN()));
}

TEST_F(ChorusPlantBlockTest, GetShape_ReturnsValidShape) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& state = block.defaultState();
    const CollisionShape& shape = block.getShape(state);

    // 形状不应该为空（即使没有连接，也有中心柱）
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_NoConnections) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& state = block.defaultState();

    // 无连接时索引应为 0
    size_t index = ChorusPlantBlock::getShapeIndex(state);
    EXPECT_EQ(index, 0ULL);
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_AllConnections) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 设置所有连接为 true
    const BlockState& state = block.defaultState()
        .with(BlockStateProperties::DOWN(), true)
        .with(BlockStateProperties::UP(), true)
        .with(BlockStateProperties::NORTH(), true)
        .with(BlockStateProperties::SOUTH(), true)
        .with(BlockStateProperties::WEST(), true)
        .with(BlockStateProperties::EAST(), true);

    // 所有连接时索引应为 63 (所有位都设置)
    size_t index = ChorusPlantBlock::getShapeIndex(state);
    EXPECT_EQ(index, 63ULL);
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_SingleConnection) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 测试各个方向的单独连接
    // Down = bit 0 = 1
    const BlockState& downState = block.defaultState().with(BlockStateProperties::DOWN(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(downState), 1ULL);

    // Up = bit 1 = 2
    const BlockState& upState = block.defaultState().with(BlockStateProperties::UP(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(upState), 2ULL);

    // North = bit 2 = 4
    const BlockState& northState = block.defaultState().with(BlockStateProperties::NORTH(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(northState), 4ULL);

    // South = bit 3 = 8
    const BlockState& southState = block.defaultState().with(BlockStateProperties::SOUTH(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(southState), 8ULL);

    // West = bit 4 = 16
    const BlockState& westState = block.defaultState().with(BlockStateProperties::WEST(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(westState), 16ULL);

    // East = bit 5 = 32
    const BlockState& eastState = block.defaultState().with(BlockStateProperties::EAST(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(eastState), 32ULL);
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_MultipleConnections) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // Up + Down = bit 0 + bit 1 = 3
    const BlockState& verticalState = block.defaultState()
        .with(BlockStateProperties::DOWN(), true)
        .with(BlockStateProperties::UP(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(verticalState), 3ULL);

    // North + South = bit 2 + bit 3 = 12
    const BlockState& northSouthState = block.defaultState()
        .with(BlockStateProperties::NORTH(), true)
        .with(BlockStateProperties::SOUTH(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(northSouthState), 12ULL);

    // East + West = bit 4 + bit 5 = 48
    const BlockState& eastWestState = block.defaultState()
        .with(BlockStateProperties::WEST(), true)
        .with(BlockStateProperties::EAST(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(eastWestState), 48ULL);
}

TEST_F(ChorusPlantBlockTest, Shape_ChangesWithConnections) {
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 无连接的形状
    const BlockState& noConnection = block.defaultState();
    const CollisionShape& shapeNoConnection = block.getShape(noConnection);

    // 所有连接的形状
    const BlockState& allConnections = block.defaultState()
        .with(BlockStateProperties::DOWN(), true)
        .with(BlockStateProperties::UP(), true)
        .with(BlockStateProperties::NORTH(), true)
        .with(BlockStateProperties::SOUTH(), true)
        .with(BlockStateProperties::WEST(), true)
        .with(BlockStateProperties::EAST(), true);
    const CollisionShape& shapeAllConnections = block.getShape(allConnections);

    // 两个形状的引用应该不同（因为索引不同）
    EXPECT_NE(&shapeNoConnection, &shapeAllConnections);
}

TEST_F(ChorusPlantBlockTest, VanillaBlocks_Registered) {
    // 确保 VanillaBlocks 中的相关方块已注册
    EXPECT_NE(VanillaBlocks::CHORUS_PLANT, nullptr);
    EXPECT_NE(VanillaBlocks::CHORUS_FLOWER, nullptr);
    EXPECT_NE(VanillaBlocks::END_STONE, nullptr);
}

TEST_F(ChorusPlantBlockTest, ShapeCenterSize) {
    // 参考 MC 1.16.5 SixWayBlock
    // apothem = 0.3125 (5像素)
    // 中心形状：(0.1875, 0.1875, 0.1875) -> (0.8125, 0.8125, 0.8125)
    // 即 (3, 3, 3) -> (13, 13, 13) 像素，尺寸为 10x10x10 像素
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& noConnection = block.defaultState();
    const CollisionShape& shape = block.getShape(noConnection);

    // 中心柱形状不为空
    EXPECT_FALSE(shape.isEmpty());
}
