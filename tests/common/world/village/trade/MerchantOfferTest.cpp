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

#include "world/village/trade/MerchantOffer.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/village/trade/Merchant.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace world {
namespace village {
namespace trade {
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

/**
 * @brief MerchantOffer NBT 序列化测试
 *
 * 参考 MC 1.16.5 MerchantOffer NBT 格式：
 * - buy: 第一买入物品
 * - buyB: 第二买入物品（可选）
 * - sell: 卖出物品
 * - uses: 已使用次数
 * - maxUses: 最大使用次数
 * - xp: 交易经验
 * - priceMultiplier: 价格乘数
 * - specialPrice: 特殊价格修正
 * - demand: 需求修正
 * - restocksToday: 今日补货次数
 * - lastRestock: 上次补货时间
 */
class MerchantOfferTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册测试物品
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");
        book_ = getOrRegisterTestItem("minecraft:book");
        enchantedBook_ = getOrRegisterTestItem("minecraft:enchanted_book");
    }

    void TearDown() override
    {
        // 清理
    }

    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
    const Item* book_ = nullptr;
    const Item* enchantedBook_ = nullptr;
};

/**
 * @brief MerchantOffers 补货与需求更新测试
 *
 * 测试 MerchantOffers 集合的补货操作：
 * - needsRestockAny：检查是否有交易需要补货
 * - updateDemandAll：批量更新所有交易的需求值
 * - resetDailyRestockAll：重置所有交易的每日补货计数
 */
class MerchantOffersTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");
    }

    static const Item* getOrRegisterTestItem(const std::string& name)
    {
        ResourceLocation id(name);
        const Item* existing = ItemRegistry::instance().getItem(id);
        if (existing != nullptr) {
            return existing;
        }
        return &ItemRegistry::instance().registerItem(id, ItemProperties().maxStackSize(64));
    }

    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
    MerchantOffers offers;
};

TEST_F(MerchantOfferTest, DefaultConstruction)
{
    MerchantOffer offer;

    EXPECT_TRUE(offer.getBuyA().isEmpty());
    EXPECT_FALSE(offer.getBuyB().has_value());
    EXPECT_TRUE(offer.getSell().isEmpty());
    EXPECT_EQ(offer.getUses(), 0);
    EXPECT_EQ(offer.getMaxUses(), 12); // 默认值
    EXPECT_EQ(offer.getXp(), 0);
    EXPECT_FLOAT_EQ(offer.getPriceMultiplier(), 1.0f);
    EXPECT_EQ(offer.getSpecialPrice(), 0);
    EXPECT_EQ(offer.getDemand(), 0);
}

TEST_F(MerchantOfferTest, SingleItemConstruction)
{
    // 创建交易：1个绿宝石 → 1个面包
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 1);

    MerchantOffer offer(buyA, sell, 16, 2, 0.05f);

    EXPECT_TRUE(offer.getBuyA().canStackWith(buyA));
    EXPECT_TRUE(offer.getSell().canStackWith(sell));
    EXPECT_EQ(offer.getMaxUses(), 16);
    EXPECT_EQ(offer.getXp(), 2);
    EXPECT_FLOAT_EQ(offer.getPriceMultiplier(), 0.05f);
}

TEST_F(MerchantOfferTest, DoubleItemConstruction)
{
    // 创建交易：1个绿宝石 + 1个钻石 → 1个附魔书
    ItemStack buyA(emerald_, 1);
    ItemStack buyB(diamond_, 1);
    ItemStack sell(enchantedBook_, 1);

    MerchantOffer offer(buyA, buyB, sell, 8, 10, 0.2f);

    EXPECT_TRUE(offer.getBuyA().canStackWith(buyA));
    ASSERT_TRUE(offer.getBuyB().has_value());
    EXPECT_TRUE(offer.getBuyB()->canStackWith(buyB));
    EXPECT_TRUE(offer.getSell().canStackWith(sell));
    EXPECT_EQ(offer.getMaxUses(), 8);
    EXPECT_EQ(offer.getXp(), 10);
    EXPECT_FLOAT_EQ(offer.getPriceMultiplier(), 0.2f);
}

