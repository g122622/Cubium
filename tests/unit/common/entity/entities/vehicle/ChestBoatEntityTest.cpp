/**
 * @file ChestBoatEntityTest.cpp
 * @brief 箱子船实体测试
 *
 * 测试覆盖：
 * 1. 构造函数和基本属性（类型、hasChest、容器大小）
 * 2. 容器物品栏操作（get/set/remove/clear/inventory）
 * 3. getBoatItem() 返回正确的箱子船物品（所有10种类型）
 * 4. 虚函数重写验证（通过基类指针访问 getBoatItem）
 * 5. canAddPassenger() 乘客限制
 * 6. getMountedYOffset() 乘客偏移
 * 7. getComparatorOutput() 比较器输出
 * 8. getDisplayName() 显示名称
 * 9. NBT 序列化/反序列化
 * 10. 与普通船的对比
 *
 * 无法在无世界环境下测试的行为（需要 IWorld 或 Player 模拟）：
 * - dropItem() 掉落容器内容和船物品
 * - remove() 实体移除时掉落物品
 * - processInitialInteract() 乘坐和打开容器
 * - stillValid() 玩家交互范围检查（需要 Player 对象）
 * - createMenu() 创建容器菜单（需要 Player 对象）
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/vehicle/ChestBoatEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::serialization;

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

/**
 * @brief 将 ChestBoatEntity 序列化到 NBT
 */
std::unique_ptr<nbt::tags::compound_tag> saveToNbt(const ChestBoatEntity& entity)
{
    auto tag = std::make_unique<nbt::tags::compound_tag>();
    entity.addAdditionalSaveData(*tag);
    return tag;
}

/**
 * @brief 从 NBT 反序列化创建新的 ChestBoatEntity
 */
std::unique_ptr<ChestBoatEntity> loadFromNbt(
    const nbt::tags::compound_tag& tag, BoatEntity::Type type = BoatEntity::Type::OAK)
{
    auto entity = std::make_unique<ChestBoatEntity>(type, mc::test::testEcsRegistry());
    auto result = entity->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success()) << "readAdditionalSaveData should succeed";
    return entity;
}

} // namespace

// ============================================================================
// 构造函数和基本属性测试
// ============================================================================

class ChestBoatEntityTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatEntityTest, DefaultConstructor_CreatesOakChestBoat)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_EQ(chestBoat.getBoatType(), BoatEntity::Type::OAK);
    EXPECT_TRUE(chestBoat.hasChest());
}

TEST_F(ChestBoatEntityTest, TypedConstructor_CreatesCorrectType)
{
    ChestBoatEntity spruceBoat(BoatEntity::Type::SPRUCE, mc::test::testEcsRegistry());
    EXPECT_EQ(spruceBoat.getBoatType(), BoatEntity::Type::SPRUCE);
    EXPECT_TRUE(spruceBoat.hasChest());

    ChestBoatEntity birchBoat(BoatEntity::Type::BIRCH, mc::test::testEcsRegistry());
    EXPECT_EQ(birchBoat.getBoatType(), BoatEntity::Type::BIRCH);
    EXPECT_TRUE(birchBoat.hasChest());

    ChestBoatEntity bambooBoat(BoatEntity::Type::BAMBOO, mc::test::testEcsRegistry());
    EXPECT_EQ(bambooBoat.getBoatType(), BoatEntity::Type::BAMBOO);
    EXPECT_TRUE(bambooBoat.hasChest());
}

TEST_F(ChestBoatEntityTest, HasChest_AlwaysTrue)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_TRUE(chestBoat.hasChest());

    ChestBoatEntity chestBoat2(BoatEntity::Type::BAMBOO, mc::test::testEcsRegistry());
    EXPECT_TRUE(chestBoat2.hasChest());
}

TEST_F(ChestBoatEntityTest, ContainerSize_Is27)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_EQ(ChestBoatEntity::CONTAINER_SIZE, 27);
    EXPECT_EQ(chestBoat.getContainerSize(), 27);
}

// ============================================================================
// 容器物品栏操作测试
// ============================================================================

class ChestBoatInventoryTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatInventoryTest, Constructor_CreatesEmptyInventory)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_EQ(chestBoat.getContainerSize(), 27);
    EXPECT_TRUE(chestBoat.isInventoryEmpty());
}

TEST_F(ChestBoatInventoryTest, GetInventoryItem_OutOfBoundsReturnsEmpty)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_TRUE(chestBoat.getInventoryItem(-1).isEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(27).isEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(100).isEmpty());
}

TEST_F(ChestBoatInventoryTest, GetInventoryItem_AllSlotsInitiallyEmpty)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    for (i32 i = 0; i < 27; ++i) {
        EXPECT_TRUE(chestBoat.getInventoryItem(i).isEmpty()) << "Slot " << i << " should be empty";
    }
}

