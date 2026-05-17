/**
 * @file BoatEntityDropItemTest.cpp
 * @brief 测试 BoatEntity::getBoatItem() 和 dropItem() 方法
 *
 * 测试覆盖：
 * 1. BoatEntity::getBoatItem() - 根据船类型返回对应物品
 * 2. BoatEntity::dropItem() - 正确掉落船物品
 *
 * MC 1.16.5 参考：
 * - BoatEntity.getItemBoat() 行 202-218
 * - BoatEntity.attackEntityFrom() 行 149-170
 */

#include <gtest/gtest.h>
#include "entity/entities/vehicle/BoatEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// BoatEntity::getBoatItem() 测试
// ============================================================================

class BoatEntityGetBoatItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 Items 已初始化
        Items::initialize();
    }
};

/**
 * @brief 测试 OAK 类型船返回橡木船物品
 *
 * MC 1.16.5: BoatEntity.Type.OAK -> Items.OAK_BOAT
 */
TEST_F(BoatEntityGetBoatItemTest, OakBoat_ReturnsOakBoatItem)
{
    BoatEntity boat(BoatEntity::Type::OAK);

    const Item* boatItem = boat.getBoatItem();

    ASSERT_NE(boatItem, nullptr) << "getBoatItem() should not return nullptr for OAK type";
    EXPECT_EQ(boatItem, Items::OAK_BOAT) << "OAK boat should return Items::OAK_BOAT";
}

/**
 * @brief 测试 SPRUCE 类型船返回云杉木船物品
 *
 * MC 1.16.5: BoatEntity.Type.SPRUCE -> Items.SPRUCE_BOAT
 */
TEST_F(BoatEntityGetBoatItemTest, SpruceBoat_ReturnsSpruceBoatItem)
{
    BoatEntity boat(BoatEntity::Type::SPRUCE);

    const Item* boatItem = boat.getBoatItem();

    ASSERT_NE(boatItem, nullptr) << "getBoatItem() should not return nullptr for SPRUCE type";
    EXPECT_EQ(boatItem, Items::SPRUCE_BOAT) << "SPRUCE boat should return Items::SPRUCE_BOAT";
}

/**
 * @brief 测试 BIRCH 类型船返回白桦木船物品
 *
 * MC 1.16.5: BoatEntity.Type.BIRCH -> Items.BIRCH_BOAT
 */
TEST_F(BoatEntityGetBoatItemTest, BirchBoat_ReturnsBirchBoatItem)
{
    BoatEntity boat(BoatEntity::Type::BIRCH);

    const Item* boatItem = boat.getBoatItem();

    ASSERT_NE(boatItem, nullptr) << "getBoatItem() should not return nullptr for BIRCH type";
    EXPECT_EQ(boatItem, Items::BIRCH_BOAT) << "BIRCH boat should return Items::BIRCH_BOAT";
}

/**
 * @brief 测试 JUNGLE 类型船返回丛林木船物品
 *
 * MC 1.16.5: BoatEntity.Type.JUNGLE -> Items.JUNGLE_BOAT
 */
TEST_F(BoatEntityGetBoatItemTest, JungleBoat_ReturnsJungleBoatItem)
{
    BoatEntity boat(BoatEntity::Type::JUNGLE);

    const Item* boatItem = boat.getBoatItem();

    ASSERT_NE(boatItem, nullptr) << "getBoatItem() should not return nullptr for JUNGLE type";
    EXPECT_EQ(boatItem, Items::JUNGLE_BOAT) << "JUNGLE boat should return Items::JUNGLE_BOAT";
}

/**
 * @brief 测试 ACACIA 类型船返回金合欢木船物品
 *
 * MC 1.16.5: BoatEntity.Type.ACACIA -> Items.ACACIA_BOAT
 */
TEST_F(BoatEntityGetBoatItemTest, AcaciaBoat_ReturnsAcaciaBoatItem)
{
    BoatEntity boat(BoatEntity::Type::ACACIA);

    const Item* boatItem = boat.getBoatItem();

    ASSERT_NE(boatItem, nullptr) << "getBoatItem() should not return nullptr for ACACIA type";
    EXPECT_EQ(boatItem, Items::ACACIA_BOAT) << "ACACIA boat should return Items::ACACIA_BOAT";
}