TEST_F(MerchantOfferTest, SerializeDeserializeBasic)
{
    // 创建交易
    ItemStack buyA(emerald_, 5);
    ItemStack sell(bread_, 10);
    MerchantOffer original(buyA, sell, 20, 5, 0.1f);
    original.increaseUses(); // 使用一次
    original.increaseUses(); // 使用两次

    // 序列化
    nbt::tags::compound_tag tag;
    original.serialize(tag);

    // 验证序列化结果
    EXPECT_NE(tag.value.find("buy"), tag.value.end());
    EXPECT_NE(tag.value.find("sell"), tag.value.end());
    EXPECT_NE(tag.value.find("uses"), tag.value.end());
    EXPECT_NE(tag.value.find("maxUses"), tag.value.end());
    EXPECT_NE(tag.value.find("xp"), tag.value.end());
    EXPECT_NE(tag.value.find("priceMultiplier"), tag.value.end());

    // 反序列化
    MerchantOffer deserialized = MerchantOffer::deserialize(tag);

    // 验证反序列化结果
    EXPECT_TRUE(deserialized.getBuyA().canStackWith(original.getBuyA()));
    EXPECT_EQ(deserialized.getBuyA().getCount(), original.getBuyA().getCount());
    EXPECT_TRUE(deserialized.getSell().canStackWith(original.getSell()));
    EXPECT_EQ(deserialized.getSell().getCount(), original.getSell().getCount());
    EXPECT_EQ(deserialized.getUses(), 2);
    EXPECT_EQ(deserialized.getMaxUses(), 20);
    EXPECT_EQ(deserialized.getXp(), 5);
    EXPECT_FLOAT_EQ(deserialized.getPriceMultiplier(), 0.1f);
}

TEST_F(MerchantOfferTest, SerializeDeserializeWithBuyB)
{
    // 创建双物品交易
    ItemStack buyA(emerald_, 3);
    ItemStack buyB(diamond_, 1);
    ItemStack sell(book_, 1);

    MerchantOffer original(buyA, buyB, sell, 12, 15, 0.5f);

    // 序列化
    nbt::tags::compound_tag tag;
    original.serialize(tag);

    // 验证 buyB 存在
    EXPECT_NE(tag.value.find("buyB"), tag.value.end());

    // 反序列化
    MerchantOffer deserialized = MerchantOffer::deserialize(tag);

    // 验证 buyB 反序列化正确
    ASSERT_TRUE(deserialized.getBuyB().has_value());
    EXPECT_TRUE(deserialized.getBuyB()->canStackWith(buyB));
    EXPECT_EQ(deserialized.getBuyB()->getCount(), 1);
}

TEST_F(MerchantOfferTest, SerializeDeserializePriceAdjustments)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);

    MerchantOffer original(buyA, sell, 16, 2, 0.05f);
    original.setSpecialPrice(-2); // 价格优惠
    original.setDemand(5);        // 需求修正
    original.restock();           // 补货一次

    // 序列化
    nbt::tags::compound_tag tag;
    original.serialize(tag);

    // 反序列化
    MerchantOffer deserialized = MerchantOffer::deserialize(tag);

    EXPECT_EQ(deserialized.getSpecialPrice(), -2);
    EXPECT_EQ(deserialized.getDemand(), 5);
    EXPECT_EQ(deserialized.getRestocksToday(), 1);
}

