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

// 1.21.11 数据组件模型下 ItemStack 的核心往返测试：
//   - toNbt/fromNbt（{id,count,components} 格式）全组件保真
//   - toItemStackView/fromItemStackView（wire DataComponentPatch 字节）全组件保真
//   - 无非默认组件时不写出 components 段
//   - 空堆往返为空堆

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TextParser.hpp"

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::nbt;
using namespace mc::nbt::tags;

namespace {

/// 构造一个带全部 9 个已落地组件的 ItemStack（用于往返保真验证）
ItemStack makeFullyComponentedStack(const Item* sword)
{
    ItemStack stack(*sword, 1);
    stack.setDamage(7);
    stack.setRepairCost(3);
    stack.setCustomName("Hero Blade");
    std::vector<std::unique_ptr<text::ITextComponent>> lore;
    lore.push_back(text::TextParser::parse("line one"));
    lore.push_back(text::TextParser::parse("line two"));
    stack.setLore(std::move(lore));
    stack.addEnchantment("minecraft:sharpness", 5);
    stack.addEnchantment("minecraft:unbreaking", 3);
    stack.setPotionId("minecraft:water");
    stack.setCanPlaceOn(AdventureModePredicate({"minecraft:stone", "minecraft:dirt"}));
    stack.setCanDestroy(AdventureModePredicate({"minecraft:cobblestone"}));
    auto& tag = stack.getOrCreateTag();
    tag = nlohmann::json::object();
    tag["display"]["color"] = 0xFF0000;
    tag["custom"]["n"] = 42;
    return stack;
}

} // namespace

// ============================================================================
// toNbt / fromNbt：1.21.11 数据组件格式往返
// ============================================================================

class ItemStackComponentRoundTripTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_sword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
        ASSERT_NE(m_sword, nullptr);
        // JavaItemIdMap 须在 Items::initialize 后初始化；toItemStackView/fromItemStackView
        // 边界用它翻译项目内部 id ↔ vanilla wire id。
        ASSERT_TRUE(network::backend::java::JavaItemIdMap::instance().initialize().success());
        m_swordJavaId = network::backend::java::JavaItemIdMap::instance().toJavaRegistryId(*m_sword);
        ASSERT_NE(m_swordJavaId, 0u) << "diamond_sword 须命中 vanilla item 表";
    }

    const Item* m_sword = nullptr;
    u32 m_swordJavaId = 0;
};

TEST_F(ItemStackComponentRoundTripTest, NbtRoundTrip_PreservesAllComponents)
{
    const auto original = makeFullyComponentedStack(m_sword);

    compound_tag tag;
    original.toNbt(tag);

    // 1.21.11 顶层键：id / count / components
    ASSERT_NE(tag.value.find("id"), tag.value.end());
    ASSERT_NE(tag.value.find("count"), tag.value.end());
    ASSERT_NE(tag.value.find("components"), tag.value.end()) << "有非默认组件时必须写出 components 段";

    auto result = ItemStack::fromNbt(tag);
    ASSERT_TRUE(result.success()) << "fromNbt 应成功";

    const auto& restored = result.value();
    EXPECT_EQ(restored.getItem(), m_sword);
    EXPECT_EQ(restored.getCount(), 1);
    EXPECT_EQ(restored.getDamage(), 7);
    EXPECT_EQ(restored.getRepairCost(), 3);
    EXPECT_EQ(restored.getCustomName(), "Hero Blade");
    ASSERT_EQ(restored.getLore().size(), 2u);
    EXPECT_EQ(restored.getLore()[0]->getUnformattedText(), "line one");
    EXPECT_EQ(restored.getLore()[1]->getUnformattedText(), "line two");
    EXPECT_EQ(restored.getEnchantments().getLevel("minecraft:sharpness"), 5);
    EXPECT_EQ(restored.getEnchantments().getLevel("minecraft:unbreaking"), 3);
    EXPECT_EQ(restored.getPotionId(), "minecraft:water");
    ASSERT_EQ(restored.getCanPlaceOn().getPredicates().size(), 2u);
    EXPECT_EQ(restored.getCanPlaceOn().getPredicates()[0], "minecraft:stone");
    EXPECT_EQ(restored.getCanPlaceOn().getPredicates()[1], "minecraft:dirt");
    ASSERT_EQ(restored.getCanDestroy().getPredicates().size(), 1u);
    EXPECT_EQ(restored.getCanDestroy().getPredicates()[0], "minecraft:cobblestone");
    // CustomData 经 JSON→NBT→JSON 往返：display.color 与 custom.n 应保留
    const auto* restoredTag = restored.getTag();
    ASSERT_NE(restoredTag, nullptr);
    EXPECT_EQ(restoredTag->at("display").at("color").get<i32>(), 0xFF0000);
    EXPECT_EQ(restoredTag->at("custom").at("n").get<i32>(), 42);
}

