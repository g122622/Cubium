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

#include "world/blockentity/transport/HopperEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "util/Direction.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/transport/IHopper.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

namespace {
Item* ensureHopperTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }
    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}
} // namespace

// ========== HopperEntity 测试 ==========

class HopperEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        hopper_ = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
        m_diamond = ensureHopperTestItem("diamond");
    }

    std::unique_ptr<HopperEntity> hopper_;
    Item* m_diamond = nullptr;
};

TEST_F(HopperEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(hopper_->getType(), BlockEntityType::Hopper);
}

TEST_F(HopperEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(hopper_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(HopperEntityTest, Create_HasCorrectSize)
{
    EXPECT_EQ(hopper_->getContainerSize(), HopperEntity::HOPPER_SIZE);
    EXPECT_EQ(HopperEntity::HOPPER_SIZE, 5); // 漏斗有5格
}

TEST_F(HopperEntityTest, Create_IsEmptyInitially)
{
    EXPECT_TRUE(hopper_->isEmpty());
}

TEST_F(HopperEntityTest, Create_TransferCooldownIsMinusOne)
{
    // -1 表示刚放置的漏斗
    EXPECT_EQ(hopper_->getTransferCooldown(), -1);
}

TEST_F(HopperEntityTest, Create_NotOnTransferCooldownInitially)
{
    // -1 意味着不在冷却中
    EXPECT_FALSE(hopper_->isOnTransferCooldown());
}

TEST_F(HopperEntityTest, SetTransferCooldown_UpdatesValue)
{
    hopper_->setTransferCooldown(8);
    EXPECT_EQ(hopper_->getTransferCooldown(), 8);
}

TEST_F(HopperEntityTest, SetTransferCooldown_SetsOnCooldown)
{
    hopper_->setTransferCooldown(5);
    EXPECT_TRUE(hopper_->isOnTransferCooldown());
}

TEST_F(HopperEntityTest, SetTransferCooldown_ZeroMeansNotOnCooldown)
{
    hopper_->setTransferCooldown(0);
    EXPECT_FALSE(hopper_->isOnTransferCooldown());
}

TEST_F(HopperEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(hopper_->needsTick());
}

TEST_F(HopperEntityTest, GetInventory_ReturnsValidPointer)
{
    IInventory* inventory = hopper_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), HopperEntity::HOPPER_SIZE);
}

TEST_F(HopperEntityTest, Save_ContainsBasicInfo)
{
    nlohmann::json data;
    hopper_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:hopper");
    EXPECT_TRUE(data.contains("TransferCooldown"));
}

TEST_F(HopperEntityTest, Load_LoadsTransferCooldown)
{
    nlohmann::json data;
    data["TransferCooldown"] = 5;

    EXPECT_TRUE(hopper_->load(data));
    EXPECT_EQ(hopper_->getTransferCooldown(), 5);
}

TEST_F(HopperEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = hopper_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Hopper);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(HopperEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(hopper_->isChanged());
    hopper_->setChanged();
    EXPECT_TRUE(hopper_->isChanged());
}

TEST_F(HopperEntityTest, IsFull_ReturnsFalseWhenEmpty)
{
    EXPECT_FALSE(hopper_->isFull());
}

TEST_F(HopperEntityTest, TransferCooldown_ConstantsAreCorrect)
{
    EXPECT_EQ(HopperEntity::TRANSFER_COOLDOWN, 8);
    EXPECT_EQ(HopperEntity::TRANSFER_COOLDOWN_CHAIN, 7);
}

TEST_F(HopperEntityTest, MayTransfer_ReturnsFalseForNormalCooldown)
{
    hopper_->setTransferCooldown(8);
    EXPECT_FALSE(hopper_->mayTransfer());
}

TEST_F(HopperEntityTest, MayTransfer_ReturnsTrueForAboveNormalCooldown)
{
    hopper_->setTransferCooldown(9);
    EXPECT_TRUE(hopper_->mayTransfer());
}

// ========== IHopper 接口测试 ==========

class IHopperTest : public ::testing::Test {
protected:
    void SetUp() override { hopper_ = std::make_unique<HopperEntity>(BlockPos(100, 64, -50)); }

    std::unique_ptr<HopperEntity> hopper_;
};

TEST_F(IHopperTest, GetHopperPos_ReturnsCorrectPosition)
{
    EXPECT_EQ(hopper_->getHopperPos(), BlockPos(100, 64, -50));
}

TEST_F(IHopperTest, GetXPos_ReturnsCenterX)
{
    EXPECT_DOUBLE_EQ(hopper_->getXPos(), 100.5);
}

TEST_F(IHopperTest, GetYPos_ReturnsCenterY)
{
    EXPECT_DOUBLE_EQ(hopper_->getYPos(), 64.5);
}