TEST_F(MerchantOfferTest, RoundTripComplete)
{
    // 创建完整的交易
    ItemStack buyA(emerald_, 10);
    ItemStack buyB(diamond_, 2);
    ItemStack sell(enchantedBook_, 1);
    sell.addEnchantment("minecraft:sharpness", 5); // 附魔

    MerchantOffer original(buyA, buyB, sell, 32, 30, 0.15f);
    original.increaseUses();
    original.increaseUses();
    original.increaseUses();
    original.setSpecialPrice(3);
    original.setDemand(10);

    // 序列化
    nbt::tags::compound_tag tag;
    original.serialize(tag);

    // 反序列化
    MerchantOffer deserialized = MerchantOffer::deserialize(tag);

    // 完整验证
    EXPECT_TRUE(deserialized.getBuyA().canStackWith(original.getBuyA()));
    EXPECT_EQ(deserialized.getBuyA().getCount(), 10);

    ASSERT_TRUE(deserialized.getBuyB().has_value());
    EXPECT_TRUE(deserialized.getBuyB()->canStackWith(original.getBuyB().value()));
    EXPECT_EQ(deserialized.getBuyB()->getCount(), 2);

    EXPECT_TRUE(deserialized.getSell().canStackWith(original.getSell()));
    EXPECT_EQ(deserialized.getSell().getCount(), 1);
    // 验证附魔保留
    EXPECT_TRUE(deserialized.getSell().hasEnchantment("minecraft:sharpness"));
    EXPECT_EQ(deserialized.getSell().getEnchantmentLevel("minecraft:sharpness"), 5);

    EXPECT_EQ(deserialized.getUses(), 3);
    EXPECT_EQ(deserialized.getMaxUses(), 32);
    EXPECT_EQ(deserialized.getXp(), 30);
    EXPECT_FLOAT_EQ(deserialized.getPriceMultiplier(), 0.15f);
    EXPECT_EQ(deserialized.getSpecialPrice(), 3);
    EXPECT_EQ(deserialized.getDemand(), 10);
}

TEST_F(MerchantOfferTest, CanAcceptSingleItem)
{
    ItemStack buyA(emerald_, 3);
    ItemStack sell(bread_, 6);

    MerchantOffer offer(buyA, sell, 16, 2, 1.0f);

    // 足够的绿宝石
    ItemStack offered(emerald_, 5);
    EXPECT_TRUE(offer.canAccept(offered));

    // 刚好足够的绿宝石
    ItemStack exactOffer(emerald_, 3);
    EXPECT_TRUE(offer.canAccept(exactOffer));

    // 不够的绿宝石
    ItemStack notEnough(emerald_, 2);
    EXPECT_FALSE(offer.canAccept(notEnough));

    // 错误的物品
    ItemStack wrongItem(diamond_, 10);
    EXPECT_FALSE(offer.canAccept(wrongItem));
}

TEST_F(MerchantOfferTest, IsOutOfStock)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 1);

    MerchantOffer offer(buyA, sell, 3, 1, 1.0f); // 最大使用 3 次

    EXPECT_FALSE(offer.isOutOfStock());

    offer.increaseUses();
    EXPECT_FALSE(offer.isOutOfStock());

    offer.increaseUses();
    EXPECT_FALSE(offer.isOutOfStock());

    offer.increaseUses(); // 第 3 次
    EXPECT_TRUE(offer.isOutOfStock());
}

TEST_F(MerchantOfferTest, Restock)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 1);

    MerchantOffer offer(buyA, sell, 2, 1, 1.0f);

    // 使用两次
    offer.increaseUses();
    offer.increaseUses();
    EXPECT_TRUE(offer.isOutOfStock());

    // 补货
    offer.restock();
    EXPECT_FALSE(offer.isOutOfStock());
    EXPECT_EQ(offer.getUses(), 0);
    EXPECT_EQ(offer.getRestocksToday(), 1);

    // 再次补货
    offer.restock();
    EXPECT_EQ(offer.getRestocksToday(), 2);
}

TEST_F(MerchantOfferTest, PriceAdjustment)
{
    ItemStack buyA(emerald_, 5); // 基础价格 5
    ItemStack sell(bread_, 1);

    MerchantOffer offer(buyA, sell, 16, 2, 0.1f);

    // 基础价格
    EXPECT_EQ(offer.getAdjustedBuyPrice(), 5);

    // 特殊价格修正（优惠）
    offer.setSpecialPrice(-2);
    EXPECT_EQ(offer.getAdjustedBuyPrice(), 3);

    // 需求调整 - 注意：applyDemand 会覆盖 specialPrice
    // applyDemand 设置 specialPrice = demand * priceMultiplier
    // 所以 specialPrice 变为 10 * 0.1 = 1
    offer.applyDemand(10);
    EXPECT_EQ(offer.getSpecialPrice(), 1);     // 被覆盖为 1
    EXPECT_EQ(offer.getAdjustedBuyPrice(), 6); // 5 + 1 = 6
}

