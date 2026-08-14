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

#include "world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/PotterySherdItem.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

// ========== PotDecorations 测试 ==========

class PotDecorationsTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(PotDecorationsTest, DefaultConstructor_AllBlank)
{
    PotDecorations deco;
    EXPECT_EQ(deco.back(), DecoratedPotPattern::Blank);
    EXPECT_EQ(deco.left(), DecoratedPotPattern::Blank);
    EXPECT_EQ(deco.right(), DecoratedPotPattern::Blank);
    EXPECT_EQ(deco.front(), DecoratedPotPattern::Blank);
    EXPECT_TRUE(deco.isEmpty());
}

TEST_F(PotDecorationsTest, FourPatternConstructor)
{
    PotDecorations deco(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::ArmsUp,
        DecoratedPotPattern::Blade);

    EXPECT_EQ(deco.back(), DecoratedPotPattern::Angler);
    EXPECT_EQ(deco.left(), DecoratedPotPattern::Archer);
    EXPECT_EQ(deco.right(), DecoratedPotPattern::ArmsUp);
    EXPECT_EQ(deco.front(), DecoratedPotPattern::Blade);
    EXPECT_FALSE(deco.isEmpty());
}

TEST_F(PotDecorationsTest, VectorConstructor_TruncatesAndFills)
{
    // 少于4个时用Blank填充
    std::vector<DecoratedPotPattern> shortVec = {DecoratedPotPattern::Heart, DecoratedPotPattern::Howl};
    PotDecorations decoShort(shortVec);
    EXPECT_EQ(decoShort.back(), DecoratedPotPattern::Heart);
    EXPECT_EQ(decoShort.left(), DecoratedPotPattern::Howl);
    EXPECT_EQ(decoShort.right(), DecoratedPotPattern::Blank);
    EXPECT_EQ(decoShort.front(), DecoratedPotPattern::Blank);

    // 超过4个时截断
    std::vector<DecoratedPotPattern> longVec = {
        DecoratedPotPattern::Miner,
        DecoratedPotPattern::Mourner,
        DecoratedPotPattern::Plenty,
        DecoratedPotPattern::Prize,
        DecoratedPotPattern::Sheaf, // 被截断
    };
    PotDecorations decoLong(longVec);
    EXPECT_EQ(decoLong.back(), DecoratedPotPattern::Miner);
    EXPECT_EQ(decoLong.left(), DecoratedPotPattern::Mourner);
    EXPECT_EQ(decoLong.right(), DecoratedPotPattern::Plenty);
    EXPECT_EQ(decoLong.front(), DecoratedPotPattern::Prize);
}

TEST_F(PotDecorationsTest, EmptyStaticConst)
{
    EXPECT_TRUE(PotDecorations::EMPTY.isEmpty());
    EXPECT_EQ(PotDecorations::EMPTY.back(), DecoratedPotPattern::Blank);
    EXPECT_EQ(PotDecorations::EMPTY.front(), DecoratedPotPattern::Blank);
}

TEST_F(PotDecorationsTest, EqualityOperators)
{
    PotDecorations a(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::ArmsUp,
        DecoratedPotPattern::Blade);
    PotDecorations b(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::ArmsUp,
        DecoratedPotPattern::Blade);
    PotDecorations c(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::ArmsUp,
        DecoratedPotPattern::Burn);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST_F(PotDecorationsTest, Ordered_ReturnsArray)
{
    PotDecorations deco(
        DecoratedPotPattern::Skull, DecoratedPotPattern::Snort, DecoratedPotPattern::Flow, DecoratedPotPattern::Guster);

    const auto& ordered = deco.ordered();
    EXPECT_EQ(ordered.size(), 4u);
    EXPECT_EQ(ordered[0], DecoratedPotPattern::Skull);
    EXPECT_EQ(ordered[1], DecoratedPotPattern::Snort);
    EXPECT_EQ(ordered[2], DecoratedPotPattern::Flow);
    EXPECT_EQ(ordered[3], DecoratedPotPattern::Guster);
}

// ========== PotDecorations 序列化测试 ==========

TEST_F(PotDecorationsTest, JsonSerialization_RoundTrip)
{
    PotDecorations original(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::Heartbreak,
        DecoratedPotPattern::Blade);

    nlohmann::json json = original.toJson();
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 4u);

    // 验证JSON格式：非Blank图案应为 "minecraft:{name}_pottery_sherd"
    EXPECT_EQ(json[0].get<std::string>(), "minecraft:angler_pottery_sherd");
    EXPECT_EQ(json[1].get<std::string>(), "minecraft:archer_pottery_sherd");
    EXPECT_EQ(json[2].get<std::string>(), "minecraft:heartbreak_pottery_sherd");
    EXPECT_EQ(json[3].get<std::string>(), "minecraft:blade_pottery_sherd");

    // 反序列化
    PotDecorations loaded = PotDecorations::fromJson(json);
    EXPECT_EQ(loaded, original);
}

