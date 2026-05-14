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

} // namespace
} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
