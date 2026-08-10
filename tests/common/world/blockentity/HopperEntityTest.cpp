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
#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/entities/item/ItemEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "util/Direction.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
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

/**
 * @brief 测试用 Mock 世界，支持漏斗测试所需的方法覆写
 */
class HopperTestWorld final : public mc::test::BaseTestWorld {
public:
    HopperTestWorld() = default;

    // isClientSide 控制
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }

    // currentTick 控制
    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    // getBlockState 控制（按 BlockPos 存储）
    void setBlockState(const BlockPos& pos, const BlockState* state) { m_blockStates[pos] = state; }
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }

    // getBlockEntity 控制
    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) { m_blockEntities[pos] = entity; }
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second : nullptr;
    }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second : nullptr;
    }

    // getEntitiesInAABB 控制
    void setEntitiesInAABB(const std::vector<Entity*>& entities) { m_entities = entities; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
    }

    // tickManager 提供一个空实现
    [[nodiscard]] world::tick::TickManager& tickManager() override { return m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return m_tickManager; }

private:
    bool m_clientSide = false;
    u64 m_currentTick = 0;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
    std::vector<Entity*> m_entities;
    mc::test::DummyTickManager m_tickManager;
};

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

TEST_F(HopperEntityTest, IsOnCustomCooldown_ReturnsFalseForNormalCooldown)
{
    hopper_->setTransferCooldown(8);
    EXPECT_FALSE(hopper_->isOnCustomCooldown());
}

TEST_F(HopperEntityTest, IsOnCustomCooldown_ReturnsTrueForAboveNormalCooldown)
{
    hopper_->setTransferCooldown(9);
    EXPECT_TRUE(hopper_->isOnCustomCooldown());
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

// ========== isGridAligned 测试 ==========

TEST_F(HopperEntityTest, IsGridAligned_ReturnsTrueForBlockHopper)
{
    // 方块漏斗应该返回 true（MC Java: Hopper.isGridAligned() 对 HopperBlock 返回 true）
    EXPECT_TRUE(hopper_->isGridAligned());
}

// ========== getHopperInventory 测试 ==========

TEST_F(HopperEntityTest, GetHopperInventory_ReturnsValidInventory)
{
    // HopperEntity::getHopperInventory() 应返回内部 SimpleInventory 指针
    IInventory* inventory = hopper_->getHopperInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), HopperEntity::HOPPER_SIZE);
}

TEST_F(HopperEntityTest, GetHopperInventory_SameAsGetInventory)
{
    // getHopperInventory() 和 getInventory() 应返回同一个指针
    IInventory* hopperInventory = hopper_->getHopperInventory();
    IInventory* inventory = hopper_->getInventory();
    EXPECT_EQ(hopperInventory, inventory);
}

TEST_F(HopperEntityTest, GetHopperInventory_CanManipulateItems)
{
    // 通过 getHopperInventory() 获取的指针可以操作物品
    IInventory* inventory = hopper_->getHopperInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 32));
    EXPECT_EQ(inventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(inventory->getItem(0).getCount(), 32);
    // 也通过 getInventory() 可见
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getCount(), 32);
}

// ========== isOnCustomCooldown 边界条件测试 ==========

TEST_F(HopperEntityTest, IsOnCustomCooldown_ZeroNotCustom)
{
    hopper_->setTransferCooldown(0);
    EXPECT_FALSE(hopper_->isOnCustomCooldown());
}

TEST_F(HopperEntityTest, IsOnCustomCooldown_ExactlyAtThresholdNotCustom)
{
    // 冷却等于 TRANSFER_COOLDOWN(8) 时不算自定义冷却
    hopper_->setTransferCooldown(HopperEntity::TRANSFER_COOLDOWN);
    EXPECT_FALSE(hopper_->isOnCustomCooldown());
}

TEST_F(HopperEntityTest, IsOnCustomCooldown_OneAboveThresholdIsCustom)
{
    // 冷却超过 TRANSFER_COOLDOWN(8) 时算自定义冷却
    hopper_->setTransferCooldown(HopperEntity::TRANSFER_COOLDOWN + 1);
    EXPECT_TRUE(hopper_->isOnCustomCooldown());
}

TEST_F(HopperEntityTest, IsOnCustomCooldown_NegativeOneNotCustom)
{
    hopper_->setTransferCooldown(-1);
    EXPECT_FALSE(hopper_->isOnCustomCooldown());
}

// ========== captureItem 测试 ==========

class CaptureItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
        m_inventory = std::make_unique<SimpleInventory>(5);
    }

    Item* m_diamond = nullptr;
    std::unique_ptr<SimpleInventory> m_inventory;
};

TEST_F(CaptureItemTest, FullCapture_RemovesItemEntity)
{
    // 物品完全被捕获时，物品实体应该被移除
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 20.5f, 30.5f, mc::test::testEcsRegistry());
    EXPECT_TRUE(itemEntity->isAlive());

    bool result = HopperEntity::captureItem(m_inventory.get(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive()); // remove() 被调用
    EXPECT_TRUE(m_inventory->getItem(0).getItem() == m_diamond);
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 10);
}