/**
 * @brief 测试 DARK_OAK 类型船返回深色橡木船物品
 *
 * MC 1.16.5: BoatEntity.Type.DARK_OAK -> Items.DARK_OAK_BOAT
 */
TEST_F(BoatEntityGetBoatItemTest, DarkOakBoat_ReturnsDarkOakBoatItem)
{
    BoatEntity boat(BoatEntity::Type::DARK_OAK);

    const Item* boatItem = boat.getBoatItem();

    ASSERT_NE(boatItem, nullptr) << "getBoatItem() should not return nullptr for DARK_OAK type";
    EXPECT_EQ(boatItem, Items::DARK_OAK_BOAT) << "DARK_OAK boat should return Items::DARK_OAK_BOAT";
}

/**
 * @brief 测试通过 setBoatType() 更改船类型后 getBoatItem() 返回正确的物品
 */
TEST_F(BoatEntityGetBoatItemTest, SetBoatType_UpdatesReturnedItem)
{
    BoatEntity boat(BoatEntity::Type::OAK);
    EXPECT_EQ(boat.getBoatItem(), Items::OAK_BOAT);

    boat.setBoatType(BoatEntity::Type::BIRCH);
    EXPECT_EQ(boat.getBoatItem(), Items::BIRCH_BOAT);

    boat.setBoatType(BoatEntity::Type::DARK_OAK);
    EXPECT_EQ(boat.getBoatItem(), Items::DARK_OAK_BOAT);
}

/**
 * @brief 测试所有船类型都有对应的物品
 */
TEST_F(BoatEntityGetBoatItemTest, AllTypes_HaveCorrespondingItems)
{
    struct TestCase {
        BoatEntity::Type type;
        const Item* expectedItem;
        std::string name;
    };

    std::vector<TestCase> testCases = {
        { BoatEntity::Type::OAK, Items::OAK_BOAT, "OAK" },
        { BoatEntity::Type::SPRUCE, Items::SPRUCE_BOAT, "SPRUCE" },
        { BoatEntity::Type::BIRCH, Items::BIRCH_BOAT, "BIRCH" },
        { BoatEntity::Type::JUNGLE, Items::JUNGLE_BOAT, "JUNGLE" },
        { BoatEntity::Type::ACACIA, Items::ACACIA_BOAT, "ACACIA" },
        { BoatEntity::Type::DARK_OAK, Items::DARK_OAK_BOAT, "DARK_OAK" },
    };

    for (const auto& tc : testCases) {
        BoatEntity boat(tc.type);
        const Item* item = boat.getBoatItem();

        EXPECT_NE(item, nullptr)
            << "getBoatItem() should not return nullptr for type " << tc.name;
        EXPECT_EQ(item, tc.expectedItem)
            << "Type " << tc.name << " should return correct item";
    }
}

// ============================================================================
// BoatEntity 类型测试
// ============================================================================

class BoatEntityTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 Items 已初始化
        Items::initialize();
    }
};

/**
 * @brief 测试默认构造函数使用 OAK 类型
 */
TEST_F(BoatEntityTypeTest, DefaultConstructor_CreatesOakBoat)
{
    BoatEntity boat; // 默认构造

    EXPECT_EQ(boat.getBoatType(), BoatEntity::Type::OAK);
    EXPECT_EQ(boat.getBoatItem(), Items::OAK_BOAT);
}

/**
 * @brief 测试 getBoatType() 和 setBoatType() 的基本功能
 */
TEST_F(BoatEntityTypeTest, GetSetBoatType_WorksCorrectly)
{
    BoatEntity boat(BoatEntity::Type::BIRCH);

    EXPECT_EQ(boat.getBoatType(), BoatEntity::Type::BIRCH);

    boat.setBoatType(BoatEntity::Type::JUNGLE);
    EXPECT_EQ(boat.getBoatType(), BoatEntity::Type::JUNGLE);
}

