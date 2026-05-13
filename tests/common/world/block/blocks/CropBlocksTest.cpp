#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/WheatBlock.hpp"
#include "common/world/block/blocks/agricultural/CarrotBlock.hpp"
#include "common/world/block/blocks/agricultural/PotatoBlock.hpp"
#include "common/world/block/blocks/agricultural/BeetrootBlock.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 农作物方块测试夹具
 *
 * 初始化必要的注册表，确保 Items 和 VanillaBlocks 可用
 */
class CropBlocksTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();
        // 初始化物品注册表
        Items::initialize();
    }
};

// ============================================================================
// WheatBlock 测试
// ============================================================================

TEST_F(CropBlocksTest, WheatBlock_GetCropItem_ReturnsWheatItemId) {
    // 创建小麦方块
    WheatBlock wheatBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    // 获取作物物品ID
    u32 cropItemId = wheatBlock.getCropItem();

    // 验证返回的是 WHEAT 的物品ID
    ASSERT_NE(Items::WHEAT, nullptr) << "Items::WHEAT should be initialized";
    EXPECT_EQ(cropItemId, Items::WHEAT->itemId())
        << "WheatBlock::getCropItem() should return Items::WHEAT->itemId()";
}

TEST_F(CropBlocksTest, WheatBlock_GetSeedItem_ReturnsWheatSeedsItemId) {
    WheatBlock wheatBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 seedItemId = wheatBlock.getSeedItem();

    ASSERT_NE(Items::WHEAT_SEEDS, nullptr) << "Items::WHEAT_SEEDS should be initialized";
    EXPECT_EQ(seedItemId, Items::WHEAT_SEEDS->itemId())
        << "WheatBlock::getSeedItem() should return Items::WHEAT_SEEDS->itemId()";
}

TEST_F(CropBlocksTest, WheatBlock_CropAndSeedItemsAreDifferent) {
    WheatBlock wheatBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = wheatBlock.getCropItem();
    u32 seedItemId = wheatBlock.getSeedItem();

    // 小麦和种子是不同的物品
    EXPECT_NE(cropItemId, seedItemId)
        << "Wheat crop and seeds should be different items";
}

// ============================================================================
// CarrotBlock 测试
// ============================================================================

TEST_F(CropBlocksTest, CarrotBlock_GetCropItem_ReturnsCarrotItemId) {
    CarrotBlock carrotBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = carrotBlock.getCropItem();

    ASSERT_NE(Items::CARROT, nullptr) << "Items::CARROT should be initialized";
    EXPECT_EQ(cropItemId, Items::CARROT->itemId())
        << "CarrotBlock::getCropItem() should return Items::CARROT->itemId()";
}

TEST_F(CropBlocksTest, CarrotBlock_GetSeedItem_ReturnsCarrotItemId) {
    CarrotBlock carrotBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 seedItemId = carrotBlock.getSeedItem();

    ASSERT_NE(Items::CARROT, nullptr) << "Items::CARROT should be initialized";
    EXPECT_EQ(seedItemId, Items::CARROT->itemId())
        << "CarrotBlock::getSeedItem() should return Items::CARROT->itemId()";
}

TEST_F(CropBlocksTest, CarrotBlock_CropAndSeedItemsAreSame) {
    CarrotBlock carrotBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = carrotBlock.getCropItem();
    u32 seedItemId = carrotBlock.getSeedItem();

    // 胡萝卜的作物和种子是同一个物品
    EXPECT_EQ(cropItemId, seedItemId)
        << "Carrot crop and seed should be the same item (CARROT)";
}

// ============================================================================
// PotatoBlock 测试
// ============================================================================

TEST_F(CropBlocksTest, PotatoBlock_GetCropItem_ReturnsPotatoItemId) {
    PotatoBlock potatoBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = potatoBlock.getCropItem();

    ASSERT_NE(Items::POTATO, nullptr) << "Items::POTATO should be initialized";
    EXPECT_EQ(cropItemId, Items::POTATO->itemId())
        << "PotatoBlock::getCropItem() should return Items::POTATO->itemId()";
}

TEST_F(CropBlocksTest, PotatoBlock_GetSeedItem_ReturnsPotatoItemId) {
    PotatoBlock potatoBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 seedItemId = potatoBlock.getSeedItem();

    ASSERT_NE(Items::POTATO, nullptr) << "Items::POTATO should be initialized";
    EXPECT_EQ(seedItemId, Items::POTATO->itemId())
        << "PotatoBlock::getSeedItem() should return Items::POTATO->itemId()";
}