TEST_F(MerchantOfferTest, Progress)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 1);

    MerchantOffer offer(buyA, sell, 4, 1, 1.0f);

    EXPECT_FLOAT_EQ(offer.getProgress(), 0.0f);

    offer.increaseUses();
    EXPECT_FLOAT_EQ(offer.getProgress(), 0.25f);

    offer.increaseUses();
    EXPECT_FLOAT_EQ(offer.getProgress(), 0.5f);

    offer.increaseUses();
    EXPECT_FLOAT_EQ(offer.getProgress(), 0.75f);

    offer.increaseUses();
    EXPECT_FLOAT_EQ(offer.getProgress(), 1.0f);
}

TEST_F(MerchantOfferTest, SerializeDeserializeEmptyBuyB)
{
    // 单物品交易（没有 buyB）
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 3);

    MerchantOffer original(buyA, sell, 8, 1, 0.05f);

    // 序列化
    nbt::tags::compound_tag tag;
    original.serialize(tag);

    // 验证 buyB 不存在
    EXPECT_EQ(tag.value.find("buyB"), tag.value.end());

    // 反序列化
    MerchantOffer deserialized = MerchantOffer::deserialize(tag);

    // 验证 buyB 不存在
    EXPECT_FALSE(deserialized.getBuyB().has_value());
}

// ============================================================================
// MerchantOffer::needsRestock() 测试
// ============================================================================
//
// 验证 needsRestock() 的逻辑：交易被使用过（uses > 0）时返回 true，
// 未被使用过（uses == 0）时返回 false。
// 这是 MC 原版逻辑：只要交易被使用过就视为需要补货，
// 而不是等到售罄才补货。

TEST_F(MerchantOfferTest, NeedsRestock_ZeroUses_ReturnsFalse)
{
    // 未使用过的交易不需要补货
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 16, 2, 0.05f);

    EXPECT_EQ(offer.getUses(), 0);
    EXPECT_FALSE(offer.needsRestock());
}

TEST_F(MerchantOfferTest, NeedsRestock_OneUse_ReturnsTrue)
{
    // 使用1次后需要补货
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 16, 2, 0.05f);

    offer.increaseUses();
    EXPECT_EQ(offer.getUses(), 1);
    EXPECT_TRUE(offer.needsRestock());
}

TEST_F(MerchantOfferTest, NeedsRestock_PartiallyUsed_ReturnsTrue)
{
    // 部分使用（未售罄）时也需要补货
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 8, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    offer.increaseUses();
    EXPECT_EQ(offer.getUses(), 3);
    EXPECT_FALSE(offer.isOutOfStock()); // 尚未售罄
    EXPECT_TRUE(offer.needsRestock());  // 但需要补货
}

TEST_F(MerchantOfferTest, NeedsRestock_FullyUsed_ReturnsTrue)
{
    // 售罄时需要补货
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 2, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    EXPECT_TRUE(offer.isOutOfStock());
    EXPECT_TRUE(offer.needsRestock());
}

TEST_F(MerchantOfferTest, NeedsRestock_AfterRestock_ReturnsFalse)
{
    // 补货后不再需要补货（uses 被重置为 0）
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 4, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    EXPECT_TRUE(offer.needsRestock());

    offer.restock();
    EXPECT_EQ(offer.getUses(), 0);
    EXPECT_FALSE(offer.needsRestock());
}

// ============================================================================
// MerchantOffer::updateDemand() 测试
// ============================================================================
//
// 验证 updateDemand() 的逻辑：
//   demand = demand + uses - (maxUses - uses)
// 即 demand += 2 * uses - maxUses
// 然后 specialPrice = demand * priceMultiplier

TEST_F(MerchantOfferTest, UpdateDemand_NoUses_DemandDecreases)
{
    // 未使用时：demand += 0 - maxUses = -maxUses
    // demand 初始为 0，maxUses = 16
    // 新 demand = 0 + 0 - 16 = -16
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 16, 2, 0.05f);

    EXPECT_EQ(offer.getDemand(), 0);
    offer.updateDemand();

    EXPECT_EQ(offer.getDemand(), -16);                                 // 0 + 0 - 16
    EXPECT_EQ(offer.getSpecialPrice(), static_cast<i32>(-16 * 0.05f)); // demand * priceMultiplier
}