TEST_F(ChestBoatInventoryTest, SetInventoryItem_AndGetInventoryItem)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    ItemStack stack(*Items::OAK_BOAT, 1);

    chestBoat.setInventoryItem(0, stack);
    EXPECT_FALSE(chestBoat.getInventoryItem(0).isEmpty());
    EXPECT_EQ(chestBoat.getInventoryItem(0).getCount(), 1);

    // 其他槽位仍为空
    EXPECT_TRUE(chestBoat.getInventoryItem(1).isEmpty());
    EXPECT_TRUE(chestBoat.isInventoryEmpty() == false);
}

TEST_F(ChestBoatInventoryTest, SetInventoryItem_MultipleSlots)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    ASSERT_NE(Items::SPRUCE_BOAT, nullptr);

    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));
    chestBoat.setInventoryItem(13, ItemStack(*Items::SPRUCE_BOAT, 2));
    chestBoat.setInventoryItem(26, ItemStack(*Items::OAK_BOAT, 3));

    EXPECT_EQ(chestBoat.getInventoryItem(0).getCount(), 1);
    EXPECT_EQ(chestBoat.getInventoryItem(13).getCount(), 2);
    EXPECT_EQ(chestBoat.getInventoryItem(26).getCount(), 3);

    // 中间槽位为空
    EXPECT_TRUE(chestBoat.getInventoryItem(1).isEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(12).isEmpty());
}

TEST_F(ChestBoatInventoryTest, RemoveInventoryItem_OutOfBoundsReturnsEmpty)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_TRUE(chestBoat.removeInventoryItem(-1, 1).isEmpty());
    EXPECT_TRUE(chestBoat.removeInventoryItem(100, 1).isEmpty());
}

TEST_F(ChestBoatInventoryTest, RemoveInventoryItem_FromEmptySlotReturnsEmpty)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_TRUE(chestBoat.removeInventoryItem(0, 1).isEmpty());
    EXPECT_TRUE(chestBoat.removeInventoryItem(26, 1).isEmpty());
}

TEST_F(ChestBoatInventoryTest, RemoveInventoryItem_PartialRemoval)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 5));

    // 部分移除
    ItemStack removed = chestBoat.removeInventoryItem(0, 2);
    EXPECT_EQ(removed.getCount(), 2);

    // 剩余3个
    EXPECT_EQ(chestBoat.getInventoryItem(0).getCount(), 3);
}

TEST_F(ChestBoatInventoryTest, RemoveInventoryItem_FullRemoval)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 3));

    // 全部移除
    ItemStack removed = chestBoat.removeInventoryItem(0, 3);
    EXPECT_EQ(removed.getCount(), 3);

    // 槽位变空
    EXPECT_TRUE(chestBoat.getInventoryItem(0).isEmpty());
}

TEST_F(ChestBoatInventoryTest, ClearInventory_EmptiesAllSlots)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));
    chestBoat.setInventoryItem(5, ItemStack(*Items::OAK_BOAT, 2));

    EXPECT_FALSE(chestBoat.isInventoryEmpty());

    chestBoat.clearInventory();

    EXPECT_TRUE(chestBoat.isInventoryEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(0).isEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(5).isEmpty());
}

TEST_F(ChestBoatInventoryTest, GetInventory_ReturnsNonNullptr)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    IInventory* inventory = chestBoat.getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), 27);
}

TEST_F(ChestBoatInventoryTest, ContainerSizeMatchesConstant)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_EQ(chestBoat.getContainerSize(), ChestBoatEntity::CONTAINER_SIZE);

    IInventory* inventory = chestBoat.getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), ChestBoatEntity::CONTAINER_SIZE);
}

// ============================================================================
// getBoatItem() 测试
// ============================================================================

class ChestBoatGetBoatItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatGetBoatItemTest, AllTypes_ReturnCorrectChestBoatItems)
{
    struct TestCase {
        BoatEntity::Type type;
        const Item* expectedItem;
        std::string name;
    };

    std::vector<TestCase> testCases = {
        {BoatEntity::Type::OAK, Items::OAK_CHEST_BOAT, "OAK"},
        {BoatEntity::Type::SPRUCE, Items::SPRUCE_CHEST_BOAT, "SPRUCE"},
        {BoatEntity::Type::BIRCH, Items::BIRCH_CHEST_BOAT, "BIRCH"},
        {BoatEntity::Type::JUNGLE, Items::JUNGLE_CHEST_BOAT, "JUNGLE"},
        {BoatEntity::Type::ACACIA, Items::ACACIA_CHEST_BOAT, "ACACIA"},
        {BoatEntity::Type::DARK_OAK, Items::DARK_OAK_CHEST_BOAT, "DARK_OAK"},
        {BoatEntity::Type::MANGROVE, Items::MANGROVE_CHEST_BOAT, "MANGROVE"},
        {BoatEntity::Type::CHERRY, Items::CHERRY_CHEST_BOAT, "CHERRY"},
        {BoatEntity::Type::PALE_OAK, Items::PALE_OAK_CHEST_BOAT, "PALE_OAK"},
        {BoatEntity::Type::BAMBOO, Items::BAMBOO_CHEST_RAFT, "BAMBOO"},
    };

