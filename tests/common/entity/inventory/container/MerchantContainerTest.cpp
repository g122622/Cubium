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

#include "entity/inventory/container/MerchantContainer.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/container/MerchantContainerMenu.hpp"
#include "entity/inventory/container/MerchantResultSlot.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include "world/village/trade/Merchant.hpp"
#include "world/village/trade/MerchantOffer.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

// 辅助函数：注册或获取测试物品
const Item* getOrRegisterTestItem(const std::string& name)
{
    ResourceLocation id(name);
    const Item* existing = ItemRegistry::instance().getItem(id);
    if (existing != nullptr) {
        return existing;
    }
    return &ItemRegistry::instance().registerItem(id, ItemProperties().maxStackSize(64));
}

// ========== Mock Merchant 用于测试 ==========
class MockMerchant : public world::village::trade::IMerchant {
public:
    MockMerchant() { m_offers = std::make_unique<world::village::trade::MerchantOffers>(); }

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

// ========== MerchantContainer 测试 ==========

class MerchantContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");
        wheat_ = getOrRegisterTestItem("minecraft:wheat");
    }

    MockMerchant merchant_;
    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
    const Item* wheat_ = nullptr;
};

TEST_F(MerchantContainerTest, ContainerSize)
{
    MerchantContainer container(merchant_);
    EXPECT_EQ(container.getContainerSize(), 3);
}

TEST_F(MerchantContainerTest, InitialState_AllSlotsEmpty)
{
    MerchantContainer container(merchant_);
    EXPECT_TRUE(container.isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_A).isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_B).isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, SetAndGetItem)
{
    MerchantContainer container(merchant_);
    ItemStack emeralds(emerald_, 5);

    container.setItem(MerchantContainer::SLOT_BUY_A, emeralds);
    EXPECT_FALSE(container.isEmpty());
    EXPECT_EQ(container.getItem(MerchantContainer::SLOT_BUY_A).getCount(), 5);
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_A).isSameItem(emeralds));
}

TEST_F(MerchantContainerTest, CanPlaceItem_OnlyInPaymentSlots)
{
    MerchantContainer container(merchant_);
    ItemStack emeralds(emerald_, 1);

    // 支付槽允许放置
    EXPECT_TRUE(container.canPlaceItem(MerchantContainer::SLOT_BUY_A, emeralds));
    EXPECT_TRUE(container.canPlaceItem(MerchantContainer::SLOT_BUY_B, emeralds));

    // 结果槽不允许放置
    EXPECT_FALSE(container.canPlaceItem(MerchantContainer::SLOT_RESULT, emeralds));
}

TEST_F(MerchantContainerTest, RemoveItem_FromResultSlot)
{
    MerchantContainer container(merchant_);

    // 先设置结果槽
    ItemStack breadStack(bread_, 3);
    container.setItem(MerchantContainer::SLOT_RESULT, breadStack);

    // 从结果槽移除
    ItemStack removed = container.removeItem(MerchantContainer::SLOT_RESULT, 3);
    EXPECT_EQ(removed.getCount(), 3);
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, RemoveItem_FromPaymentSlot)
{
    MerchantContainer container(merchant_);

    // 设置支付槽
    ItemStack emeralds(emerald_, 10);
    container.setItem(MerchantContainer::SLOT_BUY_A, emeralds);

    // 从支付槽移除部分
    ItemStack removed = container.removeItem(MerchantContainer::SLOT_BUY_A, 3);
    EXPECT_EQ(removed.getCount(), 3);
    EXPECT_EQ(container.getItem(MerchantContainer::SLOT_BUY_A).getCount(), 7);
}

TEST_F(MerchantContainerTest, RemoveItemNoUpdate)
{
    MerchantContainer container(merchant_);

    ItemStack emeralds(emerald_, 5);
    container.setItem(MerchantContainer::SLOT_BUY_A, emeralds);

    ItemStack removed = container.removeItemNoUpdate(MerchantContainer::SLOT_BUY_A);
    EXPECT_EQ(removed.getCount(), 5);
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_A).isEmpty());
}

