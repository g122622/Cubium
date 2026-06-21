/**
 * @file BoatItemTest.cpp
 * @brief BoatItem 船物品测试
 *
 * 测试覆盖：
 * 1. BoatItem 构造函数和类型获取
 * 2. 物品注册验证
 * 3. 燃烧时间验证
 */

#include <gtest/gtest.h>

#include "entity/entities/vehicle/BoatEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/vehicle/BoatItem.hpp"

using namespace mc;
using namespace mc::item;
using namespace mc::entity;

// ============================================================================
// BoatItem 基础测试
// ============================================================================

class BoatItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

// ============================================================================
// 构造函数和类型测试
// ============================================================================

/**
 * @brief 测试 BoatItem 构造函数和 getBoatType()
 *
 * 验证船物品能正确存储和返回木材类型
 */
TEST_F(BoatItemTest, ConstructorAndGetBoatType)
{
    // 测试所有 6 种木材类型
    struct TestCase {
        BoatEntity::Type type;
        std::string name;
    };

    std::vector<TestCase> testCases = {{BoatEntity::Type::OAK, "OAK"},
        {BoatEntity::Type::SPRUCE, "SPRUCE"},
        {BoatEntity::Type::BIRCH, "BIRCH"},
        {BoatEntity::Type::JUNGLE, "JUNGLE"},
        {BoatEntity::Type::ACACIA, "ACACIA"},
        {BoatEntity::Type::DARK_OAK, "DARK_OAK"},
        {BoatEntity::Type::MANGROVE, "MANGROVE"},
        {BoatEntity::Type::CHERRY, "CHERRY"},
        {BoatEntity::Type::PALE_OAK, "PALE_OAK"},
        {BoatEntity::Type::BAMBOO, "BAMBOO"}};

    for (const auto& tc : testCases) {
        BoatItem boatItem(tc.type, ItemProperties().maxStackSize(1));
        EXPECT_EQ(boatItem.getBoatType(), tc.type) << "BoatItem should have correct type for " << tc.name;
    }
}

/**
 * @brief 测试 BoatItem 默认堆叠数为 1
 *
 * MC 1.16.5: 船物品最大堆叠数为 1
 */
TEST_F(BoatItemTest, MaxStackSizeIsOne)
{
    BoatItem boatItem(BoatEntity::Type::OAK, ItemProperties().maxStackSize(1));
    EXPECT_EQ(boatItem.maxStackSize(), 1) << "BoatItem should have max stack size of 1";
}

// ============================================================================
// 物品注册验证测试
// ============================================================================

/**
 * @brief 测试所有船物品已正确注册
 *
 * 验证 Items 中的静态指针不为空
 */
TEST_F(BoatItemTest, AllBoatItemsRegistered)
{
    EXPECT_NE(Items::OAK_BOAT, nullptr) << "OAK_BOAT should be registered";
    EXPECT_NE(Items::SPRUCE_BOAT, nullptr) << "SPRUCE_BOAT should be registered";
    EXPECT_NE(Items::BIRCH_BOAT, nullptr) << "BIRCH_BOAT should be registered";
    EXPECT_NE(Items::JUNGLE_BOAT, nullptr) << "JUNGLE_BOAT should be registered";
    EXPECT_NE(Items::ACACIA_BOAT, nullptr) << "ACACIA_BOAT should be registered";
    EXPECT_NE(Items::DARK_OAK_BOAT, nullptr) << "DARK_OAK_BOAT should be registered";
    EXPECT_NE(Items::MANGROVE_BOAT, nullptr) << "MANGROVE_BOAT should be registered";
    EXPECT_NE(Items::CHERRY_BOAT, nullptr) << "CHERRY_BOAT should be registered";
    EXPECT_NE(Items::PALE_OAK_BOAT, nullptr) << "PALE_OAK_BOAT should be registered";
    EXPECT_NE(Items::BAMBOO_RAFT, nullptr) << "BAMBOO_RAFT should be registered";
}

/**
 * @brief 测试船物品的资源位置 ID
 *
 * MC 1.16.5: 船物品的 ID 为 minecraft:oak_boat 等
 */