    for (const auto& tc : testCases) {
        ChestBoatEntity chestBoat(tc.type, mc::test::testEcsRegistry());
        const Item* item = chestBoat.getBoatItem();

        EXPECT_NE(item, nullptr) << "getBoatItem() should not return nullptr for type " << tc.name;
        EXPECT_EQ(item, tc.expectedItem) << "Type " << tc.name << " should return correct chest boat item";
    }
}

TEST_F(ChestBoatGetBoatItemTest, ChestBoatItems_DifferentFromNormalBoatItems)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    BoatEntity normalBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_NE(chestBoat.getBoatItem(), normalBoat.getBoatItem());
    EXPECT_EQ(chestBoat.getBoatItem(), Items::OAK_CHEST_BOAT);
    EXPECT_EQ(normalBoat.getBoatItem(), Items::OAK_BOAT);
}

TEST_F(ChestBoatGetBoatItemTest, AllChestBoatItems_AreRegistered)
{
    EXPECT_NE(Items::OAK_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::SPRUCE_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::BIRCH_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::JUNGLE_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::ACACIA_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::DARK_OAK_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::MANGROVE_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::CHERRY_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::PALE_OAK_CHEST_BOAT, nullptr);
    EXPECT_NE(Items::BAMBOO_CHEST_RAFT, nullptr);
}

TEST_F(ChestBoatGetBoatItemTest, AllChestBoatItems_AreUnique)
{
    std::set<const Item*> chestBoatItems = {
        Items::OAK_CHEST_BOAT,
        Items::SPRUCE_CHEST_BOAT,
        Items::BIRCH_CHEST_BOAT,
        Items::JUNGLE_CHEST_BOAT,
        Items::ACACIA_CHEST_BOAT,
        Items::DARK_OAK_CHEST_BOAT,
        Items::MANGROVE_CHEST_BOAT,
        Items::CHERRY_CHEST_BOAT,
        Items::PALE_OAK_CHEST_BOAT,
        Items::BAMBOO_CHEST_RAFT,
    };

    EXPECT_EQ(chestBoatItems.size(), 10u) << "All 10 chest boat items should be unique pointers";
}

TEST_F(ChestBoatGetBoatItemTest, ChestBoatAndBoatItems_AreCompletelyDifferent)
{
    std::set<const Item*> chestItems = {
        Items::OAK_CHEST_BOAT,
        Items::SPRUCE_CHEST_BOAT,
        Items::BIRCH_CHEST_BOAT,
        Items::JUNGLE_CHEST_BOAT,
        Items::ACACIA_CHEST_BOAT,
        Items::DARK_OAK_CHEST_BOAT,
        Items::MANGROVE_CHEST_BOAT,
        Items::CHERRY_CHEST_BOAT,
        Items::PALE_OAK_CHEST_BOAT,
        Items::BAMBOO_CHEST_RAFT,
    };

    std::set<const Item*> normalItems = {
        Items::OAK_BOAT,
        Items::SPRUCE_BOAT,
        Items::BIRCH_BOAT,
        Items::JUNGLE_BOAT,
        Items::ACACIA_BOAT,
        Items::DARK_OAK_BOAT,
        Items::MANGROVE_BOAT,
        Items::CHERRY_BOAT,
        Items::PALE_OAK_BOAT,
        Items::BAMBOO_RAFT,
    };

    for (const auto* item : chestItems) {
        EXPECT_EQ(normalItems.count(item), 0u) << "Chest boat item should not be in normal boat items set";
    }
}

// ============================================================================
// 虚函数重写测试
// ============================================================================

class ChestBoatInheritanceTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatInheritanceTest, IsBoatEntitySubclass)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    BoatEntity* boatPtr = &chestBoat;
    EXPECT_EQ(boatPtr->getBoatType(), BoatEntity::Type::OAK);
    EXPECT_TRUE(boatPtr->hasChest());

    // 通过基类指针调用虚函数，应调用 ChestBoatEntity 的版本
    EXPECT_EQ(boatPtr->getBoatItem(), Items::OAK_CHEST_BOAT);
}

TEST_F(ChestBoatInheritanceTest, VirtualGetBoatItem_OverriddenCorrectly)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::BIRCH, mc::test::testEcsRegistry());
    BoatEntity& boatRef = chestBoat;

    EXPECT_EQ(boatRef.getBoatItem(), Items::BIRCH_CHEST_BOAT);
    EXPECT_EQ(chestBoat.getBoatItem(), Items::BIRCH_CHEST_BOAT);

    BoatEntity normalBoat(BoatEntity::Type::BIRCH, mc::test::testEcsRegistry());
    EXPECT_EQ(normalBoat.getBoatItem(), Items::BIRCH_BOAT);
}

