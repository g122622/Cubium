#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/MerchantContainerMenu.hpp"
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
#include "server/menu/CraftingMenu.hpp"
#include "server/network/outbound/PacketBuilders.hpp"
#include "server/network/play/ServerPlayHandler.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <limits>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::ir::play;
using namespace mc::network::test;

namespace {

class _ContainerTestMerchant : public world::village::trade::IMerchant {
public:
    _ContainerTestMerchant() { m_offers = std::make_unique<world::village::trade::MerchantOffers>(); }

    // IMerchant 接口实现
    [[nodiscard]] world::village::trade::MerchantOffers& getOffers() override { return *m_offers; }
    [[nodiscard]] const world::village::trade::MerchantOffers& getOffers() const override { return *m_offers; }
    void setOffers(world::village::trade::MerchantOffers offers) override
    {
        m_offers = std::make_unique<world::village::trade::MerchantOffers>(std::move(offers));
    }
    void overrideOffers(world::village::trade::MerchantOffers offers) override
    {
        m_offers = std::make_unique<world::village::trade::MerchantOffers>(std::move(offers));
    }
    [[nodiscard]] bool hasOffers() const override { return m_offers != nullptr && !m_offers->empty(); }
    [[nodiscard]] Player* getTradingPlayer() const override { return m_tradingPlayer; }
    void startTrading(Player* player) override { m_tradingPlayer = player; }
    void stopTrading() override { m_tradingPlayer = nullptr; }
    [[nodiscard]] bool isTrading() const override { return m_tradingPlayer != nullptr; }
    [[nodiscard]] i32 getExperience() const override { return m_experience; }
    void setExperience(i32 exp) override { m_experience = exp; }
    void addExperience(i32 amount) override { m_experience += amount; }
    void restock() override
    {
        if (m_offers) {
            m_offers->restockAll();
        }
    }
    void notifyTrade(world::village::trade::MerchantOffer& offer) override
    {
        offer.increaseUses();
        m_lastTradeXp = offer.getXp();
        m_notifyTradeCount++;
    }
    void notifyTradeUpdated(const ItemStack& resultStack) override
    {
        m_lastResultStack = resultStack;
        m_notifyTradeUpdatedCount++;
    }
    [[nodiscard]] i32 getVillagerXp() const override { return m_experience; }
    void overrideXp(i32 xp) override { m_experience = xp; }
    [[nodiscard]] bool showProgressBar() const override { return true; }
    [[nodiscard]] bool canRestock() const override { return true; }
    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] bool stillValid(const Player& player) const override
    {
        (void)player;
        return true;
    }
    [[nodiscard]] Entity* asEntity() override { return nullptr; }
    [[nodiscard]] const Entity* asEntity() const override { return nullptr; }

    // 测试辅助
    void addTestOffer(std::unique_ptr<world::village::trade::MerchantOffer> offer)
    {
        m_offers->addOffer(std::move(offer));
    }
    i32 getNotifyTradeCount() const { return m_notifyTradeCount; }
    i32 getNotifyTradeUpdatedCount() const { return m_notifyTradeUpdatedCount; }
    i32 getLastTradeXp() const { return m_lastTradeXp; }

private:
    std::unique_ptr<world::village::trade::MerchantOffers> m_offers;
    Player* m_tradingPlayer = nullptr;
    i32 m_experience = 0;
    i32 m_notifyTradeCount = 0;
    i32 m_notifyTradeUpdatedCount = 0;
    i32 m_lastTradeXp = 0;
    ItemStack m_lastResultStack;
};

class _ContainerRequestWorld : public server::ServerWorld {
public:
    explicit _ContainerRequestWorld(const BlockState* state)
        : ServerWorld(server::ServerWorldConfig{2, 2, 0, 1, "container-request-test"})
        , blockState(state)
    {}

    const BlockState* getBlockState(i32, i32, i32) const override { return blockState; }
    bool setBlockState(i32, i32, i32, const BlockState* state, i32) override
    {
        blockState = state;
        return true;
    }
    void playEvent(i32, const BlockPos&, i32) override {}
    const BlockState* blockState;
};

