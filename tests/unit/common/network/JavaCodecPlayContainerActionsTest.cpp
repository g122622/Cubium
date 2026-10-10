#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/BeaconContainer.hpp"
#include "common/entity/inventory/container/EnchantmentContainer.hpp"
#include "common/entity/inventory/container/LoomContainer.hpp"
#include "common/entity/inventory/container/MerchantContainerMenu.hpp"
#include "common/item/Items.hpp"
#include "common/item/component/DataComponentPatchWire.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/items/special/bundle/BundleItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/backend/java/mappings/JavaBannerPatternIds.hpp"
#include "common/network/backend/java/mappings/JavaMobEffectIdMap.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IntegratedServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/network/handshake/RegistryDataBuilder.hpp"
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

class _ButtonRequestMenu : public AbstractContainerMenu {
public:
    _ButtonRequestMenu(ContainerId id, PlayerInventory* inventory)
        : AbstractContainerMenu(id, inventory)
    {}
    bool stillValid(const Player&) const override { return m_valid; }
    bool clickMenuButton(Player&, i32 button) override
    {
        ++m_calls;
        m_button = button;
        return true;
    }
    i32 m_calls = 0;
    i32 m_button = -1;
    bool m_valid = true;
};

class _ContainerRequestWorld : public server::ServerWorld {
public:
    explicit _ContainerRequestWorld(const BlockState* state)
        : ServerWorld(server::ServerWorldConfig{2, 2, 0, 1, "container-request-test"})
        , m_blockState(state)
    {}

    const BlockState* getBlockState(i32, i32, i32) const override { return m_blockState; }
    bool setBlockState(i32, i32, i32, const BlockState* state, i32) override
    {
        m_blockState = state;
        return true;
    }
    void playEvent(i32, const BlockPos&, i32) override {}
    BlockEntity* getBlockEntity(const BlockPos&) override { return m_beacon.get(); }
    std::unique_ptr<blockentity::BeaconEntity> m_beacon;
    const BlockState* m_blockState;
};

class _ContainerRequestServer : public server::IntegratedServer {
public:
    _ContainerRequestServer()
    {
        m_playerManager = std::make_unique<server::core::PlayerManager>(10);
        m_merchant = std::make_unique<_ContainerTestMerchant>();
        m_containerManager = std::make_unique<server::interaction::ContainerManager>(*m_playerManager);
        m_world = std::make_unique<_ContainerRequestWorld>(&VanillaBlocks::ANVIL->defaultState());
        auto entity = std::make_unique<Player>(101, "Player", *m_world->entityRegistry());
        m_player = entity.get();
        m_player->setWorld(m_world.get());
        const auto entityId = m_world->entityManager().addEntity(std::move(entity));
        EXPECT_TRUE(m_playerEntityManager.registerExistingPlayerEntity(1, entityId, *m_world));
        m_playerManager->addPlayer(1, "00000000000000000000000000000001", "Player", nullptr);
        m_inventoryManager = std::make_unique<server::interaction::InventoryManager>(*m_playerManager);
        m_inventoryManager->setInventoryResolver([this](PlayerId) { return &m_player->inventory(); });
        containerManager().setInventoryManager(m_inventoryManager.get());
        containerManager().setMenuFactory(
            [this](ContainerId id, ContainerType type, const BlockPos& position, PlayerInventory*, PlayerId) {
                server::ContainerMenuCreateResult result;
                if (type == ContainerType::Anvil) {
                    result.menu = std::make_unique<AnvilContainer>(id, &m_player->inventory(), position, m_world.get());
                } else if (type == ContainerType::Beacon) {
                    m_world->m_blockState = &VanillaBlocks::BEACON->defaultState();
                    m_world->m_beacon = std::make_unique<blockentity::BeaconEntity>(position);
                    result.menu =
                        std::make_unique<BeaconContainer>(id, &m_player->inventory(), *m_world, *m_world->m_beacon);
                } else if (type == ContainerType::Enchantment) {
                    m_world->m_blockState = &VanillaBlocks::ENCHANTING_TABLE->defaultState();
                    result.menu =
                        std::make_unique<EnchantmentContainer>(id, &m_player->inventory(), position, m_world.get());
                } else if (type == ContainerType::Loom) {
                    m_world->m_blockState = &VanillaBlocks::LOOM->defaultState();
                    result.menu = std::make_unique<entity::inventory::container::LoomContainer>(
                        id, &m_player->inventory(), position, m_world.get());
                } else if (type == ContainerType::Generic9x1) {
                    result.menu = std::make_unique<_ButtonRequestMenu>(id, &m_player->inventory());
                } else if (type == ContainerType::Merchant) {
                    m_merchant->startTrading(m_player);
                    auto menu = std::make_unique<MerchantContainerMenu>(id, &m_player->inventory(), *m_merchant);
                    menu->setMerchantLevel(2);
                    menu->setShowProgressBar(m_merchant->showProgressBar());
                    menu->setCanRestock(m_merchant->canRestock());
                    result.menu = std::move(menu);
                }
                return result;
            });
    }