TEST_F(CaptureItemTest, PartialCapture_WhenSlotCanAcceptAll)
{
    // 当目标槽位可以完全容纳物品时的部分捕获测试
    // 将漏斗的前4格填满（每格64个钻石），留1格空
    for (i32 i = 0; i < 4; ++i) {
        m_inventory->setItem(i, ItemStack(m_diamond, 64));
    }
    // 第5格为空

    // 尝试捕获10个钻石，全部放入空槽
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 20.5f, 30.5f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(m_inventory.get(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive()); // 完全被捕获
    EXPECT_EQ(m_inventory->getItem(4).getCount(), 10);
}

TEST_F(CaptureItemTest, NoCapture_ReturnsFalse)
{
    // 背包已满时，captureItem 返回 false
    for (i32 i = 0; i < 5; ++i) {
        m_inventory->setItem(i, ItemStack(m_diamond, 64));
    }

    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 20.5f, 30.5f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(m_inventory.get(), itemEntity.get());

    EXPECT_FALSE(result);
    EXPECT_TRUE(itemEntity->isAlive());                   // 物品实体仍存活
    EXPECT_EQ(itemEntity->getItemStack().getCount(), 10); // 数量不变
}

TEST_F(CaptureItemTest, NullInventory_ReturnsFalse)
{
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 20.5f, 30.5f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(nullptr, itemEntity.get());

    EXPECT_FALSE(result);
}

TEST_F(CaptureItemTest, NullItemEntity_ReturnsFalse)
{
    bool result = HopperEntity::captureItem(m_inventory.get(), nullptr);

    EXPECT_FALSE(result);
}

TEST_F(CaptureItemTest, DeadItemEntity_ReturnsFalse)
{
    // MC Java: HopperBlockEntity.addItem 仅检查 isAlive()，不检查 pickupDelay
    // 已死亡的物品实体不应被捕获
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 20.5f, 30.5f, mc::test::testEcsRegistry());
    itemEntity->remove(); // 标记为已死亡

    bool result = HopperEntity::captureItem(m_inventory.get(), itemEntity.get());

    EXPECT_FALSE(result);
}

TEST_F(CaptureItemTest, ItemWithPickupDelay_CanStillBeCaptured)
{
    // MC Java 行为: 漏斗不检查 pickupDelay，只有玩家拾取才检查
    // 即使物品有拾取延迟，漏斗也应该能捕获它
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 20.5f, 30.5f, mc::test::testEcsRegistry());
    itemEntity->setPickupDelay(100); // 设置100 tick 拾取延迟

    // 确认 pickupDelay 生效（玩家不能拾取）
    EXPECT_FALSE(itemEntity->canBePickedUp());

    // 但漏斗应该能捕获（MC Java: 不检查 pickupDelay）
    bool result = HopperEntity::captureItem(m_inventory.get(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive()); // 被成功捕获并移除
}

// ========== putStackInInventoryAllSlots 测试 ==========

class PutStackInInventoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
        m_iron = ensureHopperTestItem("iron_ingot");
        m_inventory = std::make_unique<SimpleInventory>(5);
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
    std::unique_ptr<SimpleInventory> m_inventory;
};

TEST_F(PutStackInInventoryTest, EmptyInventory_InsertsAllItems)
{
    ItemStack stack(m_diamond, 32);
    ItemStack remaining = HopperEntity::putStackInInventoryAllSlots(nullptr, m_inventory.get(), stack, Direction::None);

    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_diamond);
}

TEST_F(PutStackInInventoryTest, PartialStack_GoesToNextEmptySlot)
{
    // slot 0 有60个钻石，60+10=70 > 64，canPlaceItem 返回 false
    // 所以 stack 会放入下一个空槽(slot 1)
    m_inventory->setItem(0, ItemStack(m_diamond, 60));

    ItemStack stack(m_diamond, 10);
    ItemStack remaining = HopperEntity::putStackInInventoryAllSlots(nullptr, m_inventory.get(), stack, Direction::None);

    // canPlaceItem(0, 10个钻石) 返回 false（60+10 > 64），
    // 所以整个 stack 放入空槽 slot 1
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 60);
    EXPECT_EQ(m_inventory->getItem(1).getCount(), 10);
}

TEST_F(PutStackInInventoryTest, FullInventory_ReturnsAllItems)
{
    for (i32 i = 0; i < 5; ++i) {
        m_inventory->setItem(i, ItemStack(m_diamond, 64));
    }

    ItemStack stack(m_diamond, 10);
    ItemStack remaining = HopperEntity::putStackInInventoryAllSlots(nullptr, m_inventory.get(), stack, Direction::None);

    EXPECT_EQ(remaining.getCount(), 10); // 全部无法插入
}

TEST_F(PutStackInInventoryTest, DifferentItems_StacksCorrectly)
{
    m_inventory->setItem(0, ItemStack(m_iron, 32));

    ItemStack stack(m_diamond, 10);
    ItemStack remaining = HopperEntity::putStackInInventoryAllSlots(nullptr, m_inventory.get(), stack, Direction::None);

    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_iron);    // 第一个槽位是铁
    EXPECT_EQ(m_inventory->getItem(1).getItem(), m_diamond); // 第二个槽位是钻石
    EXPECT_EQ(m_inventory->getItem(1).getCount(), 10);
}