TEST_F(IHopperTest, GetZPos_ReturnsCenterZ)
{
    EXPECT_DOUBLE_EQ(hopper_->getZPos(), -49.5); // -50 + 0.5
}

TEST_F(IHopperTest, GetOutputDirection_ReturnsDownByDefault)
{
    EXPECT_EQ(hopper_->getOutputDirection(), Direction::Down);
}

TEST_F(IHopperTest, GetWorld_ReturnsNullptrInitially)
{
    EXPECT_EQ(hopper_->getWorld(), nullptr);
}

TEST_F(IHopperTest, GetCollectionArea_ReturnsValidAABB)
{
    AxisAlignedBB area = IHopper::getCollectionArea(*hopper_);

    // 收集区域应该在漏斗上方一格
    // 碗状区域 + 上方完整方块区域
    EXPECT_GE(area.minY, hopper_->getYPos()); // 至少从漏斗中心开始
}

// ========== Direction 工具函数测试（补充边界条件测试） ==========

class DirectionTest : public ::testing::Test {};

TEST_F(DirectionTest, Opposite_DownReturnsUp)
{
    EXPECT_EQ(Directions::opposite(Direction::Down), Direction::Up);
}

TEST_F(DirectionTest, Opposite_UpReturnsDown)
{
    EXPECT_EQ(Directions::opposite(Direction::Up), Direction::Down);
}

TEST_F(DirectionTest, Opposite_NorthReturnsSouth)
{
    EXPECT_EQ(Directions::opposite(Direction::North), Direction::South);
}

TEST_F(DirectionTest, Opposite_SouthReturnsNorth)
{
    EXPECT_EQ(Directions::opposite(Direction::South), Direction::North);
}

TEST_F(DirectionTest, Opposite_WestReturnsEast)
{
    EXPECT_EQ(Directions::opposite(Direction::West), Direction::East);
}

TEST_F(DirectionTest, Opposite_EastReturnsWest)
{
    EXPECT_EQ(Directions::opposite(Direction::East), Direction::West);
}

TEST_F(DirectionTest, Opposite_NoneReturnsNone)
{
    // 测试边界条件
    EXPECT_EQ(Directions::opposite(Direction::None), Direction::None);
}

TEST_F(DirectionTest, IsValid_ReturnsTrueForValidDirections)
{
    EXPECT_TRUE(Directions::isValid(Direction::Down));
    EXPECT_TRUE(Directions::isValid(Direction::Up));
    EXPECT_TRUE(Directions::isValid(Direction::North));
    EXPECT_TRUE(Directions::isValid(Direction::South));
    EXPECT_TRUE(Directions::isValid(Direction::West));
    EXPECT_TRUE(Directions::isValid(Direction::East));
}

TEST_F(DirectionTest, IsValid_ReturnsFalseForNone)
{
    EXPECT_FALSE(Directions::isValid(Direction::None));
}

TEST_F(DirectionTest, XOffset_ReturnsCorrectValues)
{
    EXPECT_EQ(Directions::xOffset(Direction::West), -1);
    EXPECT_EQ(Directions::xOffset(Direction::East), 1);
    EXPECT_EQ(Directions::xOffset(Direction::Up), 0);
    EXPECT_EQ(Directions::xOffset(Direction::Down), 0);
    EXPECT_EQ(Directions::xOffset(Direction::North), 0);
    EXPECT_EQ(Directions::xOffset(Direction::South), 0);
}

TEST_F(DirectionTest, YOffset_ReturnsCorrectValues)
{
    EXPECT_EQ(Directions::yOffset(Direction::Down), -1);
    EXPECT_EQ(Directions::yOffset(Direction::Up), 1);
    EXPECT_EQ(Directions::yOffset(Direction::North), 0);
    EXPECT_EQ(Directions::yOffset(Direction::South), 0);
    EXPECT_EQ(Directions::yOffset(Direction::West), 0);
    EXPECT_EQ(Directions::yOffset(Direction::East), 0);
}

TEST_F(DirectionTest, ZOffset_ReturnsCorrectValues)
{
    EXPECT_EQ(Directions::zOffset(Direction::North), -1);
    EXPECT_EQ(Directions::zOffset(Direction::South), 1);
    EXPECT_EQ(Directions::zOffset(Direction::Up), 0);
    EXPECT_EQ(Directions::zOffset(Direction::Down), 0);
    EXPECT_EQ(Directions::zOffset(Direction::West), 0);
    EXPECT_EQ(Directions::zOffset(Direction::East), 0);
}

TEST_F(DirectionTest, XOffset_NoneReturnsZero)
{
    EXPECT_EQ(Directions::xOffset(Direction::None), 0);
}

