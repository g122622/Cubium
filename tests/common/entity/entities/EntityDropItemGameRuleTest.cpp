/**
 * @file EntityDropItemGameRuleTest.cpp
 * @brief 测试实体 dropItem 方法受 DO_ENTITY_DROPS 游戏规则控制的行为
 *
 * 测试覆盖：
 * 1. BoatEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 2. AbstractMinecartEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 3. ChestMinecartEntity::dropItem 容器内容物受 DO_ENTITY_DROPS 控制
 * 4. HopperMinecartEntity::dropItem 容器内容物受 DO_ENTITY_DROPS 控制
 * 5. PaintingEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 6. ItemFrameEntity::dropItem 受 DO_ENTITY_DROPS 控制（展示框 + 内容物）
 * 7. LeashKnotEntity::dropItem 受 DO_ENTITY_DROPS 控制
 *
 * 参考 MC 1.21.11：VehicleEntity.destroy()、ContainerEntity.chestVehicleDestroyed()、
 * Painting.dropItem()、ItemFrame.dropItem()、Leashable.tickLeash() 中的 ENTITY_DROPS 检查
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace test {

// ============================================================================
// 测试用 Mock World
// ============================================================================

/**
 * @brief 用于实体掉落测试的 Mock World
 *
 * 捕获 spawnEntity 调用，提供可控的 GameRules。
 */
class EntityDropTestWorld final : public BaseTestWorld {
public:
    EntityDropTestWorld()
    {
        // DO_ENTITY_DROPS 默认为 true
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        if (box.minY <= 0) {
            return true;
        }
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        std::vector<AxisAlignedBB> collisions;
        if (box.minY <= 0) {
            collisions.push_back(AxisAlignedBB(-1000.0, -1000.0, -1000.0, 1000.0, 0.0, 1000.0));
        }
        return collisions;
    }

    // ========== 实体管理 ==========

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    // ========== 游戏规则 ==========

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // ========== 其他必需方法 ==========

    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }
    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EntityDropTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EntityDropTestWorld::tickManager not implemented");
    }

    // ========== 测试辅助 ==========

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    Entity* getLastSpawnedEntity()
    {
        if (m_spawnedEntities.empty()) {
            return nullptr;
        }
        return m_spawnedEntities.back().get();
    }

    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    world::gamerule::GameRules m_gameRules;
    bool m_isClientSide = false;
};

// ============================================================================
// 测试固件
// ============================================================================

class EntityDropItemGameRuleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }

    EntityDropTestWorld m_world;
};

// ============================================================================
// BoatEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试 BoatEntity::dropItem 在 DO_ENTITY_DROPS=true 时生成物品实体
 */
TEST_F(EntityDropItemGameRuleTest, Boat_DropItem_WhenEntityDropsEnabled_SpawnsItem)
{
    // DO_ENTITY_DROPS 默认为 true
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::BoatEntity boat(entity::BoatEntity::Type::OAK);
    boat.setWorld(&m_world);
    boat.setPosition(0.0, 0.0, 0.0);

    boat.dropItem();

    // 应该生成1个物品实体（船物品）
    EXPECT_EQ(m_world.spawnedEntityCount(), 1u)
        << "BoatEntity::dropItem should spawn 1 item entity when DO_ENTITY_DROPS is true";
}

/**
 * @brief 测试 BoatEntity::dropItem 在 DO_ENTITY_DROPS=false 时不生成物品实体
 */
TEST_F(EntityDropItemGameRuleTest, Boat_DropItem_WhenEntityDropsDisabled_NoItemSpawned)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    ASSERT_FALSE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::BoatEntity boat(entity::BoatEntity::Type::OAK);
    boat.setWorld(&m_world);
    boat.setPosition(0.0, 0.0, 0.0);

    boat.dropItem();

    // 不应该生成任何物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "BoatEntity::dropItem should not spawn any item entity when DO_ENTITY_DROPS is false";
}

/**
 * @brief 测试 BoatEntity::dropItem 在客户端世界不生成物品
 */
TEST_F(EntityDropItemGameRuleTest, Boat_DropItem_OnClientSide_NoItemSpawned)
{
    m_world.setClientSide(true);

    entity::BoatEntity boat(entity::BoatEntity::Type::OAK);
    boat.setWorld(&m_world);
    boat.setPosition(0.0, 0.0, 0.0);

    boat.dropItem();

    EXPECT_EQ(m_world.spawnedEntityCount(), 0u) << "BoatEntity::dropItem should not spawn items on client side";
}