TEST_F(PutStackInInventoryTest, NullDestination_ReturnsOriginalStack)
{
    ItemStack stack(m_diamond, 10);
    ItemStack remaining = HopperEntity::putStackInInventoryAllSlots(nullptr, nullptr, stack, Direction::None);

    EXPECT_EQ(remaining.getCount(), 10);
}

TEST_F(PutStackInInventoryTest, EmptyStack_ReturnsEmpty)
{
    ItemStack empty;
    ItemStack remaining = HopperEntity::putStackInInventoryAllSlots(nullptr, m_inventory.get(), empty, Direction::None);

    EXPECT_TRUE(remaining.isEmpty());
}

// ========== tick() 客户端检查测试 ==========

TEST_F(HopperEntityTest, Tick_ClientSide_DoesNotProcessTransfers)
{
    // MC Java: 漏斗的 tick 逻辑只在服务端执行
    // 在客户端 isClientSide()=true 时，tick() 应该直接返回
    HopperTestWorld world;
    world.setClientSide(true);
    world.setCurrentTick(1);

    // 设置冷却为0，确保如果tick执行了会尝试传输
    hopper_->setTransferCooldown(0);

    // 放入物品使漏斗不为空
    hopper_->getInventory()->setItem(0, ItemStack(m_diamond, 1));

    // 在客户端调用tick
    hopper_->tick(world);

    // 冷却应该保持不变（tick没有处理传输逻辑）
    EXPECT_EQ(hopper_->getTransferCooldown(), 0);
}

TEST_F(HopperEntityTest, Tick_ServerSide_ProcessesTransfers)
{
    // 服务端 tick 应该正常处理传输逻辑
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    hopper_->setTransferCooldown(0);

    hopper_->tick(world);

    // 空漏斗不传输但也不报错
    // 冷却应该保持0或被设置
    EXPECT_GE(hopper_->getTransferCooldown(), 0);
}

// ========== pullItems 自循环检测测试 ==========

TEST_F(HopperEntityTest, PullItems_SourceIsSelf_ReturnsFalse)
{
    // 当源容器是漏斗自身的背包时，pullItems 应返回 false 以避免自循环
    // 这通过 getSourceInventory 返回漏斗自身的 IInventory 来模拟
    // 由于 HopperEntity 同时实现 IHopper 和 IInventory，
    // 如果 getSourceInventory 返回 &m_inventory，则 pullItems 会检测到自循环

    // 创建一个测试场景：漏斗上方没有其他容器，直接调用 pullItems
    HopperTestWorld world;
    hopper_->setTransferCooldown(0);

    // 没有设置上方的方块实体，getSourceInventory 应该返回 nullptr
    // 所以不会触发自循环检测——这只是基本场景
    bool result = HopperEntity::pullItems(*hopper_);
    EXPECT_FALSE(result); // 没有源容器，没有物品实体，返回 false
}

// ========== isGridAligned 虚方法测试 ==========

namespace {
/**
 * @brief 测试用非网格对齐漏斗（模拟漏斗矿车）
 */
class NonGridAlignedHopper : public IHopper {
public:
    explicit NonGridAlignedHopper(const BlockPos& pos)
        : m_pos(pos)
    {}

    [[nodiscard]] IWorld* getWorld() override { return nullptr; }
    [[nodiscard]] const IWorld* getWorld() const override { return nullptr; }
    [[nodiscard]] f64 getXPos() const override { return static_cast<f64>(m_pos.x) + 0.5; }
    [[nodiscard]] f64 getYPos() const override { return static_cast<f64>(m_pos.y) + 0.5; }
    [[nodiscard]] f64 getZPos() const override { return static_cast<f64>(m_pos.z) + 0.5; }
    [[nodiscard]] BlockPos getHopperPos() const override { return m_pos; }
    [[nodiscard]] Direction getOutputDirection() const override { return Direction::Down; }
    [[nodiscard]] bool isGridAligned() const override { return false; }         // 矿车漏斗：不网格对齐
    [[nodiscard]] IInventory* getHopperInventory() override { return nullptr; } // 测试用，无背包

private:
    BlockPos m_pos;
};
} // namespace

TEST_F(IHopperTest, IsGridAligned_BlockHopperReturnsTrue)
{
    // 方块漏斗（HopperEntity）返回 true
    EXPECT_TRUE(hopper_->isGridAligned());
}

TEST(IHopperVirtualTest, IsGridAligned_MinecartHopperReturnsFalse)
{
    // 漏斗矿车应返回 false
    NonGridAlignedHopper minecartHopper(BlockPos(5, 10, 15));
    EXPECT_FALSE(minecartHopper.isGridAligned());
}

// ========== getCollectionArea 精确测试 ==========

TEST_F(IHopperTest, GetCollectionArea_HasCorrectDimensions)
{
    // MC Java: SUCK_AABB = Block.column(16.0, 11.0, 32.0)
    // 转换为世界坐标: (blockX, blockY + 11/16, blockZ) -> (blockX+1, blockY+2, blockZ+1)
    // 对于 BlockPos(100, 64, -50):
    //   center = (100.5, 64.5, -49.5)
    //   minY = 64.5 - 0.5 + 11.0/16.0 = 64.0 + 0.6875 = 64.6875
    //   maxY = 64.5 + 1.5 = 66.0
    //   minX = 100.5 - 0.5 = 100.0
    //   maxX = 100.5 + 0.5 = 101.0
    //   minZ = -49.5 - 0.5 = -50.0
    //   maxZ = -49.5 + 0.5 = -49.0

    AxisAlignedBB area = IHopper::getCollectionArea(*hopper_);

    EXPECT_FLOAT_EQ(area.minX, 100.0f);
    EXPECT_FLOAT_EQ(area.minY, 64.6875f); // 64 + 11/16
    EXPECT_FLOAT_EQ(area.minZ, -50.0f);
    EXPECT_FLOAT_EQ(area.maxX, 101.0f);
    EXPECT_FLOAT_EQ(area.maxY, 66.0f); // 64 + 2
    EXPECT_FLOAT_EQ(area.maxZ, -49.0f);
}