TEST_F(ItemStackComponentRoundTripTest, NbtRoundTrip_NoComponents_OmitsComponentsSection)
{
    ItemStack plain(*m_sword, 1);

    compound_tag tag;
    plain.toNbt(tag);

    EXPECT_EQ(tag.value.find("components"), tag.value.end()) << "无非默认组件时不应写出 components 段";
    ASSERT_NE(tag.value.find("count"), tag.value.end());

    auto result = ItemStack::fromNbt(tag);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getItem(), m_sword);
    EXPECT_EQ(result.value().getCount(), 1);
    EXPECT_EQ(result.value().getDamage(), 0);
}

TEST_F(ItemStackComponentRoundTripTest, NbtRoundTrip_EmptyStack_RoundTripsToAir)
{
    ItemStack empty;

    compound_tag tag;
    empty.toNbt(tag);

    ASSERT_NE(tag.value.find("id"), tag.value.end());
    const auto& idTag = dynamic_cast<const string_tag&>(*tag.value.at("id"));
    EXPECT_EQ(idTag.value, "minecraft:air");

    auto result = ItemStack::fromNbt(tag);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isEmpty());
}

// ============================================================================
// toItemStackView / fromItemStackView：wire DataComponentPatch 字节往返
// ============================================================================

TEST_F(ItemStackComponentRoundTripTest, WireRoundTrip_PreservesAllComponents)
{
    const auto original = makeFullyComponentedStack(m_sword);

    const auto view = network::ir::toItemStackView(original);
    EXPECT_EQ(view.itemId, m_swordJavaId);
    EXPECT_EQ(view.count, 1);
    EXPECT_FALSE(view.componentsPatch.empty()) << "有非默认组件时 componentsPatch 须非空";

    auto result = network::ir::fromItemStackView(view);
    ASSERT_TRUE(result.success()) << "fromItemStackView 应成功";

    const auto& restored = result.value();
    EXPECT_EQ(restored.getItem(), m_sword);
    EXPECT_EQ(restored.getCount(), 1);
    EXPECT_EQ(restored.getDamage(), 7);
    EXPECT_EQ(restored.getRepairCost(), 3);
    EXPECT_EQ(restored.getCustomName(), "Hero Blade");
    ASSERT_EQ(restored.getLore().size(), 2u);
    EXPECT_EQ(restored.getEnchantments().getLevel("minecraft:sharpness"), 5);
    EXPECT_EQ(restored.getEnchantments().getLevel("minecraft:unbreaking"), 3);
    EXPECT_EQ(restored.getPotionId(), "minecraft:water");
    ASSERT_EQ(restored.getCanPlaceOn().getPredicates().size(), 2u);
    ASSERT_EQ(restored.getCanDestroy().getPredicates().size(), 1u);
    const auto* restoredTag = restored.getTag();
    ASSERT_NE(restoredTag, nullptr);
    EXPECT_EQ(restoredTag->at("display").at("color").get<i32>(), 0xFF0000);
    EXPECT_EQ(restoredTag->at("custom").at("n").get<i32>(), 42);
}

TEST_F(ItemStackComponentRoundTripTest, WireRoundTrip_NoComponents_HasEmptyPatch)
{
    ItemStack plain(*m_sword, 2);

    const auto view = network::ir::toItemStackView(plain);
    EXPECT_EQ(view.itemId, m_swordJavaId);
    EXPECT_EQ(view.count, 2);
    // 无组件的非空栈：patch 为空，但 wire 须写出空 patch 的表示 0x00 0x00
    // （vanilla DataComponentPatch.STREAM_CODEC 对空 patch 写 VarInt(0)+VarInt(0)）。
    EXPECT_EQ(view.componentsPatch, (std::vector<u8>{0x00, 0x00}));

    auto result = network::ir::fromItemStackView(view);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getItem(), m_sword);
    EXPECT_EQ(result.value().getCount(), 2);
    EXPECT_EQ(result.value().getDamage(), 0);
}

TEST_F(ItemStackComponentRoundTripTest, WireRoundTrip_EmptyStack_ProducesEmptyView)
{
    ItemStack empty;
    const auto view = network::ir::toItemStackView(empty);
    EXPECT_EQ(view.itemId, 0u);
    EXPECT_EQ(view.count, 0);
    EXPECT_TRUE(view.componentsPatch.empty());

    auto result = network::ir::fromItemStackView(view);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isEmpty());
}
