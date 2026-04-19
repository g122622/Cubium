#include <gtest/gtest.h>

#include "server/menu/CraftingMenu.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/world/blockentity/CraftingTableEntity.hpp"

#include <memory>

using namespace mc;

namespace {

class CraftingMenuTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_player = std::make_unique<Player>(1, "MenuTester");
        m_playerInventory = std::make_unique<PlayerInventory>(m_player.get());
        m_blockEntity = std::make_unique<CraftingTableEntity>(BlockPos(0, 64, 0));
    }

    std::unique_ptr<Player> m_player;
    std::unique_ptr<PlayerInventory> m_playerInventory;
    std::unique_ptr<CraftingTableEntity> m_blockEntity;
};

TEST_F(CraftingMenuTest, StillValid_WhenPlayerIsNearCraftingTable_ReturnsTrue) {
    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());
    m_player->setPosition(0.5f, 64.0f, 0.5f);

    EXPECT_TRUE(menu.stillValid(*m_player));
}

TEST_F(CraftingMenuTest, StillValid_WhenPlayerIsTooFar_ReturnsFalse) {
    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());
    m_player->setPosition(12.5f, 64.0f, 0.5f);

    EXPECT_FALSE(menu.stillValid(*m_player));
}

} // namespace