TEST_F(DirectionTest, YOffset_NoneReturnsZero)
{
    EXPECT_EQ(Directions::yOffset(Direction::None), 0);
}

TEST_F(DirectionTest, ZOffset_NoneReturnsZero)
{
    EXPECT_EQ(Directions::zOffset(Direction::None), 0);
}

TEST_F(DirectionTest, ToString_NoneReturnsNone)
{
    EXPECT_EQ(Directions::toString(Direction::None), "none");
}

TEST_F(DirectionTest, GetAxis_NoneReturnsY)
{
    // 默认返回Y轴
    EXPECT_EQ(Directions::getAxis(Direction::None), Axis::Y);
}

TEST_F(DirectionTest, GetAxisDirection_NoneReturnsPositive)
{
    EXPECT_EQ(Directions::getAxisDirection(Direction::None), AxisDirection::Positive);
}

TEST_F(DirectionTest, IsHorizontal_ReturnsTrueForHorizontalDirections)
{
    EXPECT_TRUE(Directions::isHorizontal(Direction::North));
    EXPECT_TRUE(Directions::isHorizontal(Direction::South));
    EXPECT_TRUE(Directions::isHorizontal(Direction::West));
    EXPECT_TRUE(Directions::isHorizontal(Direction::East));
}

TEST_F(DirectionTest, IsHorizontal_ReturnsFalseForVerticalDirections)
{
    EXPECT_FALSE(Directions::isHorizontal(Direction::Up));
    EXPECT_FALSE(Directions::isHorizontal(Direction::Down));
}

TEST_F(DirectionTest, IsHorizontal_ReturnsFalseForNone)
{
    EXPECT_FALSE(Directions::isHorizontal(Direction::None));
}

TEST_F(DirectionTest, IsVertical_ReturnsTrueForVerticalDirections)
{
    EXPECT_TRUE(Directions::isVertical(Direction::Up));
    EXPECT_TRUE(Directions::isVertical(Direction::Down));
}

TEST_F(DirectionTest, IsVertical_ReturnsFalseForHorizontalDirections)
{
    EXPECT_FALSE(Directions::isVertical(Direction::North));
    EXPECT_FALSE(Directions::isVertical(Direction::South));
    EXPECT_FALSE(Directions::isVertical(Direction::West));
    EXPECT_FALSE(Directions::isVertical(Direction::East));
}

TEST_F(DirectionTest, IsVertical_ReturnsFalseForNone)
{
    EXPECT_FALSE(Directions::isVertical(Direction::None));
}

TEST_F(DirectionTest, FromDelta_ReturnsCorrectDirections)
{
    EXPECT_EQ(Directions::fromDelta(0, -1, 0), Direction::Down);
    EXPECT_EQ(Directions::fromDelta(0, 1, 0), Direction::Up);
    EXPECT_EQ(Directions::fromDelta(0, 0, -1), Direction::North);
    EXPECT_EQ(Directions::fromDelta(0, 0, 1), Direction::South);
    EXPECT_EQ(Directions::fromDelta(-1, 0, 0), Direction::West);
    EXPECT_EQ(Directions::fromDelta(1, 0, 0), Direction::East);
}

TEST_F(DirectionTest, FromDelta_ReturnsNoneForInvalidDeltas)
{
    EXPECT_EQ(Directions::fromDelta(0, 0, 0), Direction::None);
    EXPECT_EQ(Directions::fromDelta(1, 1, 0), Direction::None); // 多个轴有变化
    EXPECT_EQ(Directions::fromDelta(2, 0, 0), Direction::None); // 值不在 -1, 0, 1 范围
}

TEST_F(HopperEntityTest, SaveLoad_PreservesInventoryBySlot)
{
    IInventory* inventory = hopper_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(2, ItemStack(m_diamond, 6));

    nlohmann::json data;
    hopper_->save(data);

    HopperEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    EXPECT_EQ(loadedInventory->getItem(2).getItem(), m_diamond);
    EXPECT_EQ(loadedInventory->getItem(2).getCount(), 6);
}

TEST_F(HopperEntityTest, Clone_CopiesInventoryAndCooldownState)
{
    hopper_->setTransferCooldown(9);
    IInventory* inventory = hopper_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(1, ItemStack(m_diamond, 4));

    std::unique_ptr<BlockEntity> copy = hopper_->clone();
    ASSERT_NE(copy, nullptr);

    const auto* cloned = dynamic_cast<const HopperEntity*>(copy.get());
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getTransferCooldown(), 9);

    const IInventory* clonedInventory = cloned->getInventory();
    ASSERT_NE(clonedInventory, nullptr);
    EXPECT_EQ(clonedInventory->getItem(1).getItem(), m_diamond);
    EXPECT_EQ(clonedInventory->getItem(1).getCount(), 4);
}