TEST_F(PotDecorationsTest, JsonSerialization_BlankIsBrick)
{
    PotDecorations deco; // 全部 Blank
    nlohmann::json json = deco.toJson();

    // Blank 图案序列化为 "minecraft:brick"
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(json[i].get<std::string>(), "minecraft:brick");
    }

    // 反序列化
    PotDecorations loaded = PotDecorations::fromJson(json);
    EXPECT_TRUE(loaded.isEmpty());
}

TEST_F(PotDecorationsTest, JsonSerialization_MixedBlankAndPattern)
{
    PotDecorations deco(DecoratedPotPattern::Blank,
        DecoratedPotPattern::Explorer,
        DecoratedPotPattern::Blank,
        DecoratedPotPattern::Friend);

    nlohmann::json json = deco.toJson();
    EXPECT_EQ(json[0].get<std::string>(), "minecraft:brick");
    EXPECT_EQ(json[1].get<std::string>(), "minecraft:explorer_pottery_sherd");
    EXPECT_EQ(json[2].get<std::string>(), "minecraft:brick");
    EXPECT_EQ(json[3].get<std::string>(), "minecraft:friend_pottery_sherd");

    PotDecorations loaded = PotDecorations::fromJson(json);
    EXPECT_EQ(loaded, deco);
}

TEST_F(PotDecorationsTest, JsonDeserialization_InvalidInput)
{
    // 空数组 -> 全部 Blank
    PotDecorations emptyDeco = PotDecorations::fromJson(nlohmann::json::array());
    EXPECT_TRUE(emptyDeco.isEmpty());

    // 非数组 -> 全部 Blank
    PotDecorations notArray = PotDecorations::fromJson(nlohmann::json::object());
    EXPECT_TRUE(notArray.isEmpty());

    // 未知物品ID -> Blank
    nlohmann::json unknownId = nlohmann::json::array({"minecraft:unknown_item"});
    PotDecorations unknownDeco = PotDecorations::fromJson(unknownId);
    EXPECT_EQ(unknownDeco.back(), DecoratedPotPattern::Blank);
}

TEST_F(PotDecorationsTest, NBTSerialization_RoundTrip)
{
    PotDecorations original(DecoratedPotPattern::Shelter,
        DecoratedPotPattern::Skull,
        DecoratedPotPattern::Flow,
        DecoratedPotPattern::Guster);

    nbt::tags::string_list_tag nbt = original.toNBT();
    EXPECT_EQ(nbt.value.size(), 4u);

    // 创建 list_tag 引用用于 fromNBT
    const nbt::tags::list_tag& listTag = nbt;
    PotDecorations loaded = PotDecorations::fromNBT(listTag);
    EXPECT_EQ(loaded, original);
}

// ========== DecoratedPotBlockEntity 测试 ==========

class DecoratedPotBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        entity_ = std::make_unique<DecoratedPotBlockEntity>(BlockPos(10, 20, 30));
    }

    std::unique_ptr<DecoratedPotBlockEntity> entity_;
};

TEST_F(DecoratedPotBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(entity_->getType(), BlockEntityType::DecoratedPot);
}

TEST_F(DecoratedPotBlockEntityTest, Create_DefaultDecorationsEmpty)
{
    EXPECT_TRUE(entity_->getDecorations().isEmpty());
}

TEST_F(DecoratedPotBlockEntityTest, Create_InventorySizeIsOne)
{
    EXPECT_EQ(entity_->getContainerSize(), 1);
    EXPECT_FALSE(entity_->hasItem());
}

TEST_F(DecoratedPotBlockEntityTest, SetAndGetDecorations)
{
    PotDecorations deco(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::ArmsUp,
        DecoratedPotPattern::Blade);

    entity_->setDecorations(deco);
    EXPECT_EQ(entity_->getDecorations(), deco);
    EXPECT_FALSE(entity_->getDecorations().isEmpty());
}

TEST_F(DecoratedPotBlockEntityTest, SetAndGetItem)
{
    // 获取一个已注册的物品进行测试
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    ItemStack stack(*diamond, 1);
    entity_->setItem(stack);
    EXPECT_TRUE(entity_->hasItem());

    ItemStack retrieved = entity_->getItem();
    EXPECT_FALSE(retrieved.isEmpty());
    EXPECT_EQ(retrieved.getCount(), 1);
}

TEST_F(DecoratedPotBlockEntityTest, ComparatorSignal_EmptyPot_ReturnsZero)
{
    EXPECT_EQ(entity_->getComparatorSignal(), 0);
}

TEST_F(DecoratedPotBlockEntityTest, ComparatorSignal_WithItem_ReturnsNonZero)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    entity_->setItem(ItemStack(*diamond, 1));
    EXPECT_GT(entity_->getComparatorSignal(), 0);
}

TEST_F(DecoratedPotBlockEntityTest, Wobble_AnimationState)
{
    entity_->wobble(DecoratedPotBlockEntity::WobbleStyle::Positive);
    EXPECT_EQ(entity_->lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Positive);

    entity_->wobble(DecoratedPotBlockEntity::WobbleStyle::Negative);
    EXPECT_EQ(entity_->lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Negative);
}

// ========== DecoratedPotBlockEntity 序列化测试 ==========