/**
 * @brief 测试船类型枚举值顺序与 MC 1.16.5 一致
 *
 * MC 1.16.5: OAK=0, SPRUCE=1, BIRCH=2, JUNGLE=3, ACACIA=4, DARK_OAK=5
 */
TEST_F(BoatEntityTypeTest, EnumValues_MatchMC1165)
{
    EXPECT_EQ(static_cast<u8>(BoatEntity::Type::OAK), 0);
    EXPECT_EQ(static_cast<u8>(BoatEntity::Type::SPRUCE), 1);
    EXPECT_EQ(static_cast<u8>(BoatEntity::Type::BIRCH), 2);
    EXPECT_EQ(static_cast<u8>(BoatEntity::Type::JUNGLE), 3);
    EXPECT_EQ(static_cast<u8>(BoatEntity::Type::ACACIA), 4);
    EXPECT_EQ(static_cast<u8>(BoatEntity::Type::DARK_OAK), 5);
}

// ============================================================================
// dropItem() 逻辑测试（无需世界环境）
// ============================================================================

/**
 * @brief dropItem() 的完整测试需要世界环境，这里测试基础逻辑
 *
 * 主要测试点：
 * 1. getBoatItem() 返回正确的物品
 * 2. 类型与物品的映射关系正确
 */
TEST_F(BoatEntityTypeTest, DropItem_GetBoatItemReturnsCorrectItem)
{
    // 创建每种类型的船，验证 getBoatItem() 返回正确的物品
    // 这是 dropItem() 的核心逻辑，实际的物品实体生成需要世界环境

    BoatEntity oakBoat(BoatEntity::Type::OAK);
    EXPECT_EQ(oakBoat.getBoatItem(), Items::OAK_BOAT);

    BoatEntity spruceBoat(BoatEntity::Type::SPRUCE);
    EXPECT_EQ(spruceBoat.getBoatItem(), Items::SPRUCE_BOAT);

    BoatEntity birchBoat(BoatEntity::Type::BIRCH);
    EXPECT_EQ(birchBoat.getBoatItem(), Items::BIRCH_BOAT);

    BoatEntity jungleBoat(BoatEntity::Type::JUNGLE);
    EXPECT_EQ(jungleBoat.getBoatItem(), Items::JUNGLE_BOAT);

    BoatEntity acaciaBoat(BoatEntity::Type::ACACIA);
    EXPECT_EQ(acaciaBoat.getBoatItem(), Items::ACACIA_BOAT);

    BoatEntity darkOakBoat(BoatEntity::Type::DARK_OAK);
    EXPECT_EQ(darkOakBoat.getBoatItem(), Items::DARK_OAK_BOAT);
}

/**
 * @brief 测试 Items 中所有船物品都已注册
 */
TEST_F(BoatEntityTypeTest, AllBoatItems_AreRegistered)
{
    EXPECT_NE(Items::OAK_BOAT, nullptr) << "Items::OAK_BOAT should be registered";
    EXPECT_NE(Items::SPRUCE_BOAT, nullptr) << "Items::SPRUCE_BOAT should be registered";
    EXPECT_NE(Items::BIRCH_BOAT, nullptr) << "Items::BIRCH_BOAT should be registered";
    EXPECT_NE(Items::JUNGLE_BOAT, nullptr) << "Items::JUNGLE_BOAT should be registered";
    EXPECT_NE(Items::ACACIA_BOAT, nullptr) << "Items::ACACIA_BOAT should be registered";
    EXPECT_NE(Items::DARK_OAK_BOAT, nullptr) << "Items::DARK_OAK_BOAT should be registered";
}

/**
 * @brief 测试船物品的唯一性
 *
 * 确保每种木材类型的船物品都是独立的
 */
TEST_F(BoatEntityTypeTest, AllBoatItems_AreUnique)
{
    std::set<const Item*> boatItems = {
        Items::OAK_BOAT,
        Items::SPRUCE_BOAT,
        Items::BIRCH_BOAT,
        Items::JUNGLE_BOAT,
        Items::ACACIA_BOAT,
        Items::DARK_OAK_BOAT,
    };

    EXPECT_EQ(boatItems.size(), 6u) << "All 6 boat items should be unique pointers";
}