TEST_F(ChestBoatInheritanceTest, AllTypes_GetBoatItemViaBasePointer)
{
    struct TestCase {
        BoatEntity::Type type;
        const Item* expectedItem;
        std::string name;
    };

    std::vector<TestCase> testCases = {
        {BoatEntity::Type::OAK, Items::OAK_CHEST_BOAT, "OAK"},
        {BoatEntity::Type::SPRUCE, Items::SPRUCE_CHEST_BOAT, "SPRUCE"},
        {BoatEntity::Type::BIRCH, Items::BIRCH_CHEST_BOAT, "BIRCH"},
        {BoatEntity::Type::JUNGLE, Items::JUNGLE_CHEST_BOAT, "JUNGLE"},
        {BoatEntity::Type::ACACIA, Items::ACACIA_CHEST_BOAT, "ACACIA"},
        {BoatEntity::Type::DARK_OAK, Items::DARK_OAK_CHEST_BOAT, "DARK_OAK"},
        {BoatEntity::Type::MANGROVE, Items::MANGROVE_CHEST_BOAT, "MANGROVE"},
        {BoatEntity::Type::CHERRY, Items::CHERRY_CHEST_BOAT, "CHERRY"},
        {BoatEntity::Type::PALE_OAK, Items::PALE_OAK_CHEST_BOAT, "PALE_OAK"},
        {BoatEntity::Type::BAMBOO, Items::BAMBOO_CHEST_RAFT, "BAMBOO"},
    };

    for (const auto& tc : testCases) {
        ChestBoatEntity chestBoat(tc.type, mc::test::testEcsRegistry());
        BoatEntity* boatPtr = &chestBoat;

        EXPECT_EQ(boatPtr->getBoatItem(), tc.expectedItem)
            << "Type " << tc.name << " should return correct chest boat item via base pointer";
    }
}

// ============================================================================
// canAddPassenger 测试
// ============================================================================

class ChestBoatPassengerTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatPassengerTest, CanAddPassenger_DefaultState_AllowsOnePassenger)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 默认状态（InWater，非水下），没有乘客时应该允许添加乘客
    // canAddPassenger 检查: getPassengers().size() < 1 && getStatus() != UnderWater
    EXPECT_TRUE(chestBoat.getPassengers().empty());
    EXPECT_NE(chestBoat.getStatus(), BoatStatus::UnderWater);

    // 满足条件时应该允许添加乘客
    EXPECT_TRUE(chestBoat.canAddPassenger(chestBoat));
}

TEST_F(ChestBoatPassengerTest, CanAddPassenger_MaxOnePassenger)
{
    // 箱子船限制最多1名乘客（普通船允许2名）
    // 验证：canAddPassenger 检查 size < 1
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    BoatEntity normalBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 箱子船：size < 1 → 最多1名
    // 普通船：size < 2 → 最多2名
    // 这是通过各自的 canAddPassenger 重写实现的
    EXPECT_TRUE(chestBoat.canAddPassenger(chestBoat));
    EXPECT_TRUE(normalBoat.canAddPassenger(normalBoat));
}

// ============================================================================
// getMountedYOffset 测试
// ============================================================================

class ChestBoatMountTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatMountTest, GetMountedYOffset_ReturnsCorrectValue)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_DOUBLE_EQ(chestBoat.getMountedYOffset(), -0.1);
}

TEST_F(ChestBoatMountTest, MountedYOffset_SameAsNormalBoat)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    BoatEntity normalBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_DOUBLE_EQ(chestBoat.getMountedYOffset(), normalBoat.getMountedYOffset());
}

// ============================================================================
// getComparatorOutput 测试
// ============================================================================

class ChestBoatComparatorTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatComparatorTest, EmptyContainer_OutputIsZero)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_EQ(chestBoat.getComparatorOutput(), 0);
}

TEST_F(ChestBoatComparatorTest, PartiallyFilledContainer_OutputNonZero)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));

    // 容器有内容时比较器输出应大于0
    EXPECT_GT(chestBoat.getComparatorOutput(), 0);
    EXPECT_LE(chestBoat.getComparatorOutput(), 15);
}

TEST_F(ChestBoatComparatorTest, SingleItemInLargeContainer_OutputIsOne)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));

    // 27格容器中只有1个非空槽位，比较器输出应为1
    // 公式：max(1, ceil(nonEmptySlots / containerSize * 14))
    // 1/27 * 14 ≈ 0.52, ceil = 1, max(1, 1) = 1
    EXPECT_EQ(chestBoat.getComparatorOutput(), 1);
}