    server::ServerWorld* getPlayerWorld(PlayerId) override { return m_world.get(); }

    std::unique_ptr<_ContainerTestMerchant> m_merchant;
    Player* m_player = nullptr;
    std::unique_ptr<_ContainerRequestWorld> m_world;
};

class ContainerRequestTest : public NetworkTestBase {
protected:
    void SetUp() override
    {
        NetworkTestBase::SetUp();
        item::enchant::EnchantmentRegistry::initialize();
        ASSERT_TRUE(backend::java::JavaMobEffectIdMap::instance().initialize().success());
        BlockTags::initialize();
        m_server = std::make_unique<_ContainerRequestServer>();
        m_handler = std::make_unique<server::net::ServerPlayHandler>(*m_server);
    }

    void TearDown() override
    {
        m_handler.reset();
        m_server->containerManager().setOnContainerClose({});
        m_server->containerManager().setOnContainerData({});
        m_server->containerManager().setOnContainerUpdate({});
        m_server->containerManager().closeAll();
        m_server.reset();
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
    m_server->m_world->m_blockState = &VanillaBlocks::AIR->defaultState();
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
    m_server->m_player->setGameMode(GameMode::Spectator);
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
    for (i32 slot = 0; slot < m_server->m_player->inventory().getContainerSize(); ++slot) {
        const auto item = m_server->m_player->inventory().getItem(slot);
        if (item.getItem() == Items::DIAMOND_SWORD) swords += item.getCount();
        if (item.getItem() == Items::DIAMOND) diamonds += item.getCount();
    }
    EXPECT_EQ(swords, 1);
    EXPECT_EQ(diamonds, 2);
}

TEST_F(ContainerRequestTest, PlayerInventoryCloseReturnsCraftingCellsAndKeepsMenuReusable)
{
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &m_server->m_player->inventory()));
    auto* menu = m_server->containerManager().getPlayerInventoryMenu(1);
    ASSERT_NE(menu, nullptr);
    menu->getSlot(1)->set(ItemStack(Items::STONE, 3));
    route(PlayPacket{ContainerClose{0}});
    EXPECT_EQ(m_server->containerManager().getPlayerInventoryMenu(1), menu);
    EXPECT_TRUE(menu->getSlot(1)->getItem().isEmpty());
    EXPECT_TRUE(menu->getSlot(0)->getItem().isEmpty());
    i32 stones = 0;
    for (i32 slot = 0; slot < m_server->m_player->inventory().getContainerSize(); ++slot) {
        const auto item = m_server->m_player->inventory().getItem(slot);
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
    m_server->m_world->m_blockState = &VanillaBlocks::AIR->defaultState();
    m_server->containerManager().tickMenus();
    EXPECT_EQ(m_server->containerManager().getOpenMenu(1), nullptr);
    ASSERT_EQ(closed.size(), 1u);
    EXPECT_EQ(closed.front(), opened.value());
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &m_server->m_player->inventory()));
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
    m_server->m_merchant->addTestOffer(std::make_unique<world::village::trade::MerchantOffer>(
        ItemStack(Items::EMERALD, 2), ItemStack(Items::BREAD, 1), 12, 5, 0.05f));
    m_server->m_player->inventory().setItem(0, ItemStack(Items::EMERALD, 5));
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Merchant, BlockPos(0, 0, 0)).success());
    auto* menu = dynamic_cast<MerchantContainerMenu*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    route(PlayPacket{SelectTrade{0}});
    EXPECT_EQ(menu->getSlot(0)->getItem().getItem(), Items::EMERALD);
    EXPECT_EQ(menu->getSlot(0)->getItem().getCount(), 5);
    EXPECT_EQ(menu->getSlot(2)->getItem().getItem(), Items::BREAD);
    EXPECT_EQ(m_server->m_merchant->getNotifyTradeCount(), 0);
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
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &m_server->m_player->inventory()));
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
    anvil->clicked(2, 0, ClickType::Pick, *m_server->m_player);
    EXPECT_TRUE(anvil->getCarriedItem().isEmpty());
    EXPECT_FALSE(anvil->getInputSlot1().isEmpty());
    m_server->m_player->addExperienceLevels(3);
    anvil->clicked(2, 0, ClickType::Pick, *m_server->m_player);
    EXPECT_EQ(anvil->getCarriedItem().getCustomName(), "Paid");
    EXPECT_EQ(m_server->m_player->experienceLevel(), 2);
    EXPECT_TRUE(anvil->getInputSlot1().isEmpty());
    EXPECT_TRUE(anvil->getOutputSlot().isEmpty());
    anvil->clicked(2, 0, ClickType::Pick, *m_server->m_player);
    EXPECT_EQ(m_server->m_player->experienceLevel(), 2);
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
    m_server->m_player->setGameMode(GameMode::Creative);
    anvil->clicked(2, 1, ClickType::Pick, *m_server->m_player);
    EXPECT_EQ(anvil->getCarriedItem().getCount(), 64);
    EXPECT_TRUE(anvil->getInputSlot1().isEmpty());
}

