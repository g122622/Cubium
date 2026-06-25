/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do the the following conditions:
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

#include <gtest/gtest.h>

#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief ItemPredicate 单元测试
 *
 * 测试物品谓词的功能：
 * - 物品ID匹配
 * - 数量匹配
 * - 耐久匹配
 * - 药水类型匹配
 * - JSON 解析和序列化
 * - MC 1.16.5 兼容性
 */
class ItemPredicateTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化物品和药水注册表（只执行一次）
        Items::initialize();
        potion::Potions::initialize();
        item::tag::ItemTags::initialize();
    }
};

// ========== isAny 测试 ==========

TEST_F(ItemPredicateTest, DefaultIsAny)
{
    ItemPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(ItemPredicateTest, AnyPredicateMatchesAllItems)
{
    ItemPredicate predicate;
    ItemStack stack(Items::DIAMOND, 64);

    EXPECT_TRUE(predicate.test(stack));
}

TEST_F(ItemPredicateTest, AnyPredicateMatchesEmptyStack)
{
    ItemPredicate predicate;
    ItemStack emptyStack;

    EXPECT_TRUE(predicate.test(emptyStack));
}

// ========== 物品ID匹配测试 ==========

TEST_F(ItemPredicateTest, ItemIdMatch)
{
    // 创建匹配钻石的谓词
    ResourceLocation diamondId("minecraft", "diamond");
    ItemPredicate predicate(diamondId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    EXPECT_FALSE(predicate.isAny());

    ItemStack diamondStack(Items::DIAMOND, 1);
    EXPECT_TRUE(predicate.test(diamondStack));

    ItemStack ironStack(Items::IRON_INGOT, 1);
    EXPECT_FALSE(predicate.test(ironStack));
}

TEST_F(ItemPredicateTest, ItemIdMismatch)
{
    ResourceLocation diamondId("minecraft", "diamond");
    ItemPredicate predicate(diamondId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    // 铁锭不匹配钻石谓词
    ItemStack ironStack(Items::IRON_INGOT, 1);
    EXPECT_FALSE(predicate.test(ironStack));
}

// ========== 数量匹配测试 ==========

TEST_F(ItemPredicateTest, CountMatch)
{
    // 创建匹配数量为32的谓词
    ItemPredicate predicate(std::nullopt, IntBounds::exactly(32), IntBounds{}, std::nullopt, {}, {}, {});

    ItemStack stack32(Items::DIAMOND, 32);
    EXPECT_TRUE(predicate.test(stack32));

    ItemStack stack16(Items::DIAMOND, 16);
    EXPECT_FALSE(predicate.test(stack16));
}

// ========== 耐久匹配测试 ==========

TEST_F(ItemPredicateTest, DurabilityMatch)
{
    // 创建匹配耐久大于100的谓词
    IntBounds durabilityBounds;
    durabilityBounds = IntBounds::fromJson(R"({"min": 100})"_json);

    ItemPredicate predicate(std::nullopt, IntBounds{}, durabilityBounds, std::nullopt, {}, {}, {});

    // 钻石镐的耐久度是1562，满足条件
    ItemStack pickaxe(Items::DIAMOND_PICKAXE, 1);
    EXPECT_TRUE(predicate.test(pickaxe));

    // 石镐的耐久度是131，满足条件
    ItemStack stonePickaxe(Items::STONE_PICKAXE, 1);
    EXPECT_TRUE(predicate.test(stonePickaxe));
}

// ========== 药水类型匹配测试 ==========

TEST_F(ItemPredicateTest, PotionMatch_NightVision)
{
    // 验证药水注册表初始化
    ASSERT_NE(potion::Potions::NIGHT_VISION, nullptr) << "Potions::NIGHT_VISION is null";

    // 验证药水 ID
    ResourceLocation nightVisionId = potion::Potions::NIGHT_VISION->id();
    EXPECT_EQ(nightVisionId.toString(), "minecraft:night_vision")
        << "Expected 'minecraft:night_vision', got '" << nightVisionId.toString() << "'";

    // 创建匹配夜视药水的谓词
    ResourceLocation predicateId("minecraft", "night_vision");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, predicateId, {}, {}, {});

    EXPECT_FALSE(predicate.isAny());

    // 创建夜视药水
    ItemStack nightVisionPotion = potion::PotionUtils::createPotionItem(potion::Potions::NIGHT_VISION);
    ASSERT_FALSE(nightVisionPotion.isEmpty()) << "Failed to create night vision potion";

    // 验证药水ID设置
    const potion::Potion* actualPotion = potion::PotionUtils::getPotion(nightVisionPotion);
    ASSERT_NE(actualPotion, nullptr) << "getPotion returned null";
    EXPECT_EQ(actualPotion->id().toString(), "minecraft:night_vision")
        << "Potion ID mismatch: expected 'minecraft:night_vision', got '" << actualPotion->id().toString() << "'";

    EXPECT_TRUE(predicate.test(nightVisionPotion))
        << "Night vision potion should match minecraft:night_vision predicate";

    // 创建速度药水，应该不匹配
    ItemStack swiftnessPotion = potion::PotionUtils::createPotionItem(potion::Potions::SWIFTNESS);
    EXPECT_FALSE(predicate.test(swiftnessPotion));
}

TEST_F(ItemPredicateTest, PotionMatch_Healing)
{
    // 创建匹配治疗药水的谓词
    ResourceLocation healingId("minecraft", "healing");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, healingId, {}, {}, {});

    // 创建治疗药水
    ItemStack healingPotion = potion::PotionUtils::createPotionItem(potion::Potions::HEALING);
    EXPECT_TRUE(predicate.test(healingPotion));

    // 创建强效治疗药水，应该不匹配
    ItemStack strongHealingPotion = potion::PotionUtils::createPotionItem(potion::Potions::STRONG_HEALING);
    EXPECT_FALSE(predicate.test(strongHealingPotion));
}

TEST_F(ItemPredicateTest, PotionMatch_WaterBottle)
{
    // 创建匹配水瓶的谓词
    ResourceLocation waterId("minecraft", "water");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, waterId, {}, {}, {});

    // 创建水瓶
    ItemStack waterBottle = potion::PotionUtils::createPotionItem(potion::Potions::WATER);
    EXPECT_TRUE(predicate.test(waterBottle));

    // 夜视药水不是水瓶
    ItemStack nightVisionPotion = potion::PotionUtils::createPotionItem(potion::Potions::NIGHT_VISION);
    EXPECT_FALSE(predicate.test(nightVisionPotion));
}