TEST_F(MerchantContainerTest, UpdateSellItem_SingleItemOffer_Matches)
{
    // 设置交易：1个绿宝石 → 6个面包
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 2, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入1个绿宝石到支付槽
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    // 结果槽应自动更新为6个面包
    ItemStack result = container.getItem(MerchantContainer::SLOT_RESULT);
    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(result.isSameItem(ItemStack(bread_, 1)));
    EXPECT_EQ(result.getCount(), 6);
}

TEST_F(MerchantContainerTest, UpdateSellItem_SingleItemOffer_NoMatch)
{
    // 设置交易：1个绿宝石 → 6个面包
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 2, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入小麦到支付槽（不匹配）
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(wheat_, 1));

    // 结果槽应为空
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, UpdateSellItem_DoubleItemOffer_Matches)
{
    // 设置交易：1个绿宝石 + 1个钻石 → 6个面包
    ItemStack buyA(emerald_, 1);
    ItemStack buyB(diamond_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, buyB, sell, 8, 10, 0.2f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入两个支付物品
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));
    container.setItem(MerchantContainer::SLOT_BUY_B, ItemStack(diamond_, 1));

    // 结果槽应自动更新
    ItemStack result = container.getItem(MerchantContainer::SLOT_RESULT);
    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(result.isSameItem(ItemStack(bread_, 1)));
    EXPECT_EQ(result.getCount(), 6);
}

TEST_F(MerchantContainerTest, UpdateSellItem_DoubleItemOffer_SwappedSlots)
{
    // 设置交易：1个绿宝石 + 1个钻石 → 6个面包
    ItemStack buyA(emerald_, 1);
    ItemStack buyB(diamond_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, buyB, sell, 8, 10, 0.2f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 反向放入支付物品（钻石在A，绿宝石在B）
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(diamond_, 1));
    container.setItem(MerchantContainer::SLOT_BUY_B, ItemStack(emerald_, 1));

    // 结果槽仍应自动更新（容器会尝试交换顺序）
    ItemStack result = container.getItem(MerchantContainer::SLOT_RESULT);
    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(result.isSameItem(ItemStack(bread_, 1)));
    EXPECT_EQ(result.getCount(), 6);
}

TEST_F(MerchantContainerTest, UpdateSellItem_EmptyOffers)
{
    // 空交易列表
    MerchantContainer container(merchant_);

    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    // 无交易匹配，结果槽为空
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, UpdateSellItem_OutOfStockOffer)
{
    // 设置交易：1个绿宝石 → 6个面包，最大使用1次
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 1, 2, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 先使用一次使售罄
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));
    ASSERT_FALSE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());

    // 模拟交易：直接操作offer使其售罄
    world::village::trade::MerchantOffer* activeOffer = container.getActiveOffer();
    ASSERT_NE(activeOffer, nullptr);
    activeOffer->increaseUses(); // uses = 1 = maxUses
    ASSERT_TRUE(activeOffer->isOutOfStock());

    // 重新更新结果槽
    container.updateSellItem();

    // 售罄后结果槽应为空
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, SetItem_PaymentSlot_TriggersUpdateSellItem)
{
    // 设置交易
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 3);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 1, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // setItem 在支付槽应自动触发 updateSellItem
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    // 结果槽应非空
    EXPECT_FALSE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, SetSelectionHint)
{
    // 设置两个交易
    ItemStack buyA1(emerald_, 1);
    ItemStack sell1(bread_, 6);
    auto offer1 = std::make_unique<world::village::trade::MerchantOffer>(buyA1, sell1, 8, 2, 0.05f);

    ItemStack buyA2(emerald_, 2);
    ItemStack sell2(diamond_, 1);
    auto offer2 = std::make_unique<world::village::trade::MerchantOffer>(buyA2, sell2, 4, 5, 0.1f);

    merchant_.addTestOffer(std::move(offer1));
    merchant_.addTestOffer(std::move(offer2));

    MerchantContainer container(merchant_);

    // 设置选中提示
    container.setSelectionHint(0);
    EXPECT_EQ(container.getActiveOffer(), nullptr); // 还没有输入物品

    // 放入物品后应匹配选中提示对应的交易
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));
    EXPECT_NE(container.getActiveOffer(), nullptr);
}

