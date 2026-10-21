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
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/component/Fireworks.hpp"
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
#include "common/world/block/BlockRegistry.hpp"

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::nbt;
using namespace mc::nbt::tags;

namespace {

/// 构造一个带全部已落地组件的 ItemStack（用于往返保真验证）
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
    item::component::FireworkExplosion explosion{};
    explosion.shape = item::component::FireworkShape::Creeper;
    explosion.colors = {0x123456, 0xABCDEF};
    explosion.fadeColors = {0x654321};
    explosion.hasTrail = true;
    explosion.hasTwinkle = true;
    stack.setFireworkExplosion(std::move(explosion));
    item::component::Fireworks fireworks{};
    fireworks.flightDuration = 2;
    fireworks.explosions.push_back(*stack.getFireworkExplosion());
    fireworks.explosions.emplace_back();
    stack.setFireworks(std::move(fireworks));
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

TEST_F(ItemStackComponentRoundTripTest, ToolComponentDrivesMining)
{
    // tool 组件（数据驱动挖掘规则）覆盖物品基础挖掘行为
    const Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);
    ItemStack stack(*stick, 1);

    item::component::ToolPayload tool;
    item::component::ToolRule rule;
    rule.blocks.push_back(ResourceLocation("minecraft:stone"));
    rule.speed = 8.0f;
    rule.correctForDrops = true;
    tool.rules.push_back(rule);
    tool.defaultMiningSpeed = 1.0f;
    stack.setTool(tool);

    const Block* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    const BlockState& state = stone->defaultState();
    EXPECT_FLOAT_EQ(8.0f, stack.getDestroySpeed(state));
    EXPECT_TRUE(stack.canHarvestBlock(state));

    // 规则未覆盖的方块使用默认速度；掉落规则未命中 -> 不掉落
    // （土本身不需要工具，故另用需要工具的石头验证规则命中时的掉落判定）
    const Block* oakLog = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:oak_log"));
    ASSERT_NE(oakLog, nullptr);
    EXPECT_FLOAT_EQ(1.0f, stack.getDestroySpeed(oakLog->defaultState()));
    EXPECT_FALSE(stack.canHarvestBlock(oakLog->defaultState()));
}

