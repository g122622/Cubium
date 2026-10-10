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