TEST_F(ItemPredicateTest, PotionMatch_SplashPotion)
{
    // 创建匹配喷溅型夜视药水的谓词
    ResourceLocation nightVisionId("minecraft", "night_vision");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, nightVisionId, {}, {}, {});

    // 创建喷溅型夜视药水
    ItemStack splashNightVision = potion::PotionUtils::createSplashPotionItem(potion::Potions::NIGHT_VISION);
    EXPECT_TRUE(predicate.test(splashNightVision));

    // 创建喷溅型速度药水，应该不匹配
    ItemStack splashSwiftness = potion::PotionUtils::createSplashPotionItem(potion::Potions::SWIFTNESS);
    EXPECT_FALSE(predicate.test(splashSwiftness));
}

TEST_F(ItemPredicateTest, PotionMatch_LingeringPotion)
{
    // 创建匹配滞留型治疗药水的谓词
    ResourceLocation healingId("minecraft", "healing");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, healingId, {}, {}, {});

    // 创建滞留型治疗药水
    ItemStack lingeringHealing = potion::PotionUtils::createLingeringPotionItem(potion::Potions::HEALING);
    EXPECT_TRUE(predicate.test(lingeringHealing));
}

TEST_F(ItemPredicateTest, PotionMatch_NonPotionItem)
{
    // 创建匹配夜视药水的谓词
    ResourceLocation nightVisionId("minecraft", "night_vision");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, nightVisionId, {}, {}, {});

    // 钻石不是药水，应该不匹配
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(predicate.test(diamond));

    // 铁锭也不是药水
    ItemStack iron(Items::IRON_INGOT, 1);
    EXPECT_FALSE(predicate.test(iron));
}