TEST_F(MerchantOfferTest, UpdateDemand_HalfUses_DemandUnchanged)
{
    // 使用次数等于 maxUses 的一半时：demand += 2*uses - maxUses = 0
    // maxUses = 8, uses = 4
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 8, 2, 0.05f);

    for (int i = 0; i < 4; ++i) {
        offer.increaseUses();
    }
    EXPECT_EQ(offer.getUses(), 4);

    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), 0); // 0 + 4 - (8-4) = 0 + 4 - 4 = 0
    EXPECT_EQ(offer.getSpecialPrice(), 0);
}

TEST_F(MerchantOfferTest, UpdateDemand_MoreThanHalfUses_DemandIncreases)
{
    // 使用次数超过 maxUses 的一半时：demand > 0
    // maxUses = 8, uses = 6
    // demand = 0 + 6 - (8-6) = 0 + 6 - 2 = 4
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 8, 2, 0.2f);

    for (int i = 0; i < 6; ++i) {
        offer.increaseUses();
    }

    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), 4);                                // 0 + 6 - 2 = 4
    EXPECT_EQ(offer.getSpecialPrice(), static_cast<i32>(4 * 0.2f)); // 4 * 0.2 = 0 (truncated)
}

TEST_F(MerchantOfferTest, UpdateDemand_FullyUsed_DemandMaxIncrease)
{
    // 全部用完：maxUses = 4, uses = 4
    // demand = 0 + 4 - (4-4) = 0 + 4 - 0 = 4
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 4, 2, 0.05f);

    for (int i = 0; i < 4; ++i) {
        offer.increaseUses();
    }

    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), 4); // 0 + 4 - 0 = 4
}

TEST_F(MerchantOfferTest, UpdateDemand_CumulativeDemand)
{
    // 多次调用 updateDemand 会累积 demand
    // 第一次：uses=2, maxUses=8 → demand = 0 + 2 - 6 = -4
    // 补货后 uses=0 → 第二次：demand = -4 + 0 - 8 = -12
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 8, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), -4); // 0 + 2 - 6 = -4

    offer.restock(); // uses → 0
    EXPECT_EQ(offer.getUses(), 0);
    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), -12); // -4 + 0 - 8 = -12
}

TEST_F(MerchantOfferTest, UpdateDemand_SpecialPriceUpdated)
{
    // updateDemand 同时更新 specialPrice = demand * priceMultiplier
    ItemStack buyA(emerald_, 5);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 16, 2, 0.5f);

    // 使用8次：demand = 0 + 8 - 8 = 0
    for (int i = 0; i < 8; ++i) {
        offer.increaseUses();
    }
    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), 0);
    EXPECT_EQ(offer.getSpecialPrice(), 0);

    // 再使用4次（共12次）：demand = 0 + 12 - 4 = 8
    for (int i = 0; i < 4; ++i) {
        offer.increaseUses();
    }
    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), 8);               // 0 + 12 - 4 = 8
    EXPECT_EQ(offer.getSpecialPrice(), 4);         // 8 * 0.5 = 4
    EXPECT_EQ(offer.getAdjustedBuyPrice(), 5 + 4); // basePrice + specialPrice = 9
}

TEST_F(MerchantOfferTest, UpdateDemand_NegativeSpecialPriceReducesCost)
{
    // 负需求导致负 specialPrice，降低交易成本
    ItemStack buyA(emerald_, 10);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 16, 2, 0.5f);

    // 未使用：demand = 0 + 0 - 16 = -16
    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), -16);
    EXPECT_EQ(offer.getSpecialPrice(), -8);    // -16 * 0.5 = -8
    EXPECT_EQ(offer.getAdjustedBuyPrice(), 2); // max(1, 10 + (-8)) = 2
}

TEST_F(MerchantOfferTest, UpdateDemand_PriceMultiplierOne)
{
    // priceMultiplier = 1.0 时 specialPrice = demand
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 4, 2, 1.0f);

    for (int i = 0; i < 3; ++i) {
        offer.increaseUses();
    }
    // demand = 0 + 3 - (4-3) = 0 + 3 - 1 = 2
    offer.updateDemand();
    EXPECT_EQ(offer.getDemand(), 2);
    EXPECT_EQ(offer.getSpecialPrice(), 2); // 2 * 1.0 = 2
}