TEST_F(NetworkTestBase, PlayContainerButtonClickUsesTwoVarInts)
{
    const ContainerButtonClick input{300, 301};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{16, 0xac, 2, 0xad, 2}));
    auto decoded = tables()->playSb->decode(buf);
    ASSERT_TRUE(decoded.success());
    EXPECT_EQ(std::get<ContainerButtonClick>(decoded.value()), input);
}

TEST_F(ContainerRequestTest, ButtonsValidateMenuIdentitySpectatorAndValidity)
{
    const auto opened = m_server->containerManager().openContainer(1, ContainerType::Generic9x1, BlockPos(0, 0, 0));
    ASSERT_TRUE(opened.success());
    auto* menu = dynamic_cast<_ButtonRequestMenu*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    const i32 id = static_cast<i32>(opened.value());
    route(PlayPacket{ContainerButtonClick{id, 301}});
    EXPECT_EQ(menu->m_calls, 1);
    EXPECT_EQ(menu->m_button, 301);
    route(PlayPacket{ContainerButtonClick{id + 1, 1}});
    EXPECT_EQ(menu->m_calls, 1);
    m_server->m_player->setGameMode(GameMode::Spectator);
    route(PlayPacket{ContainerButtonClick{id, 1}});
    EXPECT_EQ(menu->m_calls, 1);
    m_server->m_player->setGameMode(GameMode::Survival);
    menu->m_valid = false;
    route(PlayPacket{ContainerButtonClick{id, 1}});
    EXPECT_EQ(menu->m_calls, 1);
}

TEST_F(ContainerRequestTest, EnchantmentMenuPublishesAllTenDataProperties)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Enchantment, BlockPos(0, 0, 0)).success());
    auto* menu = dynamic_cast<EnchantmentContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(menu->getTrackedIntCount(), 10);
    EXPECT_EQ(menu->getTrackedInt(3)->get(), m_server->m_player->xpSeed());
    menu->getSlot(0)->set(ItemStack(Items::DIAMOND_SWORD, 1));
    EXPECT_GT(menu->getEnchantmentLevel(0), 0);
    EXPECT_FALSE(menu->clickMenuButton(*m_server->m_player, -1));
    EXPECT_FALSE(menu->clickMenuButton(*m_server->m_player, 3));
}