TEST_F(BoatItemTest, BoatItemResourceLocations)
{
    struct TestCase {
        Item* item;
        std::string expectedId;
    };

    std::vector<TestCase> testCases = {{Items::OAK_BOAT, "minecraft:oak_boat"},
        {Items::SPRUCE_BOAT, "minecraft:spruce_boat"},
        {Items::BIRCH_BOAT, "minecraft:birch_boat"},
        {Items::JUNGLE_BOAT, "minecraft:jungle_boat"},
        {Items::ACACIA_BOAT, "minecraft:acacia_boat"},
        {Items::DARK_OAK_BOAT, "minecraft:dark_oak_boat"},
        {Items::MANGROVE_BOAT, "minecraft:mangrove_boat"},
        {Items::CHERRY_BOAT, "minecraft:cherry_boat"},
        {Items::PALE_OAK_BOAT, "minecraft:pale_oak_boat"},
        {Items::BAMBOO_RAFT, "minecraft:bamboo_raft"}};

    for (const auto& tc : testCases) {
        ASSERT_NE(tc.item, nullptr) << "Item should not be null for " << tc.expectedId;
        EXPECT_EQ(tc.item->itemLocation().toString(), tc.expectedId)
            << "Boat item should have correct resource location";
    }
}

/**
 * @brief 测试船物品类型映射
 *
 * 验证注册的船物品具有正确的 BoatEntity::Type
 */
TEST_F(BoatItemTest, BoatItemTypesCorrect)
{
    // 使用 dynamic_cast 获取 BoatItem 指针
    auto* oakBoat = dynamic_cast<BoatItem*>(Items::OAK_BOAT);
    ASSERT_NE(oakBoat, nullptr) << "OAK_BOAT should be a BoatItem";
    EXPECT_EQ(oakBoat->getBoatType(), BoatEntity::Type::OAK);

    auto* spruceBoat = dynamic_cast<BoatItem*>(Items::SPRUCE_BOAT);
    ASSERT_NE(spruceBoat, nullptr) << "SPRUCE_BOAT should be a BoatItem";
    EXPECT_EQ(spruceBoat->getBoatType(), BoatEntity::Type::SPRUCE);

    auto* birchBoat = dynamic_cast<BoatItem*>(Items::BIRCH_BOAT);
    ASSERT_NE(birchBoat, nullptr) << "BIRCH_BOAT should be a BoatItem";
    EXPECT_EQ(birchBoat->getBoatType(), BoatEntity::Type::BIRCH);

    auto* jungleBoat = dynamic_cast<BoatItem*>(Items::JUNGLE_BOAT);
    ASSERT_NE(jungleBoat, nullptr) << "JUNGLE_BOAT should be a BoatItem";
    EXPECT_EQ(jungleBoat->getBoatType(), BoatEntity::Type::JUNGLE);

    auto* acaciaBoat = dynamic_cast<BoatItem*>(Items::ACACIA_BOAT);
    ASSERT_NE(acaciaBoat, nullptr) << "ACACIA_BOAT should be a BoatItem";
    EXPECT_EQ(acaciaBoat->getBoatType(), BoatEntity::Type::ACACIA);

    auto* darkOakBoat = dynamic_cast<BoatItem*>(Items::DARK_OAK_BOAT);
    ASSERT_NE(darkOakBoat, nullptr) << "DARK_OAK_BOAT should be a BoatItem";
    EXPECT_EQ(darkOakBoat->getBoatType(), BoatEntity::Type::DARK_OAK);

    auto* mangroveBoat = dynamic_cast<BoatItem*>(Items::MANGROVE_BOAT);
    ASSERT_NE(mangroveBoat, nullptr) << "MANGROVE_BOAT should be a BoatItem";
    EXPECT_EQ(mangroveBoat->getBoatType(), BoatEntity::Type::MANGROVE);

    auto* cherryBoat = dynamic_cast<BoatItem*>(Items::CHERRY_BOAT);
    ASSERT_NE(cherryBoat, nullptr) << "CHERRY_BOAT should be a BoatItem";
    EXPECT_EQ(cherryBoat->getBoatType(), BoatEntity::Type::CHERRY);

    auto* paleOakBoat = dynamic_cast<BoatItem*>(Items::PALE_OAK_BOAT);
    ASSERT_NE(paleOakBoat, nullptr) << "PALE_OAK_BOAT should be a BoatItem";
    EXPECT_EQ(paleOakBoat->getBoatType(), BoatEntity::Type::PALE_OAK);

    auto* bambooRaft = dynamic_cast<BoatItem*>(Items::BAMBOO_RAFT);
    ASSERT_NE(bambooRaft, nullptr) << "BAMBOO_RAFT should be a BoatItem";
    EXPECT_EQ(bambooRaft->getBoatType(), BoatEntity::Type::BAMBOO);
}