TEST_F(ItemStackComponentRoundTripTest, ContainerAndChargedProjectilesRoundTrip)
{
    // 容器与充能投射物由 ItemStack 独立字段承载，经组件补丁导出/应用来回一致
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    const Item* crossbow = ItemRegistry::instance().getItem(ResourceLocation("minecraft:crossbow"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(crossbow, nullptr);
    ItemStack stack(*crossbow, 1);

    item::component::ItemContainerContentsPayload contents;
    contents.slots.push_back(
        item::component::ItemContainerContentsPayload::Slot{3, std::make_unique<ItemStack>(*diamond, 5)});
    stack.setContainerContents(std::move(contents));
    item::component::ChargedProjectilesPayload projectiles;
    projectiles.items.push_back(std::make_unique<ItemStack>(*diamond, 2));
    stack.setChargedProjectiles(std::move(projectiles));

    // 拷贝后共享同一载荷（不可变快速路径）
    ItemStack copy = stack;
    ASSERT_NE(copy.getContainerContents(), nullptr);
    EXPECT_EQ(copy.getContainerContents(), stack.getContainerContents());
    ASSERT_NE(copy.getChargedProjectiles(), nullptr);
    EXPECT_EQ(2, copy.getChargedProjectiles()->items[0]->getCount());

    // 组件补丁导出：容器/投射物载荷含 ItemStack 指针，不写入补丁
    const auto patch = stack.toComponentPatch();
    for (const auto& entry : patch.added()) {
        EXPECT_NE(entry.typeId, item::component::componentTypeId(item::component::DataComponentType::Container));
        EXPECT_NE(
            entry.typeId, item::component::componentTypeId(item::component::DataComponentType::ChargedProjectiles));
    }
    EXPECT_EQ(copy.getContainerContents()->slots[0].index, 3);
}

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
    // firework_explosion / fireworks 组件保真
    const auto* restoredExplosion = restored.getFireworkExplosion();
    ASSERT_NE(restoredExplosion, nullptr);
    EXPECT_EQ(restoredExplosion->shape, item::component::FireworkShape::Creeper);
    EXPECT_EQ(restoredExplosion->colors, (std::vector<i32>{0x123456, 0xABCDEF}));
    EXPECT_EQ(restoredExplosion->fadeColors, (std::vector<i32>{0x654321}));
    EXPECT_TRUE(restoredExplosion->hasTrail);
    EXPECT_TRUE(restoredExplosion->hasTwinkle);
    const auto* restoredFireworks = restored.getFireworks();
    ASSERT_NE(restoredFireworks, nullptr);
    EXPECT_EQ(restoredFireworks->flightDuration, 2);
    ASSERT_EQ(restoredFireworks->explosions.size(), 2u);
    EXPECT_EQ(restoredFireworks->explosions[0], *restoredExplosion);
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
    // CanPlaceOn/CanBreak 经 wire roundTrip 降级为空谓词：4c 起 AdventureModePredicate
    // wire 写空 list（项目 vector<string> 与 vanilla 结构化 BlockPredicate 不兼容，降级
    // 保互通不崩）。NBT 路径（NbtRoundTrip）不走 wire 降级，谓词保真。
    EXPECT_EQ(restored.getCanPlaceOn().getPredicates().size(), 0u);
    EXPECT_EQ(restored.getCanDestroy().getPredicates().size(), 0u);
    // firework_explosion / fireworks 组件经 wire 保真（VarInt shape + INT 颜色列表 + Bool 标志）
    const auto* restoredExplosion = restored.getFireworkExplosion();
    ASSERT_NE(restoredExplosion, nullptr);
    EXPECT_EQ(restoredExplosion->shape, item::component::FireworkShape::Creeper);
    EXPECT_EQ(restoredExplosion->colors, (std::vector<i32>{0x123456, 0xABCDEF}));
    EXPECT_EQ(restoredExplosion->fadeColors, (std::vector<i32>{0x654321}));
    EXPECT_TRUE(restoredExplosion->hasTrail);
    EXPECT_TRUE(restoredExplosion->hasTwinkle);
    const auto* restoredFireworks = restored.getFireworks();
    ASSERT_NE(restoredFireworks, nullptr);
    EXPECT_EQ(restoredFireworks->flightDuration, 2);
    ASSERT_EQ(restoredFireworks->explosions.size(), 2u);
    EXPECT_EQ(restoredFireworks->explosions[0], *restoredExplosion);
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

TEST_F(ItemStackComponentRoundTripTest, CopyPreservesRepairCostAndAllComponentStateIndependently)
{
    const auto original = makeFullyComponentedStack(m_sword);
    auto copied = original.copy();
    EXPECT_EQ(copied, original);
    EXPECT_EQ(copied.getRepairCost(), 3);
    EXPECT_EQ(
        network::ir::toItemStackView(copied).componentsPatch, network::ir::toItemStackView(original).componentsPatch);
    copied.setRepairCost(7);
    copied.setCustomName("Changed");
    copied.getOrCreateTag()["custom"]["n"] = 99;
    EXPECT_EQ(original.getRepairCost(), 3);
    EXPECT_EQ(original.getCustomName(), "Hero Blade");
    EXPECT_EQ(original.getTag()->at("custom").at("n"), 42);
    EXPECT_NE(copied, original);
}

// ============================================================================
// block_entity_data 组件
// ============================================================================

TEST_F(ItemStackComponentRoundTripTest, BlockEntityData_DefaultsToAbsent)
{
    ItemStack stack(*m_sword, 1);
    EXPECT_FALSE(stack.hasBlockEntityData());
    EXPECT_TRUE(stack.getBlockEntityData().empty());
}

TEST_F(ItemStackComponentRoundTripTest, BlockEntityData_SetAndReadBack)
{
    ItemStack stack(*m_sword, 1);
    nlohmann::json data = nlohmann::json::object();
    data["Command"] = "say hi";
    data["auto"] = 1;
    stack.setBlockEntityData(data);

    EXPECT_TRUE(stack.hasBlockEntityData());
    EXPECT_EQ(stack.getBlockEntityData().at("Command"), "say hi");
    EXPECT_EQ(stack.getBlockEntityData().at("auto"), 1);

    // 传空对象表示移除。
    stack.setBlockEntityData(nlohmann::json::object());
    EXPECT_FALSE(stack.hasBlockEntityData());
}

TEST_F(ItemStackComponentRoundTripTest, BlockEntityData_SurvivesComponentPatchRoundTrip)
{
    ItemStack stack(*m_sword, 1);
    nlohmann::json data = nlohmann::json::object();
    data["Command"] = "time set day";
    stack.setBlockEntityData(data);

    const auto patch = stack.toComponentPatch();
    bool found = false;
    for (const auto& entry : patch.added()) {
        if (entry.typeId == item::component::componentTypeId(item::component::DataComponentType::BlockEntityData)) {
            found = true;
        }
    }
    ASSERT_TRUE(found) << "block_entity_data 必须进入组件补丁";

    ItemStack restored(*m_sword, 1);
    restored.applyComponentPatch(patch);
    EXPECT_TRUE(restored.hasBlockEntityData());
    EXPECT_EQ(restored.getBlockEntityData().at("Command"), "time set day");
}

TEST_F(ItemStackComponentRoundTripTest, BlockEntityData_CopyIsIndependent)
{
    ItemStack stack(*m_sword, 1);
    nlohmann::json data = nlohmann::json::object();
    data["Command"] = "say a";
    stack.setBlockEntityData(data);

    ItemStack copied = stack.copy();
    nlohmann::json changed = nlohmann::json::object();
    changed["Command"] = "say b";
    copied.setBlockEntityData(changed);

    EXPECT_EQ(stack.getBlockEntityData().at("Command"), "say a");
    EXPECT_EQ(copied.getBlockEntityData().at("Command"), "say b");
}