TEST_F(ContainerRequestTest, LoomButtonsUseZeroBasedListAndPatternItemsAreReusable)
{
    using Loom = entity::inventory::container::LoomContainer;
    const auto opened = m_server->containerManager().openContainer(1, ContainerType::Loom, BlockPos(0, 0, 0));
    ASSERT_TRUE(opened.success());
    auto* menu = dynamic_cast<Loom*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    menu->getSlot(Loom::SLOT_BANNER)->set(ItemStack(Items::WHITE_BANNER, 1));
    menu->getSlot(Loom::SLOT_DYE)->set(ItemStack(Items::RED_DYE, 1));
    route(PlayPacket{ContainerButtonClick{static_cast<i32>(opened.value()), 0}});
    EXPECT_EQ(menu->getSelectedPatternIndex(), 0);
    EXPECT_FALSE(menu->getSlot(Loom::SLOT_RESULT)->getItem().isEmpty());
    EXPECT_FALSE(menu->clickMenuButton(*m_server->m_player, 32));
    menu->getSlot(Loom::SLOT_PATTERN)->set(ItemStack(Items::BORDURE_INDENTED_BANNER_PATTERN, 1));
    EXPECT_EQ(menu->getSelectedPatternIndex(), 0);
    EXPECT_FALSE(menu->clickMenuButton(*m_server->m_player, 1));
    menu->clicked(Loom::SLOT_RESULT, 0, ClickType::Pick, *m_server->m_player);
    EXPECT_EQ(menu->getSlot(Loom::SLOT_PATTERN)->getItem().getCount(), 1);
}

TEST_F(NetworkTestBase, LoomPatternTagsUseTheSameRegistryOrderAsButtons)
{
    const auto tags = server::net::buildConfigurationUpdateTags();
    const auto banner = std::find_if(
        tags.begin(), tags.end(), [](const auto& entry) { return entry.registryKey == "minecraft:banner_pattern"; });
    ASSERT_NE(banner, tags.end());
    ASSERT_EQ(banner->tags.size(), 11u);
    const auto& basic = banner->tags.front();
    ASSERT_EQ(basic.elementIds.size(), 32u);
    EXPECT_EQ(basic.elementIds.front(), backend::java::JavaBannerPatternIds::toWireId("minecraft:square_bottom_left"));
    EXPECT_EQ(basic.elementIds.back(), backend::java::JavaBannerPatternIds::toWireId("minecraft:gradient_up"));
    EXPECT_TRUE(blockentity::BannerPatterns::hasPatternItem(blockentity::BannerPatternType::CurlyBorder));
    EXPECT_TRUE(blockentity::BannerPatterns::hasPatternItem(blockentity::BannerPatternType::Bricks));
}

TEST_F(NetworkTestBase, BannerLayersUseHolderOffsetAndDyeVarInt)
{
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    ASSERT_NE(Items::WHITE_BANNER, nullptr);
    ItemStack banner(Items::WHITE_BANNER, 1);
    banner.getOrCreateTag()["BlockEntityTag"]["Patterns"] = nlohmann::json::array({{{"Pattern", "bl"}, {"Color", 14}}});
    auto view = toItemStackView(banner);
    EXPECT_EQ(view.componentsPatch, (std::vector<u8>{1, 0, 70, 1, 27, 14}));
    auto restored = fromItemStackView(view);
    ASSERT_TRUE(restored.success());
    EXPECT_EQ(restored.value().getTag()->at("BlockEntityTag").at("Patterns"),
        banner.getTag()->at("BlockEntityTag").at("Patterns"));
    nbt::tags::compound_tag saved;
    banner.toNbt(saved);
    const auto loaded = ItemStack::fromNbt(saved);
    ASSERT_TRUE(loaded.success());
    EXPECT_EQ(toItemStackView(loaded.value()).componentsPatch, view.componentsPatch);
}

TEST_F(ContainerRequestTest, EnchantmentButtonRejectsInsufficientLevelAndCreativeDoesNotConsumeLapis)
{
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Enchantment, BlockPos(0, 0, 0)).success());
    auto* menu = dynamic_cast<EnchantmentContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    menu->getSlot(0)->set(ItemStack(Items::DIAMOND_SWORD, 1));
    menu->getSlot(1)->set(ItemStack(Items::LAPIS_LAZULI, 3));
    EXPECT_FALSE(menu->clickMenuButton(*m_server->m_player, 0));
    EXPECT_EQ(menu->getSlot(1)->getItem().getCount(), 3);
    m_server->m_player->setGameMode(GameMode::Creative);
    ASSERT_TRUE(menu->clickMenuButton(*m_server->m_player, 0));
    EXPECT_EQ(menu->getSlot(1)->getItem().getCount(), 3);
    EXPECT_TRUE(menu->getSlot(0)->getItem().hasEnchantments());
}

