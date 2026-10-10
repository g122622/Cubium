#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/item/Items.hpp"
#include "common/network/NetworkTestFixtures.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IntegratedServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/network/play/ServerPlayHandler.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

namespace {

class _ContainerRequestWorld : public server::ServerWorld {
public:
    explicit _ContainerRequestWorld(const BlockState* state)
        : ServerWorld(server::ServerWorldConfig{2, 2, 0, 1, "container-request-test"})
        , blockState(state)
    {}

    const BlockState* getBlockState(i32, i32, i32) const override { return blockState; }
    const BlockState* blockState;
};

class _ContainerRequestServer : public server::IntegratedServer {
public:
    _ContainerRequestServer()
    {
        m_playerManager = std::make_unique<server::core::PlayerManager>(10);
        m_containerManager = std::make_unique<server::interaction::ContainerManager>(*m_playerManager);
        world = std::make_unique<_ContainerRequestWorld>(&VanillaBlocks::ANVIL->defaultState());
        auto entity = std::make_unique<Player>(101, "Player", *world->entityRegistry());
        player = entity.get();
        player->setWorld(world.get());
        const auto entityId = world->entityManager().addEntity(std::move(entity));
        EXPECT_TRUE(m_playerEntityManager.registerExistingPlayerEntity(1, entityId, *world));
        m_playerManager->addPlayer(1, "00000000000000000000000000000001", "Player", nullptr);
        m_inventoryManager = std::make_unique<server::interaction::InventoryManager>(*m_playerManager);
        m_inventoryManager->setInventoryResolver([this](PlayerId) { return &player->inventory(); });
        containerManager().setInventoryManager(m_inventoryManager.get());
        containerManager().setMenuFactory(
            [this](ContainerId id, ContainerType type, const BlockPos& position, PlayerInventory*, PlayerId) {
                server::ContainerMenuCreateResult result;
                if (type == ContainerType::Anvil) {
                    result.menu = std::make_unique<AnvilContainer>(id, &player->inventory(), position, world.get());
                }
                return result;
            });
    }

    server::ServerWorld* getPlayerWorld(PlayerId) override { return world.get(); }

    Player* player = nullptr;
    std::unique_ptr<_ContainerRequestWorld> world;
};

class ContainerRequestTest : public NetworkTestBase {
protected:
    void SetUp() override
    {
        NetworkTestBase::SetUp();
        BlockTags::initialize();
        m_server = std::make_unique<_ContainerRequestServer>();
        m_handler = std::make_unique<server::net::ServerPlayHandler>(*m_server);
    }

    void route(const PlayPacket& packet) { m_handler->route(1, IrPacket{protocol::ConnectionProtocol::Play, packet}); }

    std::unique_ptr<_ContainerRequestServer> m_server;
    std::unique_ptr<server::net::ServerPlayHandler> m_handler;
};

} // namespace

TEST_F(NetworkTestBase, PlayRenameItemWire)
{
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{RenameItem{"X"}}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{47, 1, 'X'}));
    auto decoded = tables()->playSb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<RenameItem>(decoded.value()).name, "X");
}

TEST_F(ContainerRequestTest, RenameUpdatesAnvilOutputAndSendsCostData)
{
    std::vector<std::pair<i32, i32>> properties;
    m_server->containerManager().setOnContainerData(
        [&](PlayerId, ContainerId, i32 property, i32 value) { properties.emplace_back(property, value); });
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0)).success());
    auto* anvil = dynamic_cast<AnvilContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(anvil, nullptr);
    anvil->getSlot(0)->set(ItemStack(Items::DIAMOND_SWORD, 1));
    route(PlayPacket{RenameItem{"Renamed"}});
    m_server->containerManager().tickMenus();
    EXPECT_EQ(anvil->getItemName(), "Renamed");
    EXPECT_EQ(anvil->getOutputSlot().getCustomName(), "Renamed");
    EXPECT_EQ(anvil->getRepairCost(), 1);
    EXPECT_NE(std::find(properties.begin(), properties.end(), std::pair<i32, i32>{0, 1}), properties.end());
    m_server->world->blockState = &VanillaBlocks::AIR->defaultState();
    route(PlayPacket{RenameItem{"Stale"}});
    EXPECT_EQ(anvil->getItemName(), "Renamed");
}