// ============================================================================
// AbstractMinecartEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试普通矿车在 DO_ENTITY_DROPS=true 时生成物品实体
 */
TEST_F(EntityDropItemGameRuleTest, RideableMinecart_DropItem_WhenEntityDropsEnabled_SpawnsItem)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::RideableMinecartEntity minecart(EntityInstanceId(1));
    minecart.setWorld(&m_world);
    minecart.setPosition(0.0, 0.0, 0.0);

    minecart.dropItem(nullptr);

    // 应该生成1个物品实体（矿车物品）
    EXPECT_EQ(m_world.spawnedEntityCount(), 1u)
        << "RideableMinecartEntity::dropItem should spawn 1 item entity when DO_ENTITY_DROPS is true";
}

/**
 * @brief 测试普通矿车在 DO_ENTITY_DROPS=false 时不生成物品实体且被标记移除
 */
TEST_F(EntityDropItemGameRuleTest, RideableMinecart_DropItem_WhenEntityDropsDisabled_NoItemAndRemoved)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::RideableMinecartEntity minecart(EntityInstanceId(1));
    minecart.setWorld(&m_world);
    minecart.setPosition(0.0, 0.0, 0.0);

    minecart.dropItem(nullptr);

    // 不应该生成物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "RideableMinecartEntity::dropItem should not spawn items when DO_ENTITY_DROPS is false";
    // 矿车应该被标记为移除
    EXPECT_TRUE(minecart.isRemoved()) << "RideableMinecartEntity should be removed when DO_ENTITY_DROPS is false";
}

// ============================================================================
// ChestMinecartEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试箱子矿车在 DO_ENTITY_DROPS=true 时掉落容器内容物和矿车物品
 */
TEST_F(EntityDropItemGameRuleTest, ChestMinecart_DropItem_WhenEntityDropsEnabled_DropsContentsAndCart)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::ChestMinecartEntity minecart(EntityInstanceId(1));
    minecart.setWorld(&m_world);
    minecart.setPosition(0.0, 0.0, 0.0);

    // 在容器中放入一个物品
    if (minecart.getInventory() != nullptr) {
        ItemStack diamond(Items::DIAMOND, 5);
        minecart.setInventoryItem(0, diamond);
    }

    minecart.dropItem(nullptr);

    // 应该生成2个物品实体：1个容器内容物 + 1个矿车物品
    EXPECT_EQ(m_world.spawnedEntityCount(), 2u)
        << "ChestMinecartEntity::dropItem should spawn 2 item entities (contents + cart) when "
           "DO_ENTITY_DROPS is true";
}

/**
 * @brief 测试箱子矿车在 DO_ENTITY_DROPS=false 时不掉落内容物和矿车物品
 */
TEST_F(EntityDropItemGameRuleTest, ChestMinecart_DropItem_WhenEntityDropsDisabled_NoDrops)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::ChestMinecartEntity minecart(EntityInstanceId(1));
    minecart.setWorld(&m_world);
    minecart.setPosition(0.0, 0.0, 0.0);

    // 在容器中放入一个物品
    if (minecart.getInventory() != nullptr) {
        ItemStack diamond(Items::DIAMOND, 5);
        minecart.setInventoryItem(0, diamond);
    }

    minecart.dropItem(nullptr);

    // 不应该生成任何物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "ChestMinecartEntity::dropItem should not spawn any items when DO_ENTITY_DROPS is false";
    // 矿车应该被标记为移除
    EXPECT_TRUE(minecart.isRemoved()) << "ChestMinecartEntity should be removed when DO_ENTITY_DROPS is false";
}

// ============================================================================
// HopperMinecartEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试漏斗矿车在 DO_ENTITY_DROPS=true 时掉落容器内容物和矿车物品
 */