TEST_F(NetworkTestBase, PlayEditBookGoldenAndUtf16Limits)
{
    EditBook input{40, {"hello"}, std::string("Title")};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{23, 40, 1, 5, 'h', 'e', 'l', 'l', 'o', 1, 5, 'T', 'i', 't', 'l', 'e'}));
    EXPECT_EQ(std::get<EditBook>(roundTripGeneric(*tables()->playSb, PlayPacket{input})), input);
    for (usize n = 1; n < buf.size(); ++n) {
        buffer::RegistryByteBuf cut(buf.data(), n, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playSb->decode(cut).success());
    }
    input.pages = {std::string(1025, 'x')};
    buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_FALSE(tables()->playSb->decode(buf).success());
    input.pages = {"ok"};
    input.title = std::string(33, 'a');
    buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_FALSE(tables()->playSb->decode(buf).success());
}

TEST_F(ContainerRequestTest, EditBookValidatesSlotsAndSigningUsesLiteralPagesAndServerAuthor)
{
    auto& inventory = m_server->m_player->inventory();
    inventory.setItem(0, ItemStack(Items::WRITABLE_BOOK, 1));
    route(PlayPacket{EditBook{0, {"draft"}, std::nullopt}});
    EXPECT_EQ(inventory.getItem(0).getTag()->at("writable_book_content").at("pages").at(0), "draft");
    inventory.setItem(9, ItemStack(Items::WRITABLE_BOOK, 1));
    route(PlayPacket{EditBook{9, {"invalid"}, std::string("bad")}});
    EXPECT_EQ(inventory.getItem(9).getItem(), Items::WRITABLE_BOOK);
    route(PlayPacket{EditBook{0, {"{\"selector\":\"@a\"}"}, std::string("Title")}});
    const auto signedBook = inventory.getItem(0);
    EXPECT_EQ(signedBook.getItem(), Items::WRITTEN_BOOK);
    EXPECT_FALSE(signedBook.getTag()->contains("writable_book_content"));
    const auto& content = signedBook.getTag()->at("written_book_content");
    EXPECT_EQ(content.at("author"), "Player");
    EXPECT_EQ(content.at("pages").at(0).at("text"), "{\"selector\":\"@a\"}");
    EXPECT_TRUE(content.at("resolved").get<bool>());
    const auto view = toItemStackView(signedBook);
    ASSERT_FALSE(view.componentsPatch.empty());
    EXPECT_EQ(view.componentsPatch.at(2), 53);
    auto restored = fromItemStackView(view);
    ASSERT_TRUE(restored.success());
    EXPECT_EQ(toItemStackView(restored.value()).componentsPatch, view.componentsPatch);
    route(PlayPacket{EditBook{0, {"overwrite"}, std::string("bad")}});
    EXPECT_EQ(inventory.getItem(0), signedBook);
}

TEST_F(NetworkTestBase, PlayBundleSelectionAllowsOnlyMinusOneAsNegativeIndex)
{
    BundleItemSelected input{36, -1};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{2, 36, 255, 255, 255, 255, 15}));
    EXPECT_EQ(std::get<BundleItemSelected>(roundTripGeneric(*tables()->playSb, PlayPacket{input})), input);
    input.selectedItemIndex = -2;
    buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_FALSE(tables()->playSb->decode(buf).success());
}