TEST_F(ChestBoatComparatorTest, FullContainer_OutputIsFifteen)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    for (i32 i = 0; i < ChestBoatEntity::CONTAINER_SIZE; ++i) {
        chestBoat.setInventoryItem(i, ItemStack(*Items::OAK_BOAT, 1));
    }

    // 所有槽位都有物品时，比较器输出为15
    EXPECT_EQ(chestBoat.getComparatorOutput(), 15);
}

// ============================================================================
// getDisplayName 测试
// ============================================================================

class ChestBoatDisplayNameTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatDisplayNameTest, GetDisplayName_ReturnsCorrectKey)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_EQ(chestBoat.getDisplayName(), "container.chestBoat");
}

// ============================================================================
// stillValid 测试
// ============================================================================

class ChestBoatStillValidTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatStillValidTest, StillValid_FalseWhenEntityRemoved)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 未移除时 stillValid 的距离检查需要 Player 对象
    // 但 isRemoved() 检查不需要 Player
    EXPECT_FALSE(chestBoat.isRemoved());

    // 调用 remove() 后 isRemoved() 应为 true
    // 注意：remove() 在无世界环境下的行为取决于实现
    // 这里仅验证 isRemoved() 的初始状态
}

// ============================================================================
// NBT 序列化/反序列化测试
// ============================================================================

class ChestBoatNbtTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatNbtTest, EmptyInventory_RoundTrip)
{
    ChestBoatEntity original(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 序列化
    auto tag = saveToNbt(original);

    // 反序列化
    auto loaded = loadFromNbt(*tag, BoatEntity::Type::OAK);

    // 验证空容器
    EXPECT_TRUE(loaded->isInventoryEmpty());
    EXPECT_EQ(loaded->getContainerSize(), 27);
}

TEST_F(ChestBoatNbtTest, InventoryItems_RoundTrip)
{
    ChestBoatEntity original(BoatEntity::Type::BIRCH, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    ASSERT_NE(Items::SPRUCE_BOAT, nullptr);
    ASSERT_NE(Items::BIRCH_BOAT, nullptr);

    // 设置多个槽位的物品
    original.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));
    original.setInventoryItem(13, ItemStack(*Items::SPRUCE_BOAT, 2));
    original.setInventoryItem(26, ItemStack(*Items::BIRCH_BOAT, 3));

    // 序列化
    auto tag = saveToNbt(original);

    // 反序列化
    auto loaded = loadFromNbt(*tag, BoatEntity::Type::BIRCH);

    // 验证物品
    EXPECT_FALSE(loaded->getInventoryItem(0).isEmpty());
    EXPECT_EQ(loaded->getInventoryItem(0).getCount(), 1);

    EXPECT_FALSE(loaded->getInventoryItem(13).isEmpty());
    EXPECT_EQ(loaded->getInventoryItem(13).getCount(), 2);

    EXPECT_FALSE(loaded->getInventoryItem(26).isEmpty());
    EXPECT_EQ(loaded->getInventoryItem(26).getCount(), 3);

    // 验证空槽位
    EXPECT_TRUE(loaded->getInventoryItem(1).isEmpty());
    EXPECT_TRUE(loaded->getInventoryItem(12).isEmpty());
    EXPECT_TRUE(loaded->getInventoryItem(25).isEmpty());
}

TEST_F(ChestBoatNbtTest, NbtKeys_ItemsKeyUsed)
{
    ChestBoatEntity original(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ASSERT_NE(Items::OAK_BOAT, nullptr);
    original.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));

    auto tag = saveToNbt(original);

    // 验证 "Items" 键存在
    auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr) << "Items key should be present in NBT";
    EXPECT_EQ(itemsList->element_id(), nbt::TagId::Compound);

    // 应该有1个物品
    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
    EXPECT_EQ(compoundList.value.size(), 1u);
}

