#include <gtest/gtest.h>

#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/entity/loot/LootTable.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mc;

namespace {

class BlockInteractionManagerPlacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();

        server::ServerWorldConfig config;
        config.viewDistance = 8;
        config.dimension = 0;
        config.seed = 114514;
        config.isDebugWorld = false;

        m_world = std::make_unique<server::ServerWorld>(config);
        auto worldInit = m_world->initialize();
        ASSERT_TRUE(worldInit.success());

        m_connectionPair = std::make_unique<network::LocalConnectionPair>();
        m_connectionPair->connect();

        m_playerManager = std::make_unique<server::core::PlayerManager>();
        auto connection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());
        m_player = m_playerManager->addPlayer(m_playerId, "PlacementTester", connection);
        ASSERT_NE(m_player, nullptr);
        m_player->x = 0.5f;
        m_player->y = 64.0f;
        m_player->z = 0.5f;
        m_player->yaw = 0.0f;
        m_player->pitch = 0.0f;
        m_player->gameMode = GameMode::Survival;

        m_inventoryManager = std::make_unique<server::interaction::InventoryManager>(*m_playerManager);
        m_inventoryManager->initializeInventory(m_playerId);

        m_blockInteractionManager = std::make_unique<server::interaction::BlockInteractionManager>(
            *m_world, *m_playerManager, m_lootTableManager);
        m_blockInteractionManager->setInventoryManager(m_inventoryManager.get());
    }

    void TearDown() override {
        m_blockInteractionManager.reset();
        m_inventoryManager.reset();
        m_playerManager.reset();
        m_connectionPair.reset();

        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
    }

    void setHeldBlockItem(const Block& block, i32 count) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block.blockId());
        ASSERT_NE(blockItem, nullptr);

        PlayerInventory* inventory = m_inventoryManager->getInventory(m_playerId);
        ASSERT_NE(inventory, nullptr);

        inventory->setSelectedSlot(0);
        inventory->setItem(0, ItemStack(*blockItem, count));
    }

    [[nodiscard]] ItemStack heldItem() const {
        return m_inventoryManager->getHeldItem(m_playerId);
    }

protected:
    static constexpr PlayerId m_playerId = 1;

    std::unique_ptr<server::ServerWorld> m_world;
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
    std::unique_ptr<server::core::PlayerManager> m_playerManager;
    std::unique_ptr<server::interaction::InventoryManager> m_inventoryManager;
    std::unique_ptr<server::interaction::BlockInteractionManager> m_blockInteractionManager;

    loot::LootTableManager m_lootTableManager;
    server::ServerPlayerData* m_player = nullptr;
};

TEST_F(BlockInteractionManagerPlacementTest, RejectsPlacementWhenBlockIntersectsPlayer) {
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    setHeldBlockItem(*VanillaBlocks::STONE, 16);

    auto result = m_blockInteractionManager->handleBlockPlacement(
        m_playerId,
        BlockPos(0, 63, 0),
        Vector3(0.5f, 63.99f, 0.5f),
        Direction::Up,
        heldItem());

    ASSERT_TRUE(result.success());

    const server::BlockPlacementResult& placement = result.value();
    const BlockPos expectedPos(0, 64, 0);
    EXPECT_FALSE(placement.success);
    EXPECT_FALSE(placement.blockPlaced);
    EXPECT_FALSE(placement.itemConsumed);
    EXPECT_EQ(placement.position, expectedPos);
    EXPECT_EQ(placement.newBlockStateId, 0u);
    EXPECT_EQ(placement.message, "Cannot place block inside player");

    const BlockState* placedState = m_world->getBlockState(0, 64, 0);
    if (placedState != nullptr) {
        EXPECT_TRUE(placedState->isAir());
    }

    const PlayerInventory* inventory = m_inventoryManager->getInventory(m_playerId);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getSelectedStack().getCount(), 16);
}

TEST_F(BlockInteractionManagerPlacementTest, PlacesBlockWhenNoPlayerCollision) {
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    setHeldBlockItem(*VanillaBlocks::STONE, 16);

    m_player->x = 3.5f;
    m_player->y = 64.0f;
    m_player->z = 0.5f;

    auto result = m_blockInteractionManager->handleBlockPlacement(
        m_playerId,
        BlockPos(0, 63, 0),
        Vector3(0.5f, 63.99f, 0.5f),
        Direction::Up,
        heldItem());

    ASSERT_TRUE(result.success());

    const server::BlockPlacementResult& placement = result.value();
    const BlockPos expectedPos(0, 64, 0);
    EXPECT_TRUE(placement.success);
    EXPECT_TRUE(placement.blockPlaced);
    EXPECT_TRUE(placement.itemConsumed);
    EXPECT_EQ(placement.position, expectedPos);

    const BlockState* placedState = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(placedState, nullptr);
    EXPECT_TRUE(placedState->is(VanillaBlocks::STONE));

    const PlayerInventory* inventory = m_inventoryManager->getInventory(m_playerId);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getSelectedStack().getCount(), 15);
}

} // namespace