TEST_F(ItemPredicateTest, PotionMatch_EmptyStack)
{
    // 创建匹配夜视药水的谓词
    ResourceLocation nightVisionId("minecraft", "night_vision");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, nightVisionId, {}, {}, {});

    // 空物品堆不匹配
    ItemStack emptyStack;
    EXPECT_FALSE(predicate.test(emptyStack));
}

// ========== 组合条件测试 ==========

TEST_F(ItemPredicateTest, CombinedItemAndPotion)
{
    // 创建匹配药水物品 + 夜视药水的谓词
    ResourceLocation potionId("minecraft", "potion");
    ResourceLocation nightVisionId("minecraft", "night_vision");
    ItemPredicate predicate(potionId, IntBounds{}, IntBounds{}, nightVisionId, {}, {}, {});

    // 夜视药水
    ItemStack nightVisionPotion = potion::PotionUtils::createPotionItem(potion::Potions::NIGHT_VISION);
    EXPECT_TRUE(predicate.test(nightVisionPotion));

    // 喷溅型夜视药水不是普通药水物品
    ItemStack splashNightVision = potion::PotionUtils::createSplashPotionItem(potion::Potions::NIGHT_VISION);
    EXPECT_FALSE(predicate.test(splashNightVision));
}

TEST_F(ItemPredicateTest, CombinedCountAndPotion)
{
    // 创建匹配数量2 + 治疗药水的谓词
    ResourceLocation healingId("minecraft", "healing");
    ItemPredicate predicate(std::nullopt, IntBounds::exactly(2), IntBounds{}, healingId, {}, {}, {});

    // 数量为2的治疗药水
    ItemStack healing2 = potion::PotionUtils::createPotionItem(potion::Potions::HEALING);
    healing2.setCount(2);
    EXPECT_TRUE(predicate.test(healing2));

    // 数量为1的治疗药水
    ItemStack healing1 = potion::PotionUtils::createPotionItem(potion::Potions::HEALING);
    EXPECT_FALSE(predicate.test(healing1));
}

// ========== JSON 解析测试 ==========

TEST_F(ItemPredicateTest, FromJson_ItemOnly)
{
    nlohmann::json json = R"({"item": "minecraft:diamond"})"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_TRUE(predicate.getItem().has_value());
    EXPECT_EQ(predicate.getItem().value().toString(), "minecraft:diamond");
    EXPECT_TRUE(predicate.isAny() == false);
}

TEST_F(ItemPredicateTest, FromJson_CountOnly)
{
    nlohmann::json json = R"({"count": 16})"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_FALSE(predicate.getCount().isUnbounded());
    EXPECT_TRUE(predicate.getCount().getMin().has_value());
    EXPECT_EQ(predicate.getCount().getMin().value(), 16);
    EXPECT_TRUE(predicate.getCount().getMax().has_value());
    EXPECT_EQ(predicate.getCount().getMax().value(), 16);
}

TEST_F(ItemPredicateTest, FromJson_PotionOnly)
{
    nlohmann::json json = R"({"potion": "minecraft:night_vision"})"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_TRUE(predicate.getPotion().has_value());
    EXPECT_EQ(predicate.getPotion().value().toString(), "minecraft:night_vision");
    EXPECT_FALSE(predicate.isAny());
}

TEST_F(ItemPredicateTest, FromJson_AllFields)
{
    nlohmann::json json = R"({
        "item": "minecraft:potion",
        "count": 1,
        "potion": "minecraft:healing"
    })"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_TRUE(predicate.getItem().has_value());
    EXPECT_FALSE(predicate.getCount().isUnbounded());
    EXPECT_TRUE(predicate.getPotion().has_value());
    EXPECT_FALSE(predicate.isAny());
}