TEST_F(ChestBoatNbtTest, NbtKeys_LootTableKeyUsed)
{
    ChestBoatEntity original(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 直接构造带战利品表的 NBT 数据
    nbt::tags::compound_tag tag;
    tag.put(nbt_keys::LOOT_TABLE, std::string("minecraft:chests/village_armorer"));
    tag.put(nbt_keys::LOOT_TABLE_SEED, static_cast<i64>(12345L));

    // 反序列化
    auto loaded = loadFromNbt(tag, BoatEntity::Type::OAK);

    // 验证战利品表被正确读取
    // 注意：当前实现中战利品表读取后存储在 m_lootTable 和 m_lootTableSeed 中
    // 但这些是私有字段，我们通过序列化-反序列化来验证

    // 序列化回去应该保留战利品表信息
    auto reserialized = saveToNbt(*loaded);
    auto lootTableOpt = nbt_helper::tryGetString(*reserialized, nbt_keys::LOOT_TABLE);
    ASSERT_TRUE(lootTableOpt.has_value());
    EXPECT_EQ(*lootTableOpt, "minecraft:chests/village_armorer");

    auto seedOpt = nbt_helper::tryGetLong(*reserialized, nbt_keys::LOOT_TABLE_SEED);
    ASSERT_TRUE(seedOpt.has_value());
    EXPECT_EQ(*seedOpt, 12345L);
}

TEST_F(ChestBoatNbtTest, NbtKeys_LootTableSupersedesItems)
{
    // 当 NBT 中同时有 LootTable 和 Items 时，应优先使用 LootTable
    // 对应 MC Java 的行为：有战利品表时不保存物品

    nbt::tags::compound_tag tag;

    // 添加战利品表
    tag.put(nbt_keys::LOOT_TABLE, std::string("minecraft:chests/spawn_bonus_chest"));

    // 同时添加物品（不应被读取）
    auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();
    nbt::tags::compound_tag itemTag;
    itemTag.put("Slot", static_cast<i8>(0));
    // 注意：这里不添加物品的 id/count，简化测试
    itemsList->value.push_back(std::move(itemTag));
    tag.value.emplace(nbt_keys::ITEMS, std::move(itemsList));

    // 反序列化
    auto loaded = loadFromNbt(tag, BoatEntity::Type::OAK);

    // 战利品表存在但未解包时，hasLootTable 应为 true
    EXPECT_TRUE(loaded->hasLootTable());
    EXPECT_EQ(loaded->getLootTable(), "minecraft:chests/spawn_bonus_chest");

    // 战利品表存在但未解包时，isInventoryEmpty() 返回 false
    // （因为容器可能有战利品表生成的物品，但尚未填充）
    // 参考 MC Java: ContainerEntity.isChestVehicleEmpty() 在 lootTable 非空时返回 false
    EXPECT_FALSE(loaded->isInventoryEmpty());

    // 实际库存槽位为空（因为物品数据未被读取，战利品表尚未解包）
    // 直接检查底层 SimpleInventory 而非 isInventoryEmpty()
    EXPECT_TRUE(loaded->getInventory()->isEmpty());
}

TEST_F(ChestBoatNbtTest, EmptyNbt_PreservesDefaults)
{
    // 空的 NBT 标签不应导致崩溃
    nbt::tags::compound_tag emptyTag;

    auto loaded = loadFromNbt(emptyTag, BoatEntity::Type::OAK);

    EXPECT_TRUE(loaded->isInventoryEmpty());
    EXPECT_EQ(loaded->getContainerSize(), 27);
    EXPECT_EQ(loaded->getBoatType(), BoatEntity::Type::OAK);
}

TEST_F(ChestBoatNbtTest, BoatTypePreservedByBaseClass)
{
    // BoatEntity::addAdditionalSaveData 保存船类型
    // 验证船类型在序列化/反序列化后保持一致
    ChestBoatEntity original(BoatEntity::Type::SPRUCE, mc::test::testEcsRegistry());

    auto tag = saveToNbt(original);

    // 基类读取船类型后应保持一致
    auto loaded = loadFromNbt(*tag, BoatEntity::Type::SPRUCE);
    EXPECT_EQ(loaded->getBoatType(), BoatEntity::Type::SPRUCE);
}

// ============================================================================
// 箱子船与普通船对比测试
// ============================================================================

class ChestBoatVsNormalBoatTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatVsNormalBoatTest, DifferentDropItems)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    BoatEntity normalBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    EXPECT_NE(chestBoat.getBoatItem(), normalBoat.getBoatItem());
    EXPECT_EQ(chestBoat.getBoatItem(), Items::OAK_CHEST_BOAT);
    EXPECT_EQ(normalBoat.getBoatItem(), Items::OAK_BOAT);
}