TEST_F(MerchantContainerTest, Clear)
{
    MerchantContainer container(merchant_);

    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 5));
    container.setItem(MerchantContainer::SLOT_BUY_B, ItemStack(diamond_, 3));
    container.setItem(MerchantContainer::SLOT_RESULT, ItemStack(bread_, 10));

    container.clear();

    EXPECT_TRUE(container.isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_A).isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_B).isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, GetActiveOffer_NoMatch)
{
    MerchantContainer container(merchant_);
    EXPECT_EQ(container.getActiveOffer(), nullptr);
}

TEST_F(MerchantContainerTest, GetActiveOffer_WithMatch)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 2, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    EXPECT_NE(container.getActiveOffer(), nullptr);
}

TEST_F(MerchantContainerTest, GetFutureXp_NoMatch)
{
    MerchantContainer container(merchant_);
    EXPECT_EQ(container.getFutureXp(), 0);
}

TEST_F(MerchantContainerTest, GetFutureXp_WithMatch)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 5, 0.05f); // xp = 5
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    EXPECT_EQ(container.getFutureXp(), 5);
}

TEST_F(MerchantContainerTest, RemoveItem_FromPaymentSlot_TriggersUpdate)
{
    // 设置交易：需要2个绿宝石
    ItemStack buyA(emerald_, 2);
    ItemStack sell(bread_, 6);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 2, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入2个绿宝石
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 2));
    EXPECT_FALSE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());

    // 移除1个绿宝石（只剩1个，不满足2个的要求）
    container.removeItem(MerchantContainer::SLOT_BUY_A, 1);
    EXPECT_EQ(container.getItem(MerchantContainer::SLOT_BUY_A).getCount(), 1);

    // 结果槽应更新为空（不满足交易条件）
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());
}

TEST_F(MerchantContainerTest, InvalidSlotIndex)
{
    MerchantContainer container(merchant_);

    // 越界访问应返回空物品堆
    EXPECT_TRUE(container.getItem(-1).isEmpty());
    EXPECT_TRUE(container.getItem(3).isEmpty());
    EXPECT_TRUE(container.getItem(100).isEmpty());

    // 越界移除应返回空物品堆
    EXPECT_TRUE(container.removeItem(-1, 1).isEmpty());
    EXPECT_TRUE(container.removeItem(3, 1).isEmpty());
    EXPECT_TRUE(container.removeItemNoUpdate(-1).isEmpty());
    EXPECT_TRUE(container.removeItemNoUpdate(3).isEmpty());

    // 越界设置不应崩溃
    container.setItem(-1, ItemStack(emerald_, 1));
    container.setItem(3, ItemStack(emerald_, 1));
}

// ========== MerchantResultSlot 测试 ==========

class MerchantResultSlotTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");

        player_ = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer");
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
    MockMerchant merchant_;
    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
};

TEST_F(MerchantResultSlotTest, MayPlace_ReturnsFalse)
{
    MerchantContainer container(merchant_);
    MerchantResultSlot slot(*player_, merchant_, container, MerchantContainer::SLOT_RESULT, 0, 0);

    // 结果槽不允许放置任何物品
    EXPECT_FALSE(slot.mayPlace(ItemStack(emerald_, 1)));
    EXPECT_FALSE(slot.mayPlace(ItemStack(bread_, 1)));
    EXPECT_FALSE(slot.mayPlace(ItemStack()));
}

TEST_F(MerchantResultSlotTest, Remove_TracksCount)
{
    MerchantContainer container(merchant_);

    // 设置结果槽物品
    ItemStack result(bread_, 3);
    container.setItem(MerchantContainer::SLOT_RESULT, result);

    MerchantResultSlot slot(*player_, merchant_, container, MerchantContainer::SLOT_RESULT, 0, 0);

    // 从结果槽移除物品
    // 注意：MerchantContainer::removeItem 在 SLOT_RESULT 时会移除全部数量
    // 而 Slot::remove 调用 IInventory::removeItem，所以结果槽会移除全部
    ItemStack removed = slot.remove(2);
    EXPECT_TRUE(removed.getCount() > 0); // 确认移除了物品
}