TEST_F(ContainerRequestTest, BundleSelectionUsesMenuSlotAndDoesNotPublishTransientState)
{
    using item::items::BundleContents;
    using item::items::BundleItem;
    auto& inventory = m_server->m_player->inventory();
    ItemStack bundle(Items::BUNDLE, 1);
    BundleContents contents(std::vector<ItemStack>{ItemStack(Items::STONE, 1), ItemStack(Items::DIRT, 1)});
    bundle.getOrCreateTag()["BundleContents"] = contents.toJson();
    inventory.setItem(0, bundle);
    ASSERT_TRUE(m_server->containerManager().openPlayerInventoryMenu(1, &inventory));
    auto* menu = m_server->containerManager().getPlayerInventoryMenu(1);
    menu->detectAndSendChanges();
    i32 notifications = 0;
    menu->addListener([&](i32, ItemStack) { ++notifications; });
    const auto before = toItemStackView(inventory.getItem(0));
    route(PlayPacket{BundleItemSelected{36, 1}});
    EXPECT_EQ(BundleItem::getSelectedItem(inventory.getItem(0)), 1);
    EXPECT_EQ(toItemStackView(inventory.getItem(0)).componentsPatch, before.componentsPatch);
    menu->detectAndSendChanges();
    EXPECT_EQ(notifications, 0);
    auto restored = fromItemStackView(before);
    ASSERT_TRUE(restored.success());
    EXPECT_EQ(BundleContents::fromItemStack(restored.value()).size(), 2);
    EXPECT_EQ(BundleItem::getSelectedItem(restored.value()), -1);
    menu->clicked(36, 1, ClickType::Pick, *m_server->m_player);
    EXPECT_EQ(menu->getCarriedItem().getItem(), Items::DIRT);
    route(PlayPacket{BundleItemSelected{-1, 0}});
}

TEST_F(NetworkTestBase, PlaySetBeaconUsesOptionalPureRegistryIds)
{
    SetBeacon input{0, 18};
    auto buf = makeBoundBuf();
    ASSERT_TRUE(tables()->playSb->encode(buf, PlayPacket{input}).success());
    EXPECT_EQ(buf.bytes(), (std::vector<u8>{51, 1, 0, 1, 18}));
    EXPECT_EQ(std::get<SetBeacon>(roundTripGeneric(*tables()->playSb, PlayPacket{input})), input);
    for (usize n = 1; n < buf.size(); ++n) {
        buffer::RegistryByteBuf cut(buf.data(), n, RegistryAccess::instance());
        EXPECT_FALSE(tables()->playSb->decode(cut).success());
    }
}

TEST_F(ContainerRequestTest, BeaconRequiresPaymentFiltersEffectsAndSynchronizesProperties)
{
    item::tag::ItemTags::initialize();
    ASSERT_TRUE(m_server->containerManager().openContainer(1, ContainerType::Beacon, BlockPos(0, 0, 0)).success());
    auto* menu = dynamic_cast<BeaconContainer*>(m_server->containerManager().getOpenMenu(1));
    ASSERT_NE(menu, nullptr);
    ASSERT_EQ(menu->getSlotCount(), 37);
    auto& beacon = *m_server->m_world->m_beacon;
    beacon.setLevel(4);
    route(PlayPacket{SetBeacon{0, 18}});
    EXPECT_EQ(beacon.getPrimaryEffect(), nullptr);
    EXPECT_FALSE(menu->getSlot(0)->mayPlace(ItemStack(Items::STONE, 1)));
    EXPECT_TRUE(menu->getSlot(0)->mayPlace(ItemStack(Items::EMERALD, 1)));
    EXPECT_EQ(menu->getSlot(0)->getMaxStackSize(), 1);
    menu->getSlot(0)->set(ItemStack(Items::EMERALD, 1));
    route(PlayPacket{SetBeacon{0, 18}});
    ASSERT_NE(beacon.getPrimaryEffect(), nullptr);
    EXPECT_EQ(*beacon.getPrimaryEffect(), entity::effect::EffectType::Speed);
    EXPECT_EQ(beacon.getSecondaryEffect(), nullptr);
    EXPECT_FALSE(menu->getSlot(0)->hasItem());
    EXPECT_EQ(menu->getTrackedInt(0)->get(), 4);
    EXPECT_EQ(menu->getTrackedInt(1)->get(), 1);
    m_server->m_player->setGameMode(GameMode::Spectator);
    menu->getSlot(0)->set(ItemStack(Items::DIAMOND, 1));
    route(PlayPacket{SetBeacon{2, 18}});
    EXPECT_TRUE(menu->getSlot(0)->hasItem());
    i32 dropped = 0;
    menu->setItemDropCallback([&](const ItemStack& stack, Player&, bool) { dropped += stack.getCount(); });
    m_server->containerManager().closeContainer(1);
    EXPECT_EQ(dropped, 1);
}