TEST_F(CropBlocksTest, PotatoBlock_CropAndSeedItemsAreSame) {
    PotatoBlock potatoBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = potatoBlock.getCropItem();
    u32 seedItemId = potatoBlock.getSeedItem();

    // 马铃薯的作物和种子是同一个物品
    EXPECT_EQ(cropItemId, seedItemId)
        << "Potato crop and seed should be the same item (POTATO)";
}

// ============================================================================
// BeetrootBlock 测试
// ============================================================================

TEST_F(CropBlocksTest, BeetrootBlock_GetCropItem_ReturnsBeetrootItemId) {
    BeetrootBlock beetrootBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = beetrootBlock.getCropItem();

    ASSERT_NE(Items::BEETROOT, nullptr) << "Items::BEETROOT should be initialized";
    EXPECT_EQ(cropItemId, Items::BEETROOT->itemId())
        << "BeetrootBlock::getCropItem() should return Items::BEETROOT->itemId()";
}

TEST_F(CropBlocksTest, BeetrootBlock_GetSeedItem_ReturnsBeetrootSeedsItemId) {
    BeetrootBlock beetrootBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 seedItemId = beetrootBlock.getSeedItem();

    ASSERT_NE(Items::BEETROOT_SEEDS, nullptr) << "Items::BEETROOT_SEEDS should be initialized";
    EXPECT_EQ(seedItemId, Items::BEETROOT_SEEDS->itemId())
        << "BeetrootBlock::getSeedItem() should return Items::BEETROOT_SEEDS->itemId()";
}

TEST_F(CropBlocksTest, BeetrootBlock_CropAndSeedItemsAreDifferent) {
    BeetrootBlock beetrootBlock(BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    u32 cropItemId = beetrootBlock.getCropItem();
    u32 seedItemId = beetrootBlock.getSeedItem();

    // 甜菜根和种子是不同的物品
    EXPECT_NE(cropItemId, seedItemId)
        << "Beetroot crop and seeds should be different items";
}

// ============================================================================
// Item ID 有效性测试
// ============================================================================

TEST_F(CropBlocksTest, AllCropItemsHaveValidNonZeroItemIds) {
    // 验证所有作物物品都有有效的非零 ID
    ASSERT_NE(Items::WHEAT, nullptr);
    EXPECT_GT(Items::WHEAT->itemId(), 0u) << "WHEAT should have non-zero item ID";

    ASSERT_NE(Items::WHEAT_SEEDS, nullptr);
    EXPECT_GT(Items::WHEAT_SEEDS->itemId(), 0u) << "WHEAT_SEEDS should have non-zero item ID";

    ASSERT_NE(Items::CARROT, nullptr);
    EXPECT_GT(Items::CARROT->itemId(), 0u) << "CARROT should have non-zero item ID";

    ASSERT_NE(Items::POTATO, nullptr);
    EXPECT_GT(Items::POTATO->itemId(), 0u) << "POTATO should have non-zero item ID";

    ASSERT_NE(Items::BEETROOT, nullptr);
    EXPECT_GT(Items::BEETROOT->itemId(), 0u) << "BEETROOT should have non-zero item ID";

    ASSERT_NE(Items::BEETROOT_SEEDS, nullptr);
    EXPECT_GT(Items::BEETROOT_SEEDS->itemId(), 0u) << "BEETROOT_SEEDS should have non-zero item ID";
}

TEST_F(CropBlocksTest, AllCropItemsHaveUniqueItemIds) {
    // 收集所有物品 ID
    std::set<u32> itemIds;
    itemIds.insert(Items::WHEAT->itemId());
    itemIds.insert(Items::WHEAT_SEEDS->itemId());
    itemIds.insert(Items::CARROT->itemId());
    itemIds.insert(Items::POTATO->itemId());
    itemIds.insert(Items::BEETROOT->itemId());
    itemIds.insert(Items::BEETROOT_SEEDS->itemId());

    // 所有物品 ID 应该是唯一的
    EXPECT_EQ(itemIds.size(), 6u) << "All 6 crop items should have unique item IDs";
}

} // namespace