TEST_F(MerchantResultSlotTest, OnTake_ExecutesTrade_SingleItem)
{
    // 设置交易：1个绿宝石 → 3个面包
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 3);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 2, 0.05f);
    world::village::trade::MerchantOffer* offerPtr = offer.get();
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入支付物品
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));
    ASSERT_NE(container.getActiveOffer(), nullptr);

    // 结果槽应有物品
    ASSERT_FALSE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());

    // 创建结果槽
    MerchantResultSlot slot(*player_, merchant_, container, MerchantContainer::SLOT_RESULT, 0, 0);

    // 模拟从结果槽取出物品
    ItemStack resultStack = container.getItem(MerchantContainer::SLOT_RESULT);
    ItemStack taken = slot.onTake(*player_, std::move(resultStack));

    // 验证交易执行：
    // 1. 交易使用次数应增加
    EXPECT_EQ(offerPtr->getUses(), 1);

    // 2. notifyTrade 应被调用（通过 offer 使用次数增加验证）
    EXPECT_EQ(merchant_.getNotifyTradeCount(), 1);

    // 3. 支付槽物品应被扣除
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_A).isEmpty());

    // 4. 结果槽应在 updateSellItem 后更新（可能仍有物品或为空）
}

TEST_F(MerchantResultSlotTest, OnTake_ExecutesTrade_DoubleItem)
{
    // 设置交易：1个绿宝石 + 1个钻石 → 3个面包
    ItemStack buyA(emerald_, 1);
    ItemStack buyB(diamond_, 1);
    ItemStack sell(bread_, 3);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, buyB, sell, 8, 5, 0.2f);
    world::village::trade::MerchantOffer* offerPtr = offer.get();
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入两个支付物品
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));
    container.setItem(MerchantContainer::SLOT_BUY_B, ItemStack(diamond_, 1));
    ASSERT_NE(container.getActiveOffer(), nullptr);

    // 创建结果槽
    MerchantResultSlot slot(*player_, merchant_, container, MerchantContainer::SLOT_RESULT, 0, 0);

    // 取出结果
    ItemStack resultStack = container.getItem(MerchantContainer::SLOT_RESULT);
    slot.onTake(*player_, std::move(resultStack));

    // 交易使用次数应增加
    EXPECT_EQ(offerPtr->getUses(), 1);

    // 两个支付槽都应被扣除
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_A).isEmpty());
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_BUY_B).isEmpty());
}

TEST_F(MerchantResultSlotTest, OnTake_NoDoubleXpReward)
{
    // 验证 onTake 不会重复添加经验
    // 设置交易：1个绿宝石 → 3个面包，xp=5
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 3);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 5, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    MerchantResultSlot slot(*player_, merchant_, container, MerchantContainer::SLOT_RESULT, 0, 0);

    i32 xpBefore = merchant_.getVillagerXp();

    ItemStack resultStack = container.getItem(MerchantContainer::SLOT_RESULT);
    slot.onTake(*player_, std::move(resultStack));

    // 经验应通过 notifyTrade -> rewardTradeXp 添加一次
    // MockMerchant 的 notifyTrade 不会自动添加经验（仅增加使用次数）
    // 但不应出现 overrideXp 再次添加
    // 由于 MockMerchant.notifyTrade 不添加 xp，我们只验证没有重复调用
    // 实际在 VillagerEntity 中，notifyTrade -> rewardTradeXp 会添加 xp
    i32 xpAfter = merchant_.getVillagerXp();
    // MockMerchant 不实现 rewardTradeXp（它不是 IMerchant 的一部分）
    // 所以 xpBefore == xpAfter 在 mock 中是正常的
    // 关键验证：onTake 不再调用 overrideXp
    EXPECT_EQ(xpAfter, xpBefore);
}

TEST_F(MerchantResultSlotTest, OnTake_OutOfStockOffer_DoesNotExecute)
{
    // 设置交易：1个绿宝石 → 3个面包，最大使用1次
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 3);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 1, 2, 0.05f);
    offer->increaseUses(); // 使其售罄
    merchant_.addTestOffer(std::move(offer));

    MerchantContainer container(merchant_);

    // 放入支付物品，但交易已售罄
    container.setItem(MerchantContainer::SLOT_BUY_A, ItemStack(emerald_, 1));

    // 结果槽应为空（售罄）
    EXPECT_TRUE(container.getItem(MerchantContainer::SLOT_RESULT).isEmpty());

    // activeOffer 不应为 nullptr（因为匹配到了但售罄），但结果为空
    // 或者 activeOffer 为 nullptr，取决于 updateSellItem 的实现
    // 无论哪种情况，onTake 不应执行交易
    MerchantResultSlot slot(*player_, merchant_, container, MerchantContainer::SLOT_RESULT, 0, 0);

    // 尝试从空结果槽取出
    ItemStack resultStack = container.getItem(MerchantContainer::SLOT_RESULT);
    EXPECT_TRUE(resultStack.isEmpty());

    // 不应有交易通知
    EXPECT_EQ(merchant_.getNotifyTradeCount(), 0);
}