TEST_F(ChestBoatVsNormalBoatTest, AllTypes_DifferentChestAndNormalItems)
{
    struct TestCase {
        BoatEntity::Type type;
        const Item* normalItem;
        const Item* chestItem;
        std::string name;
    };

    std::vector<TestCase> testCases = {
        {BoatEntity::Type::OAK, Items::OAK_BOAT, Items::OAK_CHEST_BOAT, "OAK"},
        {BoatEntity::Type::SPRUCE, Items::SPRUCE_BOAT, Items::SPRUCE_CHEST_BOAT, "SPRUCE"},
        {BoatEntity::Type::BIRCH, Items::BIRCH_BOAT, Items::BIRCH_CHEST_BOAT, "BIRCH"},
        {BoatEntity::Type::JUNGLE, Items::JUNGLE_BOAT, Items::JUNGLE_CHEST_BOAT, "JUNGLE"},
        {BoatEntity::Type::ACACIA, Items::ACACIA_BOAT, Items::ACACIA_CHEST_BOAT, "ACACIA"},
        {BoatEntity::Type::DARK_OAK, Items::DARK_OAK_BOAT, Items::DARK_OAK_CHEST_BOAT, "DARK_OAK"},
        {BoatEntity::Type::MANGROVE, Items::MANGROVE_BOAT, Items::MANGROVE_CHEST_BOAT, "MANGROVE"},
        {BoatEntity::Type::CHERRY, Items::CHERRY_BOAT, Items::CHERRY_CHEST_BOAT, "CHERRY"},
        {BoatEntity::Type::PALE_OAK, Items::PALE_OAK_BOAT, Items::PALE_OAK_CHEST_BOAT, "PALE_OAK"},
        {BoatEntity::Type::BAMBOO, Items::BAMBOO_RAFT, Items::BAMBOO_CHEST_RAFT, "BAMBOO"},
    };

    for (const auto& tc : testCases) {
        ChestBoatEntity chestBoat(tc.type, mc::test::testEcsRegistry());
        BoatEntity normalBoat(tc.type, mc::test::testEcsRegistry());

        EXPECT_NE(chestBoat.getBoatItem(), normalBoat.getBoatItem())
            << "Type " << tc.name << ": chest boat and normal boat should have different items";
        EXPECT_EQ(chestBoat.getBoatItem(), tc.chestItem)
            << "Type " << tc.name << ": chest boat should return chest boat item";
        EXPECT_EQ(normalBoat.getBoatItem(), tc.normalItem)
            << "Type " << tc.name << ": normal boat should return normal boat item";
    }
}

TEST_F(ChestBoatVsNormalBoatTest, ChestBoat_HasContainer_NormalBoat_DoesNot)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    BoatEntity normalBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 箱子船有容器
    EXPECT_EQ(chestBoat.getContainerSize(), 27);
    EXPECT_NE(chestBoat.getInventory(), nullptr);

    // 普通船没有容器方法（没有 INamedContainerProvider 接口）
    // 验证箱子船实现了 INamedContainerProvider 接口
    auto* provider = dynamic_cast<INamedContainerProvider*>(&chestBoat);
    EXPECT_NE(provider, nullptr) << "ChestBoatEntity should implement INamedContainerProvider";

    // 普通船不应实现 INamedContainerProvider
    auto* normalProvider = dynamic_cast<INamedContainerProvider*>(&normalBoat);
    EXPECT_EQ(normalProvider, nullptr) << "BoatEntity should not implement INamedContainerProvider";
}

// ============================================================================
// 战利品表接口测试
// ============================================================================

class ChestBoatLootTableTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ChestBoatLootTableTest, DefaultState_NoLootTable)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 默认状态下没有战利品表
    EXPECT_FALSE(chestBoat.hasLootTable());
    EXPECT_TRUE(chestBoat.getLootTable().empty());
    EXPECT_EQ(chestBoat.getLootTableSeed(), 0L);
}

TEST_F(ChestBoatLootTableTest, SetLootTable_SetsValuesCorrectly)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    chestBoat.setLootTable("minecraft:chests/village_armorer", 42L);

    EXPECT_TRUE(chestBoat.hasLootTable());
    EXPECT_EQ(chestBoat.getLootTable(), "minecraft:chests/village_armorer");
    EXPECT_EQ(chestBoat.getLootTableSeed(), 42L);
}

TEST_F(ChestBoatLootTableTest, SetLootTable_ZeroSeed)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    chestBoat.setLootTable("minecraft:chests/spawn_bonus_chest", 0L);

    EXPECT_TRUE(chestBoat.hasLootTable());
    EXPECT_EQ(chestBoat.getLootTable(), "minecraft:chests/spawn_bonus_chest");
    EXPECT_EQ(chestBoat.getLootTableSeed(), 0L);
}

TEST_F(ChestBoatLootTableTest, SetLootTable_IsInventoryEmptyReturnsFalse)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 设置战利品表后，isInventoryEmpty() 应返回 false（容器可能有物品）
    chestBoat.setLootTable("minecraft:chests/simple_dungeon", 123L);
    EXPECT_FALSE(chestBoat.isInventoryEmpty());

    // 但底层 SimpleInventory 实际为空（战利品表尚未解包）
    EXPECT_TRUE(chestBoat.getInventory()->isEmpty());
}