TEST_F(ItemPredicateTest, FromJson_Null)
{
    auto result = ItemPredicate::fromJson(nullptr);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_TRUE(predicate.isAny());
}

// ========== JSON 序列化测试 ==========

TEST_F(ItemPredicateTest, ToJson_ItemOnly)
{
    ResourceLocation itemId("minecraft", "diamond");
    ItemPredicate predicate(itemId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    nlohmann::json json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["item"], "minecraft:diamond");
    EXPECT_FALSE(json.contains("count"));
    EXPECT_FALSE(json.contains("durability"));
    EXPECT_FALSE(json.contains("potion"));
}

TEST_F(ItemPredicateTest, ToJson_PotionOnly)
{
    ResourceLocation potionId("minecraft", "night_vision");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, potionId, {}, {}, {});

    nlohmann::json json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["potion"], "minecraft:night_vision");
    EXPECT_FALSE(json.contains("item"));
}

TEST_F(ItemPredicateTest, ToJson_AnyPredicate)
{
    ItemPredicate predicate;
    nlohmann::json json = predicate.toJson();

    EXPECT_TRUE(json.is_null());
}

TEST_F(ItemPredicateTest, RoundTrip_JsonSerialization)
{
    // 创建谓词
    ResourceLocation itemId("minecraft", "potion");
    ResourceLocation potionId("minecraft", "swiftness");
    ItemPredicate original(itemId, IntBounds::exactly(1), IntBounds{}, potionId, {}, {}, {});

    // 序列化
    nlohmann::json json = original.toJson();

    // 反序列化
    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate restored = result.value();

    // 验证
    EXPECT_EQ(restored.getItem().value().toString(), original.getItem().value().toString());
    EXPECT_TRUE(restored.getCount().getMin().has_value() && original.getCount().getMin().has_value());
    if (restored.getCount().getMin().has_value() && original.getCount().getMin().has_value()) {
        EXPECT_EQ(restored.getCount().getMin().value(), original.getCount().getMin().value());
    }
    EXPECT_EQ(restored.getPotion().value().toString(), original.getPotion().value().toString());
}

// ========== MC 1.16.5 兼容性测试 ==========

TEST_F(ItemPredicateTest, MC1165_StrongPotionVariants)
{
    // MC 1.16.5: 强效药水有不同的ID
    // strong_healing vs healing

    ResourceLocation strongHealingId("minecraft", "strong_healing");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, strongHealingId, {}, {}, {});

    // 强效治疗药水匹配
    ItemStack strongHealing = potion::PotionUtils::createPotionItem(potion::Potions::STRONG_HEALING);
    EXPECT_TRUE(predicate.test(strongHealing));

    // 普通治疗药水不匹配
    ItemStack healing = potion::PotionUtils::createPotionItem(potion::Potions::HEALING);
    EXPECT_FALSE(predicate.test(healing));
}

TEST_F(ItemPredicateTest, MC1165_LongPotionVariants)
{
    // MC 1.16.5: 延长药水有不同的ID
    // long_night_vision vs night_vision

    ResourceLocation longNightVisionId("minecraft", "long_night_vision");
    ItemPredicate predicate(std::nullopt, IntBounds{}, IntBounds{}, longNightVisionId, {}, {}, {});

    // 延长版夜视药水匹配
    ItemStack longNightVision = potion::PotionUtils::createPotionItem(potion::Potions::LONG_NIGHT_VISION);
    EXPECT_TRUE(predicate.test(longNightVision));

    // 普通夜视药水不匹配
    ItemStack nightVision = potion::PotionUtils::createPotionItem(potion::Potions::NIGHT_VISION);
    EXPECT_FALSE(predicate.test(nightVision));
}

// ========== 标签匹配测试 ==========