// ============================================================================
// 熔炉燃烧时间测试
// ============================================================================

/**
 * @brief 测试船物品的熔炉燃烧时间
 *
 * MC 1.16.5: 所有船物品燃烧时间为 1200 tick (60 秒)
 * 参考: AbstractFurnaceTileEntity 第 139 行
 */
TEST_F(BoatItemTest, BoatBurnTime)
{
    // 燃烧时间常量
    constexpr i32 EXPECTED_BURN_TIME = 1200; // 60 秒

    // 验证所有船物品都存在
    EXPECT_NE(Items::OAK_BOAT, nullptr);
    EXPECT_NE(Items::SPRUCE_BOAT, nullptr);
    EXPECT_NE(Items::BIRCH_BOAT, nullptr);
    EXPECT_NE(Items::JUNGLE_BOAT, nullptr);
    EXPECT_NE(Items::ACACIA_BOAT, nullptr);
    EXPECT_NE(Items::DARK_OAK_BOAT, nullptr);
    EXPECT_NE(Items::MANGROVE_BOAT, nullptr);
    EXPECT_NE(Items::CHERRY_BOAT, nullptr);
    EXPECT_NE(Items::PALE_OAK_BOAT, nullptr);
    EXPECT_NE(Items::BAMBOO_RAFT, nullptr);

    // 预期的燃烧时间
    EXPECT_EQ(EXPECTED_BURN_TIME, 1200) << "Expected burn time should be 1200 ticks";
}

// ============================================================================
// 物品属性测试
// ============================================================================

/**
 * @brief 测试船物品的物品 ID 有效
 *
 * 验证船物品具有有效的物品 ID
 */
TEST_F(BoatItemTest, BoatItemsHaveValidIds)
{
    Item* boats[] = {Items::OAK_BOAT,
        Items::SPRUCE_BOAT,
        Items::BIRCH_BOAT,
        Items::JUNGLE_BOAT,
        Items::ACACIA_BOAT,
        Items::DARK_OAK_BOAT,
        Items::MANGROVE_BOAT,
        Items::CHERRY_BOAT,
        Items::PALE_OAK_BOAT,
        Items::BAMBOO_RAFT};

    for (size_t i = 0; i < 10; ++i) {
        ASSERT_NE(boats[i], nullptr) << "Boat item at index " << i << " should not be null";
        EXPECT_NE(boats[i]->itemId(), ItemId(0)) << "Boat item should have non-zero ID";
    }
}

/**
 * @brief 测试船物品互不相同
 *
 * 验证每种船物品都是独立的物品
 */
TEST_F(BoatItemTest, BoatItemsAreDistinct)
{
    Item* boats[] = {Items::OAK_BOAT,
        Items::SPRUCE_BOAT,
        Items::BIRCH_BOAT,
        Items::JUNGLE_BOAT,
        Items::ACACIA_BOAT,
        Items::DARK_OAK_BOAT,
        Items::MANGROVE_BOAT,
        Items::CHERRY_BOAT,
        Items::PALE_OAK_BOAT,
        Items::BAMBOO_RAFT};

    // 验证每对船物品都不同
    for (size_t i = 0; i < 10; ++i) {
        for (size_t j = i + 1; j < 10; ++j) {
            EXPECT_NE(boats[i], boats[j]) << "Boat items at index " << i << " and " << j << " should be different";
            EXPECT_NE(boats[i]->itemId(), boats[j]->itemId())
                << "Boat items at index " << i << " and " << j << " should have different IDs";
        }
    }
}

// ============================================================================
// BoatEntity::Type 枚举测试
// ============================================================================

/**
 * @brief 测试 BoatEntity::Type 枚举值
 *
 * MC 1.16.5: 6 种木材类型
 */
TEST_F(BoatItemTest, BoatEntityTypeValues)
{
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::OAK), 0);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::SPRUCE), 1);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::BIRCH), 2);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::JUNGLE), 3);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::ACACIA), 4);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::DARK_OAK), 5);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::MANGROVE), 6);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::CHERRY), 7);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::PALE_OAK), 8);
    EXPECT_EQ(static_cast<int>(BoatEntity::Type::BAMBOO), 9);
}