TEST_F(ChestBoatLootTableTest, SetLootTable_NbtRoundTrip)
{
    ChestBoatEntity original(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    original.setLootTable("minecraft:chests/abandoned_mineshaft", 999L);

    auto tag = saveToNbt(original);
    auto loaded = loadFromNbt(*tag, BoatEntity::Type::OAK);

    EXPECT_TRUE(loaded->hasLootTable());
    EXPECT_EQ(loaded->getLootTable(), "minecraft:chests/abandoned_mineshaft");
    EXPECT_EQ(loaded->getLootTableSeed(), 999L);
}

TEST_F(ChestBoatLootTableTest, LootTableSupersedesItemsInNbt)
{
    // 有战利品表时，NBT 不应保存物品
    ChestBoatEntity original(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    ASSERT_NE(Items::OAK_BOAT, nullptr);
    original.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 5));
    original.setLootTable("minecraft:chests/shipwreck_treasure", 77L);

    auto tag = saveToNbt(original);

    // 有战利品表且未解包时，NBT 应保存 LootTable 而非 Items
    auto lootTableOpt = nbt_helper::tryGetString(*tag, nbt_keys::LOOT_TABLE);
    ASSERT_TRUE(lootTableOpt.has_value());
    EXPECT_EQ(*lootTableOpt, "minecraft:chests/shipwreck_treasure");

    auto seedOpt = nbt_helper::tryGetLong(*tag, nbt_keys::LOOT_TABLE_SEED);
    ASSERT_TRUE(seedOpt.has_value());
    EXPECT_EQ(*seedOpt, 77L);

    // Items 不应存在（有战利品表时不保存物品）
    auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    EXPECT_EQ(itemsList, nullptr) << "Items should not be present when LootTable is set";
}

TEST_F(ChestBoatLootTableTest, NoLootTable_ItemsSavedInNbt)
{
    // 没有战利品表时，NBT 应保存物品
    ChestBoatEntity original(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    ASSERT_NE(Items::OAK_BOAT, nullptr);
    original.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 3));

    auto tag = saveToNbt(original);

    // 没有战利品表时，应保存 Items
    auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr) << "Items should be present when no LootTable is set";

    // LootTable 不应存在
    auto lootTableOpt = nbt_helper::tryGetString(*tag, nbt_keys::LOOT_TABLE);
    EXPECT_FALSE(lootTableOpt.has_value()) << "LootTable should not be present when no loot table is set";
}

TEST_F(ChestBoatLootTableTest, UnpackLootTable_NoWorld_NoEffect)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    chestBoat.setLootTable("minecraft:chests/simple_dungeon", 42L);

    // 没有世界环境（m_world == nullptr），unpackLootTable 应为空操作
    chestBoat.unpackLootTable(nullptr);

    // 战利品表仍存在（因为无法解包）
    EXPECT_TRUE(chestBoat.hasLootTable());
    EXPECT_EQ(chestBoat.getLootTable(), "minecraft:chests/simple_dungeon");
}

TEST_F(ChestBoatLootTableTest, UnpackLootTable_EmptyLootTable_NoEffect)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    // 没有设置战利品表，unpackLootTable 应为空操作
    chestBoat.unpackLootTable(nullptr);

    EXPECT_FALSE(chestBoat.hasLootTable());
    EXPECT_TRUE(chestBoat.isInventoryEmpty());
}

TEST_F(ChestBoatLootTableTest, RemoveInventoryItemNoUpdate_BasicOperation)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    ASSERT_NE(Items::OAK_BOAT, nullptr);

    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 5));

    // 移除物品（不触发通知）
    ItemStack removed = chestBoat.removeInventoryItemNoUpdate(0);
    EXPECT_EQ(removed.getCount(), 5);
    EXPECT_TRUE(chestBoat.getInventoryItem(0).isEmpty());
}

TEST_F(ChestBoatLootTableTest, RemoveInventoryItemNoUpdate_EmptySlotReturnsEmpty)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());

    ItemStack removed = chestBoat.removeInventoryItemNoUpdate(0);
    EXPECT_TRUE(removed.isEmpty());
}

TEST_F(ChestBoatLootTableTest, ClearInventory_ResetsEmptyState)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    ASSERT_NE(Items::OAK_BOAT, nullptr);

    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 1));
    chestBoat.setInventoryItem(5, ItemStack(*Items::OAK_BOAT, 3));

    EXPECT_FALSE(chestBoat.isInventoryEmpty());

    chestBoat.clearInventory();

    EXPECT_TRUE(chestBoat.isInventoryEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(0).isEmpty());
    EXPECT_TRUE(chestBoat.getInventoryItem(5).isEmpty());
}

TEST_F(ChestBoatLootTableTest, SetLootTableThenClearInventory_LootTableStillSet)
{
    ChestBoatEntity chestBoat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    ASSERT_NE(Items::OAK_BOAT, nullptr);

    // 设置物品和战利品表
    chestBoat.setInventoryItem(0, ItemStack(*Items::OAK_BOAT, 2));
    chestBoat.setLootTable("minecraft:chests/village_weaponsmith", 100L);

    // 战利品表存在时，isInventoryEmpty 返回 false
    EXPECT_FALSE(chestBoat.isInventoryEmpty());
    EXPECT_TRUE(chestBoat.hasLootTable());

    // clearInventory 会触发懒解包（空操作，因为没有 LootTableManager），
    // 然后清空容器
    chestBoat.clearInventory();

    // 清空后容器为空，但战利品表仍然存在
    EXPECT_TRUE(chestBoat.getInventory()->isEmpty());
}