TEST_F(EntityDropItemGameRuleTest, HopperMinecart_DropItem_WhenEntityDropsEnabled_DropsContentsAndCart)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::HopperMinecartEntity minecart(EntityInstanceId(1));
    minecart.setWorld(&m_world);
    minecart.setPosition(0.0, 0.0, 0.0);

    // 在漏斗矿车中放入一个物品
    if (minecart.getInventory() != nullptr) {
        ItemStack iron(Items::IRON_INGOT, 3);
        minecart.setInventoryItem(0, iron);
    }

    minecart.dropItem(nullptr);

    // 应该生成2个物品实体：1个容器内容物 + 1个矿车物品
    EXPECT_EQ(m_world.spawnedEntityCount(), 2u)
        << "HopperMinecartEntity::dropItem should spawn 2 item entities (contents + cart) when "
           "DO_ENTITY_DROPS is true";
}

/**
 * @brief 测试漏斗矿车在 DO_ENTITY_DROPS=false 时不掉落内容物和矿车物品
 */
TEST_F(EntityDropItemGameRuleTest, HopperMinecart_DropItem_WhenEntityDropsDisabled_NoDrops)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::HopperMinecartEntity minecart(EntityInstanceId(1));
    minecart.setWorld(&m_world);
    minecart.setPosition(0.0, 0.0, 0.0);

    // 在漏斗矿车中放入一个物品
    if (minecart.getInventory() != nullptr) {
        ItemStack iron(Items::IRON_INGOT, 3);
        minecart.setInventoryItem(0, iron);
    }

    minecart.dropItem(nullptr);

    // 不应该生成任何物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "HopperMinecartEntity::dropItem should not spawn any items when DO_ENTITY_DROPS is false";
    // 矿车应该被标记为移除
    EXPECT_TRUE(minecart.isRemoved()) << "HopperMinecartEntity should be removed when DO_ENTITY_DROPS is false";
}

// ============================================================================
// PaintingEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试 PaintingEntity::dropItem 在 DO_ENTITY_DROPS=true 时生成画作物品
 */
TEST_F(EntityDropItemGameRuleTest, Painting_DropItem_WhenEntityDropsEnabled_SpawnsItem)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::PaintingEntity painting;
    painting.setWorld(&m_world);
    painting.setPosition(0.0, 0.0, 0.0);

    painting.dropItem();

    // 应该生成1个物品实体（画作物品）
    EXPECT_EQ(m_world.spawnedEntityCount(), 1u)
        << "PaintingEntity::dropItem should spawn 1 item entity when DO_ENTITY_DROPS is true";
}

/**
 * @brief 测试 PaintingEntity::dropItem 在 DO_ENTITY_DROPS=false 时不生成物品
 */
TEST_F(EntityDropItemGameRuleTest, Painting_DropItem_WhenEntityDropsDisabled_NoItemSpawned)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::PaintingEntity painting;
    painting.setWorld(&m_world);
    painting.setPosition(0.0, 0.0, 0.0);

    painting.dropItem();

    // 不应该生成任何物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "PaintingEntity::dropItem should not spawn any item entity when DO_ENTITY_DROPS is false";
}

// ============================================================================
// ItemFrameEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试 ItemFrameEntity::dropItem 在 DO_ENTITY_DROPS=true 时掉落展示框和内容物
 */
TEST_F(EntityDropItemGameRuleTest, ItemFrame_DropItem_WhenEntityDropsEnabled_DropsFrameAndContent)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    itemFrame.setPosition(0.0, 0.0, 0.0);

    // 放入一个展示物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    ASSERT_TRUE(itemFrame.hasItem());

    itemFrame.dropItem();

    // 应该生成2个物品实体：1个展示框 + 1个内容物
    EXPECT_EQ(m_world.spawnedEntityCount(), 2u)
        << "ItemFrameEntity::dropItem should spawn 2 item entities (frame + content) when "
           "DO_ENTITY_DROPS is true and frame has content";
}

/**
 * @brief 测试 ItemFrameEntity::dropItem 在 DO_ENTITY_DROPS=true 时空框只掉落展示框
 */
TEST_F(EntityDropItemGameRuleTest, ItemFrame_DropItem_WhenEntityDropsEnabled_EmptyFrame_DropsFrameOnly)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    itemFrame.setPosition(0.0, 0.0, 0.0);

    // 不放入展示物品
    ASSERT_FALSE(itemFrame.hasItem());

    itemFrame.dropItem();

    // 应该生成1个物品实体（仅展示框）
    EXPECT_EQ(m_world.spawnedEntityCount(), 1u)
        << "ItemFrameEntity::dropItem should spawn 1 item entity (frame only) when "
           "DO_ENTITY_DROPS is true and frame is empty";
}