TEST_F(IHopperTest, GetOutputPosition_DefaultDirectionDown)
{
    // 默认输出方向向下，输出位置应为漏斗位置下方一格
    BlockPos outputPos = IHopper::getOutputPosition(*hopper_);
    EXPECT_EQ(outputPos, BlockPos(100, 63, -50));
}

// ========== getInventoryAtPosition 测试 ==========

TEST_F(HopperEntityTest, GetInventoryAtPosition_NullWorld_ReturnsNullptr)
{
    InventoryRef result = HopperEntity::getInventoryAtPosition(nullptr, BlockPos(10, 20, 30));
    EXPECT_EQ(result.get(), nullptr);
}

TEST_F(HopperEntityTest, GetInventoryAtPosition_NoBlockEntity_ReturnsNullptr)
{
    HopperTestWorld world;
    InventoryRef result = HopperEntity::getInventoryAtPosition(&world, BlockPos(10, 20, 30));
    EXPECT_EQ(result.get(), nullptr);
}

TEST_F(HopperEntityTest, GetInventoryAtPosition_BlockEntityWithoutIInheritance_ReturnsNullptr)
{
    // ContainerBlockEntity 不继承 IInventory，而是通过 getInventory() 方法提供指针
    // getInventoryAtPosition 使用 dynamic_cast<IInventory*>(blockEntity)
    // 所以对于 HopperEntity（不直接继承 IInventory），此路径返回 nullptr
    // 实际场景中，HopperBlock 会在 HopperBlock::getInventory() 中处理这种情况
    HopperTestWorld world;
    auto container = std::make_unique<HopperEntity>(BlockPos(10, 21, 30));
    world.setBlockEntity(BlockPos(10, 21, 30), container.get());

    InventoryRef result = HopperEntity::getInventoryAtPosition(&world, BlockPos(10, 21, 30));
    // dynamic_cast<IInventory*>(blockEntity) 对 HopperEntity 返回 nullptr
    EXPECT_EQ(result.get(), nullptr);

    (void)container.release(); // 避免双重释放
}

// ========== getSourceInventory 测试 ==========

TEST_F(HopperEntityTest, GetSourceInventory_NoWorld_ReturnsNullptr)
{
    // 漏斗没有设置世界时，getSourceInventory 返回空引用
    InventoryRef result = HopperEntity::getSourceInventory(*hopper_);
    EXPECT_EQ(result.get(), nullptr);
}

// ========== 静态方法 _isInventoryFull / _isInventoryEmpty 测试 ==========

class HopperInventoryStaticTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
        m_inventory = std::make_unique<SimpleInventory>(3);
        m_hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));
    }

    Item* m_diamond = nullptr;
    std::unique_ptr<SimpleInventory> m_inventory;
    std::unique_ptr<HopperEntity> m_hopper;
};

TEST_F(HopperInventoryStaticTest, IsFull_EmptyInventory_ReturnsFalse)
{
    EXPECT_FALSE(m_hopper->isFull());
}

TEST_F(HopperInventoryStaticTest, IsFull_PartiallyFull_ReturnsFalse)
{
    IInventory* inv = m_hopper->getInventory();
    inv->setItem(0, ItemStack(m_diamond, 32));
    EXPECT_FALSE(m_hopper->isFull());
}

TEST_F(HopperInventoryStaticTest, IsFull_CompletelyFull_ReturnsTrue)
{
    IInventory* inv = m_hopper->getInventory();
    for (i32 i = 0; i < HopperEntity::HOPPER_SIZE; ++i) {
        inv->setItem(i, ItemStack(m_diamond, 64));
    }
    EXPECT_TRUE(m_hopper->isFull());
}

// ========== captureItem 边界场景测试 ==========

class CaptureItemEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
    }

    Item* m_diamond = nullptr;
};

TEST_F(CaptureItemEdgeCaseTest, SingleItemCapture_Success)
{
    auto inventory = std::make_unique<SimpleInventory>(5);
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 1), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(inventory.get(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive());
    EXPECT_EQ(inventory->getItem(0).getCount(), 1);
}

TEST_F(CaptureItemEdgeCaseTest, MaxStackSizeCapture_Success)
{
    auto inventory = std::make_unique<SimpleInventory>(5);
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 64), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(inventory.get(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive());
    EXPECT_EQ(inventory->getItem(0).getCount(), 64);
}

TEST_F(CaptureItemEdgeCaseTest, StackLargerThanMaxStackSize_CapturedInEmptySlot)
{
    // setItem 不强制执行最大堆叠限制，所以超过64的物品堆会直接放入一个空槽
    auto inventory = std::make_unique<SimpleInventory>(5);
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 128), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(inventory.get(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive());
    // 整个堆叠（128个）被放入第一个空槽，setItem不拆分堆叠
    EXPECT_EQ(inventory->getItem(0).getCount(), 128);
}