class _ContainerRequestServer : public server::IntegratedServer {
public:
    _ContainerRequestServer()
    {
        m_playerManager = std::make_unique<server::core::PlayerManager>(10);
        merchant = std::make_unique<_ContainerTestMerchant>();
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

    std::unique_ptr<_ContainerTestMerchant> merchant;
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

TEST_F(NetworkTestBase, PlaySelectTradeAndEmptyMerchantOffersWire)
{
    auto selected = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(selected, PlayPacket{SelectTrade{2}}).success());
    EXPECT_EQ(selected.bytes(), (std::vector<u8>{50, 2}));
    auto decoded = tables()->playSb->decode(selected);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<SelectTrade>(decoded.value()).index, 2);
    const MerchantOffersUpdate empty{1, {}, 1, 0, true, false};
    auto offers = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(offers, PlayPacket{empty}).success());
    EXPECT_EQ(offers.bytes(), (std::vector<u8>{50, 1, 0, 1, 0, 1, 0}));
    auto packet = tables()->playCb->decode(offers);
    ASSERT_TRUE(packet.success());
    EXPECT_EQ(std::get<MerchantOffersUpdate>(packet.value()), empty);
}

TEST_F(NetworkTestBase, PlayMerchantOffersCostOrderAndComponentPredicate)
{
    TradeOfferView offer{};
    offer.costA = TradeItemCost{1, 2, {0}};
    offer.result = ItemStackView{3, 1, {0, 0}};
    offer.outOfStock = false;
    offer.uses = 0;
    offer.maxUses = 12;
    offer.xp = 2;
    offer.specialPrice = -1;
    offer.priceMultiplier = 0.5f;
    offer.demand = 3;
    const MerchantOffersUpdate input{1, {offer}, 2, 10, true, true};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playCb->encode(buf, PlayPacket{input}).success());
    ASSERT_EQ(buf.size(), 40u);
    ASSERT_GE(buf.size(), 11u);
    EXPECT_EQ((std::vector<u8>(buf.bytes().begin(), buf.bytes().begin() + 11)),
        (std::vector<u8>{50, 1, 1, 1, 2, 0, 1, 3, 0, 0, 0}));
    EXPECT_EQ((std::vector<u8>(buf.bytes().begin() + 16, buf.bytes().begin() + 36)),
        (std::vector<u8>{0, 0, 0, 12, 0, 0, 0, 2, 0xff, 0xff, 0xff, 0xff, 0x3f, 0, 0, 0, 0, 0, 0, 3}));
    auto decoded = tables()->playCb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<MerchantOffersUpdate>(decoded.value()), input);
    offer.costB = TradeItemCost{2, 1, {1, 3, 7}};
    const MerchantOffersUpdate withPredicate{1, {offer}, 3, 20, false, true};
    EXPECT_EQ(
        std::get<MerchantOffersUpdate>(roundTripGeneric(*tables()->playCb, PlayPacket{withPredicate})), withPredicate);
    for (std::size_t size = 1; size < buf.size(); ++size) {
        buffer::RegistryByteBuf truncated(buf.data(), size, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playCb->decode(truncated).success());
    }
}

TEST_F(ContainerRequestTest, SelectTradeFillsPaymentAndBuilderPreservesOfferState)
{
    m_server->merchant->addTestOffer(std::make_unique<world::village::trade::MerchantOffer>(
        ItemStack(Items::EMERALD, 2), ItemStack(Items::BREAD, 1), 12, 5, 0.05f));
    m_server->player->inventory().setItem(0, ItemStack(Items::EMERALD, 5));
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Merchant, BlockPos(0, 0, 0)).success());
    auto* menu = dynamic_cast<MerchantContainerMenu*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    route(PlayPacket{SelectTrade{0}});
    EXPECT_EQ(menu->getSlot(0)->getItem().getItem(), Items::EMERALD);
    EXPECT_EQ(menu->getSlot(0)->getItem().getCount(), 5);
    EXPECT_EQ(menu->getSlot(2)->getItem().getItem(), Items::BREAD);
    EXPECT_EQ(m_server->merchant->getNotifyTradeCount(), 0);
    auto built = server::net::buildMerchantOffersIr(*menu);
    ASSERT_TRUE(built.success());
    const auto& packet = std::get<MerchantOffersUpdate>(std::get<PlayPacket>(built.value().packet));
    ASSERT_EQ(packet.offers.size(), 1u);
    EXPECT_EQ(packet.offers.front().costA.count, 2);
    EXPECT_EQ(packet.offers.front().costA.components, (std::vector<u8>{0}));
    EXPECT_EQ(packet.offers.front().maxUses, 12);
    EXPECT_EQ(packet.offers.front().xp, 5);
    EXPECT_EQ(packet.level, 2);
    EXPECT_TRUE(packet.canRestock);
    route(PlayPacket{SelectTrade{-1}});
    route(PlayPacket{SelectTrade{5}});
    EXPECT_EQ(menu->getSlot(0)->getItem().getCount(), 5);
}