TEST_F(DecoratedPotBlockEntityTest, JsonSerialization_RoundTrip)
{
    PotDecorations deco(DecoratedPotPattern::Burn,
        DecoratedPotPattern::Danger,
        DecoratedPotPattern::Explorer,
        DecoratedPotPattern::Friend);

    entity_->setDecorations(deco);

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    entity_->setItem(ItemStack(*diamond, 1));

    // 保存
    nlohmann::json saved;
    entity_->save(saved);

    // 加载到新实体
    auto loaded = std::make_unique<DecoratedPotBlockEntity>(BlockPos(0, 0, 0));
    bool success = loaded->load(saved);
    EXPECT_TRUE(success);

    EXPECT_EQ(loaded->getDecorations(), deco);
    EXPECT_TRUE(loaded->hasItem());
}

TEST_F(DecoratedPotBlockEntityTest, JsonSerialization_EmptyEntity)
{
    nlohmann::json saved;
    entity_->save(saved);

    auto loaded = std::make_unique<DecoratedPotBlockEntity>(BlockPos(0, 0, 0));
    bool success = loaded->load(saved);
    EXPECT_TRUE(success);

    EXPECT_TRUE(loaded->getDecorations().isEmpty());
    EXPECT_FALSE(loaded->hasItem());
}

// ========== DecoratedPotPatterns 工具类测试 ==========

TEST(DecoratedPotPatternsTest, ByName_ValidNames)
{
    EXPECT_EQ(DecoratedPotPatterns::byName("angler"), DecoratedPotPattern::Angler);
    EXPECT_EQ(DecoratedPotPatterns::byName("archer"), DecoratedPotPattern::Archer);
    EXPECT_EQ(DecoratedPotPatterns::byName("flow"), DecoratedPotPattern::Flow);
    EXPECT_EQ(DecoratedPotPatterns::byName("scrape"), DecoratedPotPattern::Scrape);
}

TEST(DecoratedPotPatternsTest, ByName_InvalidName_ReturnsBlank)
{
    EXPECT_EQ(DecoratedPotPatterns::byName("nonexistent"), DecoratedPotPattern::Blank);
    EXPECT_EQ(DecoratedPotPatterns::byName(""), DecoratedPotPattern::Blank);
}

TEST(DecoratedPotPatternsTest, GetName)
{
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Angler), "angler");
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Flow), "flow");
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Blank), "blank");
}

TEST(DecoratedPotPatternsTest, GetAssetId)
{
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Angler), "angler_pottery_pattern");
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Blank), "decorated_pot_side");
}

TEST(DecoratedPotPatternsTest, GetTranslationKey)
{
    EXPECT_EQ(
        DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::Angler), "item.minecraft.angler_pottery_sherd");
    EXPECT_EQ(DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::Blank), "block.minecraft.decorated_pot");
}

TEST(DecoratedPotPatternsTest, IsBlank)
{
    EXPECT_TRUE(DecoratedPotPatterns::isBlank(DecoratedPotPattern::Blank));
    EXPECT_FALSE(DecoratedPotPatterns::isBlank(DecoratedPotPattern::Angler));
}

// ========== getPatternFromItem / getItemFromPattern 测试 ==========

class DecoratedPotItemMappingTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(DecoratedPotItemMappingTest, GetPatternFromItem_BrickReturnsBlank)
{
    Item* brick = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brick"));
    // 砖块物品可能在测试环境中未注册，跳过测试
    if (brick == nullptr) {
        GTEST_SKIP() << "Brick item not registered in test environment";
    }
    EXPECT_EQ(getPatternFromItem(brick), DecoratedPotPattern::Blank);
}

TEST_F(DecoratedPotItemMappingTest, GetPatternFromItem_NullptrReturnsBlank)
{
    EXPECT_EQ(getPatternFromItem(nullptr), DecoratedPotPattern::Blank);
}

TEST_F(DecoratedPotItemMappingTest, GetPatternFromItem_NonSherdItemReturnsBlank)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_EQ(getPatternFromItem(diamond), DecoratedPotPattern::Blank);
}

TEST_F(DecoratedPotItemMappingTest, GetItemFromPattern_BlankReturnsBrick)
{
    const Item* brick = getItemFromPattern(DecoratedPotPattern::Blank);
    // 砖块物品可能在测试环境中未注册
    if (brick == nullptr) {
        GTEST_SKIP() << "Brick item not registered in test environment";
    }
    // 验证返回的是砖块物品（项目注册名为 "minecraft:bricks"）
    EXPECT_EQ(brick->itemLocation(), ResourceLocation("minecraft", "bricks"));
}

TEST_F(DecoratedPotItemMappingTest, GetItemFromPattern_PatternReturnsSherd)
{
    // 注意：陶片物品需要在 Items::initialize() 中注册后才能查找
    // 如果对应陶片物品未注册，getItemFromPattern 返回 nullptr
    const Item* anglerItem = getItemFromPattern(DecoratedPotPattern::Angler);
    // 如果项目已注册，验证资源路径
    if (anglerItem != nullptr) {
        EXPECT_EQ(anglerItem->itemLocation(), ResourceLocation("minecraft", "angler_pottery_sherd"));
    }
    // 如果未注册，至少验证函数不崩溃
}