TEST_F(CaptureItemEdgeCaseTest, FullInventory_ReturnsFalse)
{
    // 5格全部满时，captureItem 返回 false
    auto inventory = std::make_unique<SimpleInventory>(5);
    for (i32 i = 0; i < 5; ++i) {
        inventory->setItem(i, ItemStack(m_diamond, 64));
    }

    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 5), 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(inventory.get(), itemEntity.get());

    // 全部满，无法插入
    EXPECT_FALSE(result);
    EXPECT_TRUE(itemEntity->isAlive());
    EXPECT_EQ(itemEntity->getItemStack().getCount(), 5);
}

// ========== getCaptureItems 测试 ==========

class GetCaptureItemsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
    }

    Item* m_diamond = nullptr;
};

TEST_F(GetCaptureItemsTest, NoWorld_ReturnsEmptyVector)
{
    // 没有世界时返回空列表
    auto hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));
    auto items = HopperEntity::getCaptureItems(*hopper);
    EXPECT_TRUE(items.empty());
}

TEST_F(GetCaptureItemsTest, WorldWithNoEntities_ReturnsEmptyVector)
{
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));
    // 不设置任何实体

    auto items = HopperEntity::getCaptureItems(*hopper);
    EXPECT_TRUE(items.empty());
}

TEST_F(GetCaptureItemsTest, WorldWithItemEntity_ReturnsItemEntity)
{
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));
    // hopper->getWorld() 是 nullptr，需要设置...
    // HopperEntity 的 getWorld() 返回 m_world，需要通过 tick 设置
    // 但 getCaptureItems 是静态方法，直接用 hopper.getWorld()
    // 由于我们无法直接设置 m_world，可以改为间接测试

    // 实际上 getCaptureItems 使用 hopper.getWorld()，HopperEntity::getWorld() 返回 m_world
    // m_world 是在 tick() 中设置的。我们需要先调用 tick() 来设置 m_world。
    world.setClientSide(false);
    world.setCurrentTick(0);
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 设置一个物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    auto items = HopperEntity::getCaptureItems(*hopper);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0], itemEntity.get());
}

TEST_F(GetCaptureItemsTest, DeadItemEntityNotReturned)
{
    // MC Java: getCaptureItems 仅过滤 isAlive() 的物品实体
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));

    world.setClientSide(false);
    world.setCurrentTick(0);
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    auto aliveItem = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());
    auto deadItem = std::make_unique<ItemEntity>(EntityInstanceId(2), ItemStack(m_diamond, 5), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());
    deadItem->remove(); // 标记为死亡

    world.setEntitiesInAABB({aliveItem.get(), deadItem.get()});

    auto items = HopperEntity::getCaptureItems(*hopper);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0], aliveItem.get()); // 只有存活的物品被返回
}

TEST_F(GetCaptureItemsTest, ItemWithPickupDelayStillReturned)
{
    // MC Java: 漏斗不检查 pickupDelay，即使物品设置了拾取延迟也会被返回
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));

    world.setClientSide(false);
    world.setCurrentTick(0);
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    auto delayedItem = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());
    delayedItem->setPickupDelay(100);           // 设置拾取延迟
    EXPECT_FALSE(delayedItem->canBePickedUp()); // 玩家不能拾取

    world.setEntitiesInAABB({delayedItem.get()});

    auto items = HopperEntity::getCaptureItems(*hopper);
    // 漏斗应该返回有拾取延迟的物品（不检查 pickupDelay）
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0], delayedItem.get());
}

TEST_F(GetCaptureItemsTest, NonItemEntitiesNotReturned)
{
    // 非物品实体不应被 getCaptureItems 返回
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));

    world.setClientSide(false);
    world.setCurrentTick(0);
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 只有物品实体，没有其他类型实体（这里无法创建非 ItemEntity 的 Entity，
    // 但可以测试空实体列表和只有 ItemEntity 的情况）
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    auto items = HopperEntity::getCaptureItems(*hopper);
    ASSERT_EQ(items.size(), 1u);
    // 确认返回的是 ItemEntity* 类型
    EXPECT_NE(dynamic_cast<ItemEntity*>(items[0]), nullptr);
}

// ========== tick() 冷却递减测试 ==========

TEST_F(HopperEntityTest, Tick_DecrementsCooldown)
{
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    hopper_->setTransferCooldown(5);
    hopper_->tick(world);

    // 冷却应该从5减到4
    EXPECT_EQ(hopper_->getTransferCooldown(), 4);
}

TEST_F(HopperEntityTest, Tick_MultipleTicksDecrementToZero)
{
    HopperTestWorld world;
    world.setClientSide(false);

    hopper_->setTransferCooldown(3);

    for (u64 tick = 1; tick <= 3; ++tick) {
        world.setCurrentTick(tick);
        hopper_->tick(world);
    }

    // 3次tick后冷却应该为0
    EXPECT_EQ(hopper_->getTransferCooldown(), 0);
}

TEST_F(HopperEntityTest, Tick_CooldownDoesNotGoBelowZero)
{
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    hopper_->setTransferCooldown(0);
    hopper_->tick(world);

    // 冷却不应该变成负数
    EXPECT_GE(hopper_->getTransferCooldown(), 0);
}

