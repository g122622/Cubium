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

#include "world/blockentity/BlockEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;

// 测试用的简单方块实体
class TestBlockEntity : public BlockEntity {
public:
    TestBlockEntity(BlockEntityType type, const BlockPos& pos)
        : BlockEntity(type, pos)
    {}

    std::unique_ptr<BlockEntity> clone() const override { return std::make_unique<TestBlockEntity>(m_type, m_pos); }
};

// 测试用的容器方块实体
class TestContainerBlockEntity : public ContainerBlockEntity {
public:
    TestContainerBlockEntity(BlockEntityType type, const BlockPos& pos)
        : ContainerBlockEntity(type, pos)
    {}

    std::unique_ptr<BlockEntity> clone() const override
    {
        return std::make_unique<TestContainerBlockEntity>(m_type, m_pos);
    }
};

namespace {
Item* ensureBlockEntityTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}
} // namespace

class BlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ========== BlockEntityType 测试 ==========

TEST_F(BlockEntityTest, BlockEntityType_ToId_KnownTypes)
{
    EXPECT_EQ(blockEntityTypeToId(BlockEntityType::Chest).toString(), "minecraft:chest");
    EXPECT_EQ(blockEntityTypeToId(BlockEntityType::Furnace).toString(), "minecraft:furnace");
    EXPECT_EQ(blockEntityTypeToId(BlockEntityType::Hopper).toString(), "minecraft:hopper");
    EXPECT_EQ(blockEntityTypeToId(BlockEntityType::Sign).toString(), "minecraft:sign");
}

TEST_F(BlockEntityTest, BlockEntityType_ToId_UnknownReturnsUnknown)
{
    ResourceLocation id = blockEntityTypeToId(BlockEntityType::Unknown);
    EXPECT_EQ(id.toString(), "minecraft:unknown");
}

TEST_F(BlockEntityTest, BlockEntityType_FromId_KnownIds)
{
    EXPECT_EQ(blockEntityTypeFromId(ResourceLocation("minecraft", "chest")), BlockEntityType::Chest);
    EXPECT_EQ(blockEntityTypeFromId(ResourceLocation("minecraft", "furnace")), BlockEntityType::Furnace);
    EXPECT_EQ(blockEntityTypeFromId(ResourceLocation("minecraft", "sign")), BlockEntityType::Sign);
}

TEST_F(BlockEntityTest, BlockEntityType_FromId_ShortForm)
{
    // 简写形式也应该被识别
    EXPECT_EQ(blockEntityTypeFromId(ResourceLocation("chest")), BlockEntityType::Chest);
    EXPECT_EQ(blockEntityTypeFromId(ResourceLocation("furnace")), BlockEntityType::Furnace);
}

TEST_F(BlockEntityTest, BlockEntityType_FromId_UnknownReturnsUnknown)
{
    EXPECT_EQ(blockEntityTypeFromId(ResourceLocation("minecraft", "nonexistent")), BlockEntityType::Unknown);
}

// ========== BlockEntity 基础测试 ==========

TEST_F(BlockEntityTest, Create_GetTypeAndPos)
{
    BlockPos pos(10, 20, 30);
    TestBlockEntity entity(BlockEntityType::Chest, pos);

    EXPECT_EQ(entity.getType(), BlockEntityType::Chest);
    EXPECT_EQ(entity.getPos(), pos);
}

TEST_F(BlockEntityTest, ChangedFlag_InitiallyFalse)
{
    TestBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_FALSE(entity.isChanged());
}

TEST_F(BlockEntityTest, SetChanged_SetsFlag)
{
    TestBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));
    entity.setChanged();
    EXPECT_TRUE(entity.isChanged());
}

TEST_F(BlockEntityTest, ClearChanged_ClearsFlag)
{
    TestBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));
    entity.setChanged();
    entity.clearChanged();
    EXPECT_FALSE(entity.isChanged());
}

TEST_F(BlockEntityTest, NeedsTick_DefaultFalse)
{
    TestBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_FALSE(entity.needsTick());
}

TEST_F(BlockEntityTest, GetCustomName_DefaultEmpty)
{
    TestBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_TRUE(entity.getCustomName().empty());
}