// ============================================================================
// MerchantOffers::needsRestockAny() 测试
// ============================================================================
//
// 验证 needsRestockAny() 的逻辑：只要有一个交易 needsRestock() 返回 true，
// 则 needsRestockAny() 返回 true。

TEST_F(MerchantOffersTest, NeedsRestockAny_NoOffers_ReturnsFalse)
{
    // 空交易列表不需要补货
    EXPECT_FALSE(offers.needsRestockAny());
}

TEST_F(MerchantOffersTest, NeedsRestockAny_NoUses_ReturnsFalse)
{
    // 所有交易都未使用过，不需要补货
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.05f);
    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_FALSE(offers.needsRestockAny());
}

TEST_F(MerchantOffersTest, NeedsRestockAny_OneOfferUsed_ReturnsTrue)
{
    // 只有一个交易被使用过
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.05f);

    offer1->increaseUses(); // 使用第一个交易
    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_TRUE(offers.needsRestockAny());
}

TEST_F(MerchantOffersTest, NeedsRestockAny_AllOffersUsed_ReturnsTrue)
{
    // 所有交易都被使用过
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.05f);

    offer1->increaseUses();
    offer2->increaseUses();
    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_TRUE(offers.needsRestockAny());
}

TEST_F(MerchantOffersTest, NeedsRestockAny_AfterRestockAll_ReturnsFalse)
{
    // 补货后所有交易 uses 归零，不需要补货
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 4, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.05f);

    offer1->increaseUses();
    offer1->increaseUses();
    offer2->increaseUses();
    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_TRUE(offers.needsRestockAny());

    offers.restockAll();

    EXPECT_FALSE(offers.needsRestockAny());
}

// ============================================================================
// MerchantOffers::updateDemandAll() 测试
// ============================================================================
//
// 验证 updateDemandAll() 对所有交易调用 updateDemand()。

TEST_F(MerchantOffersTest, UpdateDemandAll_UpdatesAllOffers)
{
    // 两个交易都有使用次数，updateDemandAll 应更新两者的 demand
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 8, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 16, 5, 0.1f);

    // offer1: 使用6次 → demand = 0 + 6 - (8-6) = 4
    for (int i = 0; i < 6; ++i) {
        offer1->increaseUses();
    }
    // offer2: 使用4次 → demand = 0 + 4 - (16-4) = -8
    for (int i = 0; i < 4; ++i) {
        offer2->increaseUses();
    }

    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_EQ(offers.getOffer(0)->getDemand(), 0);
    EXPECT_EQ(offers.getOffer(1)->getDemand(), 0);

    offers.updateDemandAll();

    EXPECT_EQ(offers.getOffer(0)->getDemand(), 4);  // 0 + 6 - 2 = 4
    EXPECT_EQ(offers.getOffer(1)->getDemand(), -8); // 0 + 4 - 12 = -8
}

TEST_F(MerchantOffersTest, UpdateDemandAll_EmptyOffers_NoCrash)
{
    // 空交易列表不应崩溃
    EXPECT_NO_THROW(offers.updateDemandAll());
}

TEST_F(MerchantOffersTest, UpdateDemandAll_SpecialPriceUpdated)
{
    // updateDemandAll 同时更新每个交易的 specialPrice
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 4, 2, 0.5f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.25f);

    // offer1: 使用3次 → demand = 0 + 3 - (4-3) = 2, specialPrice = 2 * 0.5 = 1
    for (int i = 0; i < 3; ++i) {
        offer1->increaseUses();
    }
    // offer2: 使用2次 → demand = 0 + 2 - (8-2) = -4, specialPrice = -4 * 0.25 = -1
    for (int i = 0; i < 2; ++i) {
        offer2->increaseUses();
    }

    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    offers.updateDemandAll();

    EXPECT_EQ(offers.getOffer(0)->getSpecialPrice(), 1);  // 2 * 0.5 = 1
    EXPECT_EQ(offers.getOffer(1)->getSpecialPrice(), -1); // -4 * 0.25 = -1
}