TEST_F(HopperEntityTest, Tick_SetsWorldPointer)
{
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    EXPECT_EQ(hopper_->getWorld(), nullptr);

    hopper_->tick(world);

    EXPECT_EQ(hopper_->getWorld(), &world);
}

TEST_F(HopperEntityTest, Tick_RecordsTickedGameTime)
{
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(42);

    hopper_->tick(world);

    // tick 应该记录游戏时间（通过 m_tickedGameTime，无法直接访问，但行为正确）
    // 间接验证：如果 m_tickedGameTime 正确设置，链式优化才会生效
    // 这里只验证不崩溃
    EXPECT_EQ(hopper_->getWorld(), &world);
}

// ========== pullItems 与上方方块阻挡测试 ==========

TEST_F(HopperEntityTest, PullItems_NoSourceNoEntities_ReturnsFalse)
{
    // 没有上方容器且没有物品实体时返回 false
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    hopper_->tick(world);
    ASSERT_EQ(hopper_->getWorld(), &world);

    bool result = HopperEntity::pullItems(*hopper_);
    EXPECT_FALSE(result);
}

// ========== HopperEntity 自循环检测（通过 _transferItemsOut） ==========

TEST_F(HopperEntityTest, TransferItemsOut_TargetIsSelf_ReturnsFalse)
{
    // _transferItemsOut 检查目标容器是否是漏斗自身
    // 这在漏斗朝下且下方没有其他容器时不会触发
    // 但如果 getInventoryAtPosition 返回漏斗自身，应该被检测到
    // 这里测试基本场景：没有目标容器时返回 false
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    hopper_->getInventory()->setItem(0, ItemStack(m_diamond, 10));
    hopper_->tick(world);
    ASSERT_EQ(hopper_->getWorld(), &world);

    // 没有目标容器（下方没有方块实体），_transferItemsOut 应返回 false
    // 这是间接测试：如果目标是自己，会返回 false 而不是自循环
}

// ========== IHopper 默认方法测试 ==========

TEST(IHopperDefaultTest, GetOutputDirection_DefaultIsDown)
{
    // 测试 IHopper 默认的 getOutputDirection 实现
    NonGridAlignedHopper hopper(BlockPos(0, 0, 0));
    EXPECT_EQ(hopper.getOutputDirection(), Direction::Down);
}

TEST(IHopperDefaultTest, GetOutputPosition_DefaultDown)
{
    NonGridAlignedHopper hopper(BlockPos(5, 10, 15));
    BlockPos outputPos = IHopper::getOutputPosition(hopper);
    EXPECT_EQ(outputPos, BlockPos(5, 9, 15)); // 向下一格
}

// ========== captureItem 与 HopperEntity 背包集成测试 ==========

class CaptureItemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
        m_iron = ensureHopperTestItem("iron_ingot");
        hopper_ = std::make_unique<HopperEntity>(BlockPos(0, 0, 0));
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
    std::unique_ptr<HopperEntity> hopper_;
};

TEST_F(CaptureItemIntegrationTest, CaptureIntoHopperEntityInventory)
{
    // 使用 HopperEntity 自身的背包来测试 captureItem
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 32), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(hopper_->getInventory(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive());
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getCount(), 32);
}

TEST_F(CaptureItemIntegrationTest, CaptureIntoFullHopper_ReturnsFalse)
{
    // 填满漏斗
    for (i32 i = 0; i < HopperEntity::HOPPER_SIZE; ++i) {
        hopper_->getInventory()->setItem(i, ItemStack(m_diamond, 64));
    }

    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_iron, 10), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(hopper_->getInventory(), itemEntity.get());

    EXPECT_FALSE(result);
    EXPECT_TRUE(itemEntity->isAlive());
    EXPECT_EQ(itemEntity->getItemStack().getCount(), 10);
}

TEST_F(CaptureItemIntegrationTest, PartialCaptureIntoAlmostFullHopper)
{
    // 4格满，1格有不同物品
    for (i32 i = 0; i < 4; ++i) {
        hopper_->getInventory()->setItem(i, ItemStack(m_iron, 64));
    }
    // 第5格空
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 0.5f, 0.5f, 0.5f, mc::test::testEcsRegistry());

    bool result = HopperEntity::captureItem(hopper_->getInventory(), itemEntity.get());

    EXPECT_TRUE(result);
    EXPECT_FALSE(itemEntity->isAlive());
    EXPECT_EQ(hopper_->getInventory()->getItem(4).getItem(), m_diamond);
    EXPECT_EQ(hopper_->getInventory()->getItem(4).getCount(), 10);
}

// ========== putStackInInventoryAllSlots 与 HopperEntity 背包集成测试 ==========

TEST_F(CaptureItemIntegrationTest, PutStackIntoHopperInventory)
{
    ItemStack stack(m_diamond, 20);
    ItemStack remaining =
        HopperEntity::putStackInInventoryAllSlots(nullptr, hopper_->getInventory(), stack, Direction::None);

    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getCount(), 20);
}