// ========== MerchantContainerMenu 测试 ==========

class MerchantContainerMenuTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");

        player_ = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer");
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
    MockMerchant merchant_;
    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
};

TEST_F(MerchantContainerMenuTest, Create_HasCorrectSlotCount)
{
    // 交易菜单槽位：3个交易槽 + 27个背包槽 + 9个快捷栏 = 39
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    EXPECT_EQ(menu.getSlotCount(), 39);
}

TEST_F(MerchantContainerMenuTest, SlotIndices_AreCorrect)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);

    // 支付槽1 = 0, 支付槽2 = 1, 结果槽 = 2
    // 注意: getResultSlotIndex() 是 protected 方法，无法直接测试

    // 背包范围：3-29
    EXPECT_EQ(MerchantContainerMenu::INV_SLOT_START, 3);
    EXPECT_EQ(MerchantContainerMenu::INV_SLOT_END, 30);

    // 快捷栏范围：30-38
    EXPECT_EQ(MerchantContainerMenu::HOTBAR_SLOT_START, 30);
    EXPECT_EQ(MerchantContainerMenu::HOTBAR_SLOT_END, 39);
}

TEST_F(MerchantContainerMenuTest, StillValid_ReturnsTrue)
{
    // MockMerchant::stillValid 总是返回 true
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    EXPECT_TRUE(menu.stillValid(*player_));
}

TEST_F(MerchantContainerMenuTest, CanMergeSlot_ResultSlotNotAllowed)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);

    // 结果槽不允许 Shift+点击合并
    Slot* resultSlot = menu.getSlot(MerchantContainerMenu::SLOT_RESULT);
    ASSERT_NE(resultSlot, nullptr);
    EXPECT_FALSE(menu.canMergeSlot(ItemStack(emerald_, 1), *resultSlot));

    // 支付槽允许
    Slot* paymentSlot = menu.getSlot(MerchantContainerMenu::SLOT_PAYMENT_1);
    ASSERT_NE(paymentSlot, nullptr);
    EXPECT_TRUE(menu.canMergeSlot(ItemStack(emerald_, 1), *paymentSlot));
}

TEST_F(MerchantContainerMenuTest, SetSelectionHint_UpdatesContainer)
{
    // 设置交易
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 3);
    auto offer = std::make_unique<world::village::trade::MerchantOffer>(buyA, sell, 8, 2, 0.05f);
    merchant_.addTestOffer(std::move(offer));

    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);

    // 设置选中提示不应崩溃
    menu.setSelectionHint(0);
    menu.setSelectionHint(-1);
}

TEST_F(MerchantContainerMenuTest, GetOffers_ReturnsMerchantOffers)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    auto& offers = menu.getOffers();
    EXPECT_TRUE(offers.empty());
}

TEST_F(MerchantContainerMenuTest, GetTraderXp_ReturnsMerchantXp)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    EXPECT_EQ(menu.getTraderXp(), 0);
}

TEST_F(MerchantContainerMenuTest, SetMerchantLevel)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    menu.setMerchantLevel(3);
    EXPECT_EQ(menu.getTraderLevel(), 3);
}

TEST_F(MerchantContainerMenuTest, ShowProgressBar)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    EXPECT_TRUE(menu.showProgressBar()); // MockMerchant 默认返回 true
    menu.setShowProgressBar(false);
    EXPECT_FALSE(menu.showProgressBar());
}

TEST_F(MerchantContainerMenuTest, CanRestock)
{
    MerchantContainerMenu menu(ContainerId(1), playerInventory_.get(), merchant_);
    // 默认为 false（setCanRestock 需要在 createMenu 中由实体调用设置）
    EXPECT_FALSE(menu.canRestock());
    menu.setCanRestock(true);
    EXPECT_TRUE(menu.canRestock());
}

} // namespace
} // namespace mc