// ============================================================================
// MerchantOffers::resetDailyRestockAll() 测试
// ============================================================================
//
// 验证 resetDailyRestockAll() 重置所有交易的 m_restocksToday 为 0。

TEST_F(MerchantOffersTest, ResetDailyRestockAll_ResetsAllRestockCounts)
{
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 4, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.05f);

    // 模拟补货操作（补货会增加 restocksToday）
    offer1->increaseUses();
    offer1->increaseUses();
    offer1->restock(); // restocksToday = 1
    offer1->increaseUses();
    offer1->restock(); // restocksToday = 2

    offer2->increaseUses();
    offer2->restock(); // restocksToday = 1

    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 2);
    EXPECT_EQ(offers.getOffer(1)->getRestocksToday(), 1);

    // 重置每日补货计数
    offers.resetDailyRestockAll();

    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 0);
    EXPECT_EQ(offers.getOffer(1)->getRestocksToday(), 0);
}

TEST_F(MerchantOffersTest, ResetDailyRestockAll_EmptyOffers_NoCrash)
{
    // 空交易列表不应崩溃
    EXPECT_NO_THROW(offers.resetDailyRestockAll());
}

TEST_F(MerchantOffersTest, ResetDailyRestockAll_DoesNotResetUses)
{
    // resetDailyRestockAll 只重置 restocksToday，不影响 uses
    auto offer = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 4, 2, 0.05f);

    offer->increaseUses();
    offer->increaseUses();
    offer->restock(); // restocksToday = 1, uses = 0 (被 restock 重置)

    offer->increaseUses(); // uses = 1
    offer->increaseUses(); // uses = 2

    offers.addOffer(std::move(offer));

    EXPECT_EQ(offers.getOffer(0)->getUses(), 2);
    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 1);

    offers.resetDailyRestockAll();

    EXPECT_EQ(offers.getOffer(0)->getUses(), 2);          // uses 不受影响
    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 0); // restocksToday 被重置
}

// ============================================================================
// MerchantOffer::isDisabled() 测试
// ============================================================================
//
// 验证 isDisabled() 的逻辑：售罄且今日补货次数 >= 2 时禁用。

TEST_F(MerchantOfferTest, IsDisabled_NotOutOfStock_ReturnsFalse)
{
    // 未售罄，不应禁用
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 4, 2, 0.05f);

    EXPECT_FALSE(offer.isOutOfStock());
    EXPECT_FALSE(offer.isDisabled());
}

TEST_F(MerchantOfferTest, IsDisabled_OutOfStock_RestocksTodayZero_ReturnsFalse)
{
    // 售罄但未补货过，不算禁用（仍可补货）
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 2, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    EXPECT_TRUE(offer.isOutOfStock());
    EXPECT_EQ(offer.getRestocksToday(), 0);
    EXPECT_FALSE(offer.isDisabled());
}

TEST_F(MerchantOfferTest, IsDisabled_OutOfStock_RestocksTodayOne_ReturnsFalse)
{
    // 售罄且补货1次，不算禁用（还可再补货1次）
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 2, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    offer.restock(); // restocksToday = 1
    offer.increaseUses();
    offer.increaseUses();

    EXPECT_TRUE(offer.isOutOfStock());
    EXPECT_EQ(offer.getRestocksToday(), 1);
    EXPECT_FALSE(offer.isDisabled());
}

TEST_F(MerchantOfferTest, IsDisabled_OutOfStock_RestocksTodayTwo_ReturnsTrue)
{
    // 售罄且今日补货2次，禁用
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 2, 2, 0.05f);

    // 第一轮：用完 → 补货
    offer.increaseUses();
    offer.increaseUses();
    offer.restock(); // restocksToday = 1

    // 第二轮：用完 → 补货
    offer.increaseUses();
    offer.increaseUses();
    offer.restock(); // restocksToday = 2

    // 第三轮：用完
    offer.increaseUses();
    offer.increaseUses();

    EXPECT_TRUE(offer.isOutOfStock());
    EXPECT_EQ(offer.getRestocksToday(), 2);
    EXPECT_TRUE(offer.isDisabled()); // 售罄 + 今日补货 >= 2
}