/**
 * @brief 测试 ItemFrameEntity::dropItem 在 DO_ENTITY_DROPS=false 时不掉落任何物品
 */
TEST_F(EntityDropItemGameRuleTest, ItemFrame_DropItem_WhenEntityDropsDisabled_NoItemsSpawned)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    itemFrame.setPosition(0.0, 0.0, 0.0);

    // 放入一个展示物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    ASSERT_TRUE(itemFrame.hasItem());

    itemFrame.dropItem();

    // 不应该生成任何物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "ItemFrameEntity::dropItem should not spawn any items when DO_ENTITY_DROPS is false";
}

/**
 * @brief 测试 ItemFrameEntity::dropItem 在 DO_ENTITY_DROPS=false 时仍清空展示物品
 *
 * 参考 MC 1.21.11：ItemFrame.dropItem() 中 this.setItem(ItemStack.EMPTY) 在游戏规则检查之前执行，
 * 无论 doEntityDrops 是否为 true，展示物品都会被清空。
 */
TEST_F(EntityDropItemGameRuleTest, ItemFrame_DropItem_WhenEntityDropsDisabled_StillClearsDisplayedItem)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::ItemFrameEntity itemFrame;
    itemFrame.setWorld(&m_world);
    itemFrame.setPosition(0.0, 0.0, 0.0);

    // 放入一个展示物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    ASSERT_TRUE(itemFrame.hasItem());

    itemFrame.dropItem();

    // 即使 DO_ENTITY_DROPS 为 false，展示物品也应该被清空
    EXPECT_FALSE(itemFrame.hasItem())
        << "ItemFrameEntity::dropItem should clear displayed item even when DO_ENTITY_DROPS is false";
}

// ============================================================================
// LeashKnotEntity dropItem 测试
// ============================================================================

/**
 * @brief 测试 LeashKnotEntity::dropItem 在 DO_ENTITY_DROPS=true 时生成拴绳物品
 */
TEST_F(EntityDropItemGameRuleTest, LeashKnot_DropItem_WhenEntityDropsEnabled_SpawnsItem)
{
    ASSERT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    entity::LeashKnotEntity leashKnot;
    leashKnot.setWorld(&m_world);
    leashKnot.setPosition(0.0, 0.0, 0.0);

    leashKnot.dropItem();

    // 应该生成1个物品实体（拴绳）
    EXPECT_EQ(m_world.spawnedEntityCount(), 1u)
        << "LeashKnotEntity::dropItem should spawn 1 item entity when DO_ENTITY_DROPS is true";
}

/**
 * @brief 测试 LeashKnotEntity::dropItem 在 DO_ENTITY_DROPS=false 时不生成物品
 */
TEST_F(EntityDropItemGameRuleTest, LeashKnot_DropItem_WhenEntityDropsDisabled_NoItemSpawned)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::LeashKnotEntity leashKnot;
    leashKnot.setWorld(&m_world);
    leashKnot.setPosition(0.0, 0.0, 0.0);

    leashKnot.dropItem();

    // 不应该生成任何物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u)
        << "LeashKnotEntity::dropItem should not spawn any item entity when DO_ENTITY_DROPS is false";
}

// ============================================================================
// 游戏规则切换测试
// ============================================================================

/**
 * @brief 测试 DO_ENTITY_DROPS 从 false 切换回 true 后实体掉落恢复
 */
TEST_F(EntityDropItemGameRuleTest, EntityDropsToggle_FromFalseToTrue_DropsResume)
{
    // 先设置为 false
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);

    entity::BoatEntity boat1(entity::BoatEntity::Type::OAK);
    boat1.setWorld(&m_world);
    boat1.setPosition(0.0, 0.0, 0.0);
    boat1.dropItem();
    EXPECT_EQ(m_world.spawnedEntityCount(), 0u) << "No drops when DO_ENTITY_DROPS is false";

    // 切换回 true
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    m_world.clearSpawnedEntities();

    entity::BoatEntity boat2(entity::BoatEntity::Type::OAK);
    boat2.setWorld(&m_world);
    boat2.setPosition(0.0, 0.0, 0.0);
    boat2.dropItem();
    EXPECT_EQ(m_world.spawnedEntityCount(), 1u) << "Drops resume when DO_ENTITY_DROPS is set back to true";
}

} // namespace test
} // namespace mc