TEST_F(ItemPredicateTest, TagMatch_ItemInTag)
{
    // 创建匹配 "minecraft:flowers" 标签的谓词
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicate predicate(std::nullopt, tagId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    EXPECT_FALSE(predicate.isAny());
    EXPECT_TRUE(predicate.getTag().has_value());
    EXPECT_EQ(predicate.getTag().value().toString(), "minecraft:flowers");

    // 雏菊应该在花朵标签中
    const Item* dandelion = Items::DANDELION;
    if (dandelion != nullptr) {
        ItemStack dandelionStack(dandelion, 1);
        EXPECT_TRUE(predicate.test(dandelionStack));
    }
}

TEST_F(ItemPredicateTest, TagMatch_ItemNotInTag)
{
    // 创建匹配 "minecraft:flowers" 标签的谓词
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicate predicate(std::nullopt, tagId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    // 钻石不在花朵标签中
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(predicate.test(diamond));
}

TEST_F(ItemPredicateTest, TagMatch_UnknownTagRejectsAll)
{
    // 未知标签不匹配任何物品
    ResourceLocation unknownTag("minecraft", "nonexistent_tag_xyz");
    ItemPredicate predicate(std::nullopt, unknownTag, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(predicate.test(diamond));
}

TEST_F(ItemPredicateTest, TagMatch_EmptyStack)
{
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicate predicate(std::nullopt, tagId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    // 空物品堆不匹配标签谓词
    ItemStack emptyStack;
    EXPECT_FALSE(predicate.test(emptyStack));
}

TEST_F(ItemPredicateTest, TagAndItemMutuallyExclusive)
{
    // 同时指定 item 和 tag 时，item 优先（与 MC Java 行为一致）
    // 当两者都存在时，按顺序检查：先 item，后 tag
    ResourceLocation itemId("minecraft", "diamond");
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicate predicate(itemId, tagId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    // 钻石匹配物品ID但不在花朵标签中——但 item 检查先通过
    // 注意：item 和 tag 检查是 AND 关系（都需要通过），因此钻石不在花朵标签中，应不匹配
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(predicate.test(diamond));

    // 雏菊在花朵标签中但不是钻石物品——item 检查不通过
    const Item* dandelion = Items::DANDELION;
    if (dandelion != nullptr) {
        ItemStack dandelionStack(dandelion, 1);
        EXPECT_FALSE(predicate.test(dandelionStack));
    }
}

// ========== 标签 JSON 解析/序列化测试 ==========

TEST_F(ItemPredicateTest, FromJson_TagOnly)
{
    nlohmann::json json = R"({"tag": "minecraft:flowers"})"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_FALSE(predicate.isAny());
    EXPECT_FALSE(predicate.getItem().has_value());
    EXPECT_TRUE(predicate.getTag().has_value());
    EXPECT_EQ(predicate.getTag().value().toString(), "minecraft:flowers");
}

TEST_F(ItemPredicateTest, FromJson_ItemAndTag)
{
    // 同时指定 item 和 tag（虽然不常见，但 JSON 允许）
    nlohmann::json json = R"({"item": "minecraft:diamond", "tag": "minecraft:logs"})"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = result.value();
    EXPECT_TRUE(predicate.getItem().has_value());
    EXPECT_TRUE(predicate.getTag().has_value());
    EXPECT_EQ(predicate.getItem().value().toString(), "minecraft:diamond");
    EXPECT_EQ(predicate.getTag().value().toString(), "minecraft:logs");
}

TEST_F(ItemPredicateTest, ToJson_TagOnly)
{
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicate predicate(std::nullopt, tagId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    nlohmann::json json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_FALSE(json.contains("item"));
    EXPECT_TRUE(json.contains("tag"));
    EXPECT_EQ(json["tag"], "minecraft:flowers");
}

TEST_F(ItemPredicateTest, RoundTrip_TagSerialization)
{
    ResourceLocation tagId("minecraft", "logs");
    ItemPredicate original(std::nullopt, tagId, IntBounds{}, IntBounds{}, std::nullopt, {}, {}, {});

    // 序列化
    nlohmann::json json = original.toJson();

    // 反序列化
    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate restored = result.value();
    EXPECT_TRUE(restored.getTag().has_value());
    EXPECT_EQ(restored.getTag().value().toString(), "minecraft:logs");
    EXPECT_FALSE(restored.getItem().has_value());
}