// ============================================================================
// MerchantOffer::resetDailyRestock() 测试
// ============================================================================
//
// 验证 resetDailyRestock() 只重置 restocksToday，不影响其他字段。

TEST_F(MerchantOfferTest, ResetDailyRestock_ResetsRestocksToday)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 4, 2, 0.05f);

    // 补货两次
    offer.increaseUses();
    offer.increaseUses();
    offer.restock();
    offer.restock();

    EXPECT_EQ(offer.getRestocksToday(), 2);

    offer.resetDailyRestock();

    EXPECT_EQ(offer.getRestocksToday(), 0);
}

TEST_F(MerchantOfferTest, ResetDailyRestock_DoesNotAffectUsesOrDemand)
{
    ItemStack buyA(emerald_, 1);
    ItemStack sell(bread_, 6);
    MerchantOffer offer(buyA, sell, 8, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    offer.increaseUses(); // uses = 3

    offer.updateDemand(); // demand 变化

    offer.increaseUses();
    offer.increaseUses();
    offer.restock(); // restocksToday = 1, uses = 0

    offer.increaseUses(); // uses = 1

    EXPECT_EQ(offer.getUses(), 1);
    i32 demandBefore = offer.getDemand();
    EXPECT_EQ(offer.getRestocksToday(), 1);

    offer.resetDailyRestock();

    EXPECT_EQ(offer.getRestocksToday(), 0);
    EXPECT_EQ(offer.getUses(), 1);              // uses 不受影响
    EXPECT_EQ(offer.getDemand(), demandBefore); // demand 不受影响
}

// ============================================================================
// 补货完整流程集成测试
// ============================================================================
//
// 模拟村民补货的完整流程：needsRestockAny → updateDemandAll → restockAll → resetDailyRestockAll

TEST_F(MerchantOffersTest, RestockFullWorkflow_SimulateDailyRestockCycle)
{
    // 模拟一个完整的每日补货周期：
    // 1. 交易被使用
    // 2. 检查是否需要补货（needsRestockAny）
    // 3. 更新需求值（updateDemandAll）
    // 4. 补货（restockAll）
    // 5. 新的一天，重置每日补货计数（resetDailyRestockAll）

    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 5), ItemStack(bread_, 6), 8, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 3), ItemStack(diamond_, 1), 4, 10, 0.2f);

    // 模拟玩家交易
    for (int i = 0; i < 6; ++i) {
        offer1->increaseUses();
    }
    for (int i = 0; i < 3; ++i) {
        offer2->increaseUses();
    }

    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    // Step 1: 检查需要补货
    EXPECT_TRUE(offers.needsRestockAny());

    // Step 2: 更新需求值
    offers.updateDemandAll();
    // offer1: demand = 0 + 6 - (8-6) = 4, specialPrice = 4 * 0.05 = 0
    // offer2: demand = 0 + 3 - (4-3) = 2, specialPrice = 2 * 0.2 = 0
    EXPECT_EQ(offers.getOffer(0)->getDemand(), 4);
    EXPECT_EQ(offers.getOffer(1)->getDemand(), 2);

    // Step 3: 补货
    offers.restockAll();
    EXPECT_EQ(offers.getOffer(0)->getUses(), 0);
    EXPECT_EQ(offers.getOffer(1)->getUses(), 0);
    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 1);
    EXPECT_EQ(offers.getOffer(1)->getRestocksToday(), 1);

    // 补货后不再需要补货
    EXPECT_FALSE(offers.needsRestockAny());

    // 模拟再次交易
    for (int i = 0; i < 2; ++i) {
        offers.getOffer(0)->increaseUses();
    }
    EXPECT_TRUE(offers.needsRestockAny());

    // 第二次补货
    offers.updateDemandAll();
    offers.restockAll();
    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 2);
    EXPECT_EQ(offers.getOffer(1)->getRestocksToday(), 2);

    // Step 4: 新的一天，重置每日补货计数
    offers.resetDailyRestockAll();
    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 0);
    EXPECT_EQ(offers.getOffer(1)->getRestocksToday(), 0);

    // 需求值和特殊价格不受重置影响
    EXPECT_NE(offers.getOffer(0)->getDemand(), 0);
}

} // namespace
} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