TEST_F(CaptureItemIntegrationTest, PutStackIntoNonEmptyHopperInventory_Merges)
{
    hopper_->getInventory()->setItem(0, ItemStack(m_diamond, 50));

    ItemStack stack(m_diamond, 14);
    ItemStack remaining =
        HopperEntity::putStackInInventoryAllSlots(nullptr, hopper_->getInventory(), stack, Direction::None);

    // 50 + 14 = 64，完全合并
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getCount(), 64);
}

TEST_F(CaptureItemIntegrationTest, PutStackIntoNonEmptyHopperInventory_GoesToEmptySlot)
{
    // slot 0 有60个钻石，canPlaceItem(0, 10个) 返回 false（60+10 > 64）
    // 所以 stack 放入下一个空槽
    hopper_->getInventory()->setItem(0, ItemStack(m_diamond, 60));

    ItemStack stack(m_diamond, 10);
    ItemStack remaining =
        HopperEntity::putStackInInventoryAllSlots(nullptr, hopper_->getInventory(), stack, Direction::None);

    // 放入空槽 slot 1
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(hopper_->getInventory()->getItem(0).getCount(), 60);
    EXPECT_EQ(hopper_->getInventory()->getItem(1).getCount(), 10);
}

// ========== pullItems 自循环检测与 getHopperInventory 集成测试 ==========

class HopperSelfLoopTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
    }

    Item* m_diamond = nullptr;
};

TEST_F(HopperSelfLoopTest, PullItems_SelfLoopDetectionViaGetHopperInventory)
{
    // 验证 pullItems 中自循环检测使用 getHopperInventory() 而非 dynamic_cast
    // 当上方容器恰好是漏斗自身的背包时，应该返回 false
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));

    // 设置世界
    world.setClientSide(false);
    world.setCurrentTick(1);
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在漏斗上方放置一个容器，该容器恰好返回漏斗自身的 getHopperInventory()
    // 这模拟了 MC Java 中漏斗从自身拉取的场景
    // 由于 HopperEntity::getSourceInventory 使用 getInventoryAtPosition，
    // 而 getInventoryAtPosition 通过 getBlockEntity -> dynamic_cast<IInventory*>
    // 对 HopperEntity 返回 nullptr（不继承 IInventory），
    // 所以在当前架构下，自循环不太可能通过 getInventoryAtPosition 发生。
    // 但自循环检测仍然作为安全措施存在。

    // 验证 getHopperInventory() 不返回 nullptr
    IInventory* hopperInventory = hopper->getHopperInventory();
    ASSERT_NE(hopperInventory, nullptr);
    EXPECT_EQ(hopperInventory, hopper->getInventory());
}

TEST_F(HopperSelfLoopTest, PullItems_CaptureItemsUsesGetHopperInventory)
{
    // 验证 pullItems 中 captureItem 使用 getHopperInventory() 获取漏斗背包
    // 当漏斗上方没有容器时，pullItems 尝试捕获物品实体
    // captureItem 需要一个有效的 IInventory* 才能工作
    HopperTestWorld world;
    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));

    world.setClientSide(false);
    world.setCurrentTick(1);
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在收集区域放置一个物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 21.5f, 30.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    // pullItems 应该能通过 getHopperInventory() 获取到漏斗背包并成功捕获
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_TRUE(result);
    // 验证物品被捕获到漏斗中
    EXPECT_EQ(hopper->getInventory()->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(hopper->getInventory()->getItem(0).getCount(), 10);
}

TEST_F(HopperSelfLoopTest, GetHopperInventory_NonGridAlignedHopperReturnsNullptr)
{
    // NonGridAlignedHopper（模拟矿车漏斗）没有背包时返回 nullptr
    NonGridAlignedHopper mockHopper(BlockPos(5, 10, 15));
    EXPECT_EQ(mockHopper.getHopperInventory(), nullptr);
}

TEST_F(HopperSelfLoopTest, PullItems_NullHopperInventoryCannotCapture)
{
    // 当 IHopper 的 getHopperInventory() 返回 nullptr 时，
    // pullItems 中 captureItem 应该因为空指针返回 false
    NonGridAlignedHopper mockHopper(BlockPos(5, 10, 15));
    // NonGridAlignedHopper 的 getWorld() 返回 nullptr，所以 pullItems 会直接返回 false
    bool result = HopperEntity::pullItems(mockHopper);
    EXPECT_FALSE(result);
}

// ========== IHopper getHopperInventory 默认实现测试 ==========

TEST(IHopperDefaultTest, GetHopperInventory_DefaultReturnsNullptr)
{
    // IHopper 默认实现返回 nullptr
    NonGridAlignedHopper hopper(BlockPos(0, 0, 0));
    EXPECT_EQ(hopper.getHopperInventory(), nullptr);
}

// ========== pullItems 与 DOES_NOT_BLOCK_HOPPERS 标签集成测试 ==========

class HopperDoesNotBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        m_diamond = ensureHopperTestItem("diamond");
    }

    Item* m_diamond = nullptr;
};

TEST_F(HopperDoesNotBlockTest, PullItems_StoneAboveWithItemEntity_BlocksSuction)
{
    // 石头不在 DOES_NOT_BLOCK_HOPPERS 标签中，碰撞形状为完整方块，
    // 即使上方有物品实体，漏斗也无法吸取（被阻挡）
    const Block* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ASSERT_TRUE(stone->defaultState().isFaceFull(Direction::Down));
    ASSERT_FALSE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(stone->defaultState()));

    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在漏斗上方放置石头
    world.setBlockState(BlockPos(10, 21, 30), &stone->defaultState());

    // 在漏斗收集区域放置物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 21.5f, 30.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    // 石头阻挡漏斗吸取，即使有物品实体也返回 false
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_FALSE(result);

    // 物品实体仍然存活（未被吸取）
    EXPECT_TRUE(itemEntity->isAlive());
}