TEST_F(ContainerRequestTest, InputSlotMutationsRecomputeOutputAndBindMenuIndices)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0)).success());
    auto* anvil = dynamic_cast<AnvilContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(anvil, nullptr);
    route(PlayPacket{RenameItem{"Named"}});
    anvil->getSlot(0)->set(ItemStack(Items::DIAMOND_SWORD, 1));
    EXPECT_EQ(anvil->getOutputSlot().getCustomName(), "Named");
    EXPECT_EQ(anvil->getSlot(10)->getSlotNumber(), 10);
    anvil->getSlot(0)->set(ItemStack::EMPTY);
    EXPECT_TRUE(anvil->getOutputSlot().isEmpty());
    EXPECT_EQ(anvil->getRepairCost(), 0);
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &m_server->player->inventory()));
    auto* crafting = dynamic_cast<InventoryCraftingMenu*>(m_server->containerManager().getPlayerInventoryMenu(1));
    ASSERT_NE(crafting, nullptr);
    crafting->getResultInventory().setResultItem(ItemStack(Items::DIAMOND, 1));
    crafting->getSlot(1)->set(ItemStack::EMPTY);
    EXPECT_TRUE(crafting->getSlot(0)->getItem().isEmpty());
}

TEST_F(ContainerRequestTest, AnvilResultRequiresExperienceAndConsumesInputOnce)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0)).success());
    auto* anvil = dynamic_cast<AnvilContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(anvil, nullptr);
    anvil->getSlot(0)->set(ItemStack(Items::DIAMOND_SWORD, 1));
    route(PlayPacket{RenameItem{"Paid"}});
    anvil->clicked(2, 0, ClickType::Pick, *m_server->player);
    EXPECT_TRUE(anvil->getCarriedItem().isEmpty());
    EXPECT_FALSE(anvil->getInputSlot1().isEmpty());
    m_server->player->addExperienceLevels(3);
    anvil->clicked(2, 0, ClickType::Pick, *m_server->player);
    EXPECT_EQ(anvil->getCarriedItem().getCustomName(), "Paid");
    EXPECT_EQ(m_server->player->experienceLevel(), 2);
    EXPECT_TRUE(anvil->getInputSlot1().isEmpty());
    EXPECT_TRUE(anvil->getOutputSlot().isEmpty());
    anvil->clicked(2, 0, ClickType::Pick, *m_server->player);
    EXPECT_EQ(m_server->player->experienceLevel(), 2);
}

TEST_F(ContainerRequestTest, AnvilRenameCostSaturatesAndBulkResultIsAtomic)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Anvil, BlockPos(0, 0, 0)).success());
    auto* anvil = dynamic_cast<AnvilContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(anvil, nullptr);
    auto item = ItemStack(Items::STONE, 64);
    item.setRepairCost(std::numeric_limits<i32>::max());
    anvil->getSlot(0)->set(item);
    route(PlayPacket{RenameItem{"Bulk"}});
    EXPECT_EQ(anvil->getRepairCost(), 39);
    m_server->player->setGameMode(GameMode::Creative);
    anvil->clicked(2, 1, ClickType::Pick, *m_server->player);
    EXPECT_EQ(anvil->getCarriedItem().getCount(), 64);
    EXPECT_TRUE(anvil->getInputSlot1().isEmpty());
}