TEST_F(BlockEntityTest, Save_ContainsBasicInfo)
{
    TestBlockEntity entity(BlockEntityType::Chest, BlockPos(10, 20, 30));

    nlohmann::json data;
    entity.save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_TRUE(data.contains("x"));
    EXPECT_TRUE(data.contains("y"));
    EXPECT_TRUE(data.contains("z"));
    EXPECT_EQ(data["id"], "minecraft:chest");
    EXPECT_EQ(data["x"].get<i32>(), 10);
    EXPECT_EQ(data["y"].get<i32>(), 20);
    EXPECT_EQ(data["z"].get<i32>(), 30);
}

TEST_F(BlockEntityTest, Clone_CreatesCopy)
{
    TestBlockEntity original(BlockEntityType::Chest, BlockPos(5, 10, 15));
    std::unique_ptr<BlockEntity> copy = original.clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), original.getType());
    EXPECT_EQ(copy->getPos(), original.getPos());
}

// ========== ContainerBlockEntity 测试 ==========

TEST_F(BlockEntityTest, Container_OpenCount_InitiallyZero)
{
    TestContainerBlockEntity container(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_EQ(container.getOpenCount(), 0);
}

TEST_F(BlockEntityTest, Container_OpenContainer_IncrementsCount)
{
    TestContainerBlockEntity container(BlockEntityType::Chest, BlockPos(0, 0, 0));
    container.openContainer(nullptr);
    EXPECT_EQ(container.getOpenCount(), 1);

    container.openContainer(nullptr);
    EXPECT_EQ(container.getOpenCount(), 2);
}

TEST_F(BlockEntityTest, Container_CloseContainer_DecrementsCount)
{
    TestContainerBlockEntity container(BlockEntityType::Chest, BlockPos(0, 0, 0));
    container.openContainer(nullptr);
    container.openContainer(nullptr);
    EXPECT_EQ(container.getOpenCount(), 2);

    container.closeContainer(nullptr);
    EXPECT_EQ(container.getOpenCount(), 1);
}

TEST_F(BlockEntityTest, Container_CloseContainer_NotBelowZero)
{
    TestContainerBlockEntity container(BlockEntityType::Chest, BlockPos(0, 0, 0));
    container.closeContainer(nullptr); // 没有打开时关闭
    EXPECT_EQ(container.getOpenCount(), 0);
}

TEST_F(BlockEntityTest, Container_GetInventory_ReturnsNullByDefault)
{
    TestContainerBlockEntity container(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_EQ(container.getInventory(), nullptr);
    EXPECT_EQ(container.getContainerSize(), 0);
}

TEST_F(BlockEntityTest, Container_IsEmpty_ReturnsTrueByDefault)
{
    TestContainerBlockEntity container(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_TRUE(container.isEmpty());
}

TEST_F(BlockEntityTest, Container_SaveLoad_PreservesItemsAndCustomName)
{
    Items::initialize();
    auto* apple = ensureBlockEntityTestItem("apple");
    ASSERT_NE(apple, nullptr);

    // vanilla 工作台不再是方块实体，改用 TestContainerBlockEntity 验证容器存档加载契约。
    TestContainerBlockEntity table(BlockEntityType::Chest, BlockPos(4, 5, 6));
    table.setCustomName("crafting.test");
    IInventory* inventory = table.getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(apple, 3));

    nlohmann::json data;
    table.save(data);

    TestContainerBlockEntity loaded(BlockEntityType::Chest, BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    EXPECT_EQ(loadedInventory->getItem(0).getItem(), apple);
    EXPECT_EQ(loadedInventory->getItem(0).getCount(), 3);
    EXPECT_EQ(loaded.getCustomName(), "crafting.test");
}

TEST_F(BlockEntityTest, Container_Load_UsesArrayIndexFallbackWhenSlotMissing)
{
    Items::initialize();
    auto* stick = ensureBlockEntityTestItem("stick");
    ASSERT_NE(stick, nullptr);

    nlohmann::json data;
    data["id"] = "minecraft:chest";
    data["x"] = 1;
    data["y"] = 2;
    data["z"] = 3;
    data["Items"] = nlohmann::json::array();
    data["Items"].push_back(ItemStack(stick, 2).toJson());

    TestContainerBlockEntity loaded(BlockEntityType::Chest, BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    EXPECT_EQ(loadedInventory->getItem(0).getItem(), stick);
    EXPECT_EQ(loadedInventory->getItem(0).getCount(), 2);
}