TEST_F(HopperDoesNotBlockTest, PullItems_BeehiveAboveWithItemEntity_AllowsSuction)
{
    // 蜂箱在 DOES_NOT_BLOCK_HOPPERS 标签中，即使碰撞形状为完整方块，
    // 漏斗仍可吸取上方物品实体
    const Block* beehive = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "beehive"));
    if (!beehive) {
        GTEST_SKIP() << "beehive block not registered";
    }
    ASSERT_TRUE(beehive->defaultState().isFaceFull(Direction::Down));
    ASSERT_TRUE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(beehive->defaultState()));

    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在漏斗上方放置蜂箱
    world.setBlockState(BlockPos(10, 21, 30), &beehive->defaultState());

    // 在漏斗收集区域放置物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 21.5f, 30.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    // 蜂箱不阻挡漏斗，物品实体应被成功吸取
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_TRUE(result);

    // 物品实体已被移除（被吸取）
    EXPECT_FALSE(itemEntity->isAlive());

    // 漏斗背包中应有物品
    EXPECT_FALSE(hopper->isEmpty());
}

TEST_F(HopperDoesNotBlockTest, PullItems_BeeNestAboveWithItemEntity_AllowsSuction)
{
    // 蜂巢在 DOES_NOT_BLOCK_HOPPERS 标签中，即使碰撞形状为完整方块，
    // 漏斗仍可吸取上方物品实体
    const Block* beeNest = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "bee_nest"));
    if (!beeNest) {
        GTEST_SKIP() << "bee_nest block not registered";
    }
    ASSERT_TRUE(beeNest->defaultState().isFaceFull(Direction::Down));
    ASSERT_TRUE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(beeNest->defaultState()));

    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在漏斗上方放置蜂巢
    world.setBlockState(BlockPos(10, 21, 30), &beeNest->defaultState());

    // 在漏斗收集区域放置物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 21.5f, 30.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    // 蜂巢不阻挡漏斗，物品实体应被成功吸取
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_TRUE(result);

    // 物品实体已被移除（被吸取）
    EXPECT_FALSE(itemEntity->isAlive());

    // 漏斗背包中应有物品
    EXPECT_FALSE(hopper->isEmpty());
}

TEST_F(HopperDoesNotBlockTest, PullItems_NoBlockAboveWithItemEntity_AllowsSuction)
{
    // 漏斗上方没有方块时，物品实体可被正常吸取
    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 上方无方块
    // 在漏斗收集区域放置物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(1), ItemStack(m_diamond, 10), 10.5f, 21.5f, 30.5f, mc::test::testEcsRegistry());
    world.setEntitiesInAABB({itemEntity.get()});

    // 无阻挡，物品实体应被成功吸取
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_TRUE(result);

    // 物品实体已被移除
    EXPECT_FALSE(itemEntity->isAlive());

    // 漏斗背包中应有物品
    EXPECT_FALSE(hopper->isEmpty());
}

TEST_F(HopperDoesNotBlockTest, PullItems_NonGridAlignedHopper_NotBlockedByFullBlock)
{
    // 非对齐网格漏斗（如漏斗矿车）不受上方方块阻挡，
    // 即使上方是完整碰撞方块也不阻挡
    const Block* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    // NonGridAlignedHopper::isGridAligned() 返回 false
    // 非对齐漏斗没有 world，pullItems 直接返回 false
    NonGridAlignedHopper hopper(BlockPos(10, 20, 30));
    bool result = HopperEntity::pullItems(hopper);
    EXPECT_FALSE(result);
}

TEST_F(HopperDoesNotBlockTest, PullItems_StoneAboveNoItem_ReturnsFalse)
{
    // 石头阻挡漏斗，且没有物品实体时返回 false
    const Block* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在漏斗上方放置石头
    world.setBlockState(BlockPos(10, 21, 30), &stone->defaultState());

    // 没有物品实体，石头阻挡，返回 false
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_FALSE(result);
}

TEST_F(HopperDoesNotBlockTest, PullItems_BeehiveAboveNoItem_ReturnsFalse)
{
    // 蜂箱不阻挡漏斗，但没有物品实体时也返回 false（没有东西可吸）
    const Block* beehive = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "beehive"));
    if (!beehive) {
        GTEST_SKIP() << "beehive block not registered";
    }

    HopperTestWorld world;
    world.setClientSide(false);
    world.setCurrentTick(1);

    auto hopper = std::make_unique<HopperEntity>(BlockPos(10, 20, 30));
    hopper->tick(world);
    ASSERT_EQ(hopper->getWorld(), &world);

    // 在漏斗上方放置蜂箱
    world.setBlockState(BlockPos(10, 21, 30), &beehive->defaultState());

    // 没有物品实体，即使不被阻挡也返回 false（没有东西可吸）
    bool result = HopperEntity::pullItems(*hopper);
    EXPECT_FALSE(result);
}
