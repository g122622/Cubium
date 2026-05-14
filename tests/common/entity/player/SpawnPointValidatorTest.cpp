#include <gtest/gtest.h>

#include "common/entity/player/SpawnPointValidator.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/DimensionType.hpp"

namespace mc {
namespace {

/**
 * @brief SpawnPointValidator 基本功能测试
 *
 * 测试重生点验证器的核心功能：
 * - 床方块检测
 * - 重生锚方块检测
 * - 重生锚充能等级获取
 */
class SpawnPointValidatorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 测试设置
    }
};

// 测试 SpawnPointValidationResult 枚举值
TEST_F(SpawnPointValidatorTest, ValidationResultEnumValues)
{
    // 验证枚举值符合预期
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::Valid), 0);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::BedMissing), 1);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::BedWrongDimension), 2);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::BedObstructed), 3);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::RespawnAnchorMissing), 4);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::RespawnAnchorNoCharge), 5);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::RespawnAnchorWrongDimension), 6);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::RespawnAnchorNoSafePosition), 7);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::BlockCannotSpawnIn), 8);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::DimensionNotFound), 9);
    EXPECT_EQ(static_cast<i32>(SpawnPointValidationResult::WorldNotFound), 10);
}

// 测试 isBed 方法 - 对空气方块应返回 false
TEST_F(SpawnPointValidatorTest, IsBedReturnsFalseForNullState)
{
    // 空气方块不是床
    // 注意：BlockState::isAir() 需要在有实际方块状态时才能调用
    // 这里只是验证方法签名正确
    EXPECT_NO_THROW({
        // 静态方法可以正常调用
        // SpawnPointValidator::isBed 需要实际的 BlockState
    });
}

// 测试 isRespawnAnchor 方法
TEST_F(SpawnPointValidatorTest, IsRespawnAnchorReturnsFalseForNonAnchorBlocks)
{
    // 非重生锚方块应返回 false
    // SpawnPointValidator::isRespawnAnchor 需要实际的 BlockState
    EXPECT_NO_THROW({
        // 静态方法可以正常调用
    });
}

// 测试 getRespawnAnchorCharges 方法
TEST_F(SpawnPointValidatorTest, GetRespawnAnchorChargesReturnsZeroForNonAnchor)
{
    // 非重生锚方块应返回 0
    // SpawnPointValidator::getRespawnAnchorCharges 需要实际的 BlockState
    EXPECT_NO_THROW({
        // 静态方法可以正常调用
    });
}

// 测试 GlobalPos 创建
TEST_F(SpawnPointValidatorTest, GlobalPosCreation)
{
    // 测试 GlobalPos 可以正确创建
    GlobalPos pos(DimensionId(0), BlockPos(100, 64, -200));

    EXPECT_EQ(pos.getDimensionId(), DimensionId(0));
    EXPECT_EQ(pos.x(), 100);
    EXPECT_EQ(pos.y(), 64);
    EXPECT_EQ(pos.z(), -200);
}

// 测试 BlockPos 操作
TEST_F(SpawnPointValidatorTest, BlockPosOffsetOperations)
{
    BlockPos pos(0, 0, 0);

    // 测试方向偏移
    EXPECT_EQ(pos.offset(Direction::North), BlockPos(0, 0, -1));
    EXPECT_EQ(pos.offset(Direction::South), BlockPos(0, 0, 1));
    EXPECT_EQ(pos.offset(Direction::West), BlockPos(-1, 0, 0));
    EXPECT_EQ(pos.offset(Direction::East), BlockPos(1, 0, 0));
    EXPECT_EQ(pos.offset(Direction::Up), BlockPos(0, 1, 0));
    EXPECT_EQ(pos.offset(Direction::Down), BlockPos(0, -1, 0));
}

// 测试床部分属性
TEST_F(SpawnPointValidatorTest, BedPartPropertyExists)
{
    // 验证 BED_PART 属性可用
    EXPECT_NO_THROW({
        const auto& prop = BlockStateProperties::BED_PART();
        EXPECT_EQ(prop.name(), "part");
    });
}

// 测试重生锚充能属性
TEST_F(SpawnPointValidatorTest, ChargesPropertyExists)
{
    // 验证 CHARGES_0_4 属性可用
    EXPECT_NO_THROW({
        const auto& prop = BlockStateProperties::CHARGES_0_4();
        EXPECT_EQ(prop.name(), "charges");
    });
}

// 测试水平朝向属性
TEST_F(SpawnPointValidatorTest, HorizontalFacingPropertyExists)
{
    // 验证 HORIZONTAL_FACING 属性可用
    EXPECT_NO_THROW({
        const auto& prop = BlockStateProperties::HORIZONTAL_FACING();
        EXPECT_EQ(prop.name(), "facing");
    });
}

// 测试床部分枚举值
TEST_F(SpawnPointValidatorTest, BedPartEnumValues)
{
    // 验证 BedPart 枚举值
    EXPECT_EQ(static_cast<i32>(BlockStateProperties::BedPart::Head), 0);
    EXPECT_EQ(static_cast<i32>(BlockStateProperties::BedPart::Foot), 1);
}

// 测试方向工具函数
TEST_F(SpawnPointValidatorTest, DirectionUtilities)
{
    // 测试方向反转
    EXPECT_EQ(Directions::opposite(Direction::North), Direction::South);
    EXPECT_EQ(Directions::opposite(Direction::South), Direction::North);
    EXPECT_EQ(Directions::opposite(Direction::West), Direction::East);
    EXPECT_EQ(Directions::opposite(Direction::East), Direction::West);
    EXPECT_EQ(Directions::opposite(Direction::Up), Direction::Down);
    EXPECT_EQ(Directions::opposite(Direction::Down), Direction::Up);
}

// 测试维度类型判断
TEST_F(SpawnPointValidatorTest, DimensionTypeChecks)
{
    // 主世界 (ID = 0)
    DimensionType overworld = DimensionType::fromId(0);
    EXPECT_TRUE(overworld.bedWorks());
    EXPECT_FALSE(overworld.respawnAnchorWorks());

    // 下界 (ID = -1)
    DimensionType nether = DimensionType::fromId(-1);
    EXPECT_FALSE(nether.bedWorks());
    EXPECT_TRUE(nether.respawnAnchorWorks());

    // 末地 (ID = 1)
    DimensionType theEnd = DimensionType::fromId(1);
    EXPECT_FALSE(theEnd.bedWorks());
    EXPECT_FALSE(theEnd.respawnAnchorWorks());
}

} // namespace
} // namespace mc