TEST_F(ContainerRequestTest, RenameFiltersCharactersAndCountsUtf16Units)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0)).success());
    auto* anvil = dynamic_cast<AnvilContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(anvil, nullptr);
    route(PlayPacket{RenameItem{"Name\n\xc2\xa7"
                                "c"}});
    EXPECT_EQ(anvil->getItemName(), "Namec");
    std::string emoji;
    for (i32 i = 0; i < 25; ++i) {
        emoji += "\xf0\x9f\x98\x80";
    }
    route(PlayPacket{RenameItem{emoji}});
    EXPECT_EQ(anvil->getItemName(), emoji);
    route(PlayPacket{RenameItem{emoji + "\xf0\x9f\x98\x80"}});
    EXPECT_EQ(anvil->getItemName(), emoji);
    m_server->player->setGameMode(GameMode::Spectator);
    route(PlayPacket{RenameItem{"Blocked"}});
    EXPECT_EQ(anvil->getItemName(), emoji);
}

TEST_F(ContainerRequestTest, ClosingMenusReturnsInputToTheRealPlayerWithoutCopyingPreview)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0)).success());
    auto* menu = m_server->containerManager().getOpenMenu(1);
    ASSERT_NE(menu, nullptr);
    menu->getSlot(0)->set(ItemStack(Items::DIAMOND_SWORD, 1));
    menu->setCarriedItem(ItemStack(Items::DIAMOND, 2));
    route(PlayPacket{RenameItem{"Preview"}});
    m_server->containerManager().closeContainer(1);
    EXPECT_EQ(m_server->containerManager().getOpenMenu(1), nullptr);
    i32 swords = 0;
    i32 diamonds = 0;
    for (i32 slot = 0; slot < m_server->player->inventory().getContainerSize(); ++slot) {
        const auto item = m_server->player->inventory().getItem(slot);
        if (item.getItem() == Items::DIAMOND_SWORD) swords += item.getCount();
        if (item.getItem() == Items::DIAMOND) diamonds += item.getCount();
    }
    EXPECT_EQ(swords, 1);
    EXPECT_EQ(diamonds, 2);
}

TEST_F(ContainerRequestTest, PlayerInventoryCloseReturnsCraftingCellsAndKeepsMenuReusable)
{
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &m_server->player->inventory()));
    auto* menu = m_server->containerManager().getPlayerInventoryMenu(1);
    ASSERT_NE(menu, nullptr);
    menu->getSlot(1)->set(ItemStack(Items::STONE, 3));
    route(PlayPacket{ContainerClose{0}});
    EXPECT_EQ(m_server->containerManager().getPlayerInventoryMenu(1), menu);
    EXPECT_TRUE(menu->getSlot(1)->getItem().isEmpty());
    EXPECT_TRUE(menu->getSlot(0)->getItem().isEmpty());
    i32 stones = 0;
    for (i32 slot = 0; slot < m_server->player->inventory().getContainerSize(); ++slot) {
        const auto item = m_server->player->inventory().getItem(slot);
        if (item.getItem() == Items::STONE) stones += item.getCount();
    }
    EXPECT_EQ(stones, 3);
}

TEST_F(ContainerRequestTest, InvalidMenuClosesOnTickAndCloseAllClearsInventoryMenu)
{
    std::vector<ContainerId> closed;
    m_server->containerManager().setOnContainerClose(
        [&](PlayerId, ContainerId id, ContainerType, const BlockPos&) { closed.push_back(id); });
    const auto opened = m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0));
    ASSERT_TRUE(opened.success());
    m_server->world->blockState = &VanillaBlocks::AIR->defaultState();
    m_server->containerManager().tickMenus();
    EXPECT_EQ(m_server->containerManager().getOpenMenu(1), nullptr);
    ASSERT_EQ(closed.size(), 1u);
    EXPECT_EQ(closed.front(), opened.value());
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &m_server->player->inventory()));
    m_server->containerManager().closeAll();
    EXPECT_EQ(m_server->containerManager().getPlayerInventoryMenu(1), nullptr);
    ASSERT_EQ(closed.size(), 2u);
    EXPECT_EQ(closed.back(), 0);
}
