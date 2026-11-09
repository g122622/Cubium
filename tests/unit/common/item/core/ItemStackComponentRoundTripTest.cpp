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
#include "common/item/component/DataComponentPayloadCodec.hpp"
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
#include "common/world/block/registry/BlockRegistry.hpp"

#include <array>
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
    item::component::EquippablePayload equippable{};
    equippable.slot = EquipmentSlot::Chest;
    equippable.equipSound = "minecraft:item.armor.equip_diamond";
    equippable.damageOnHurt = false;
    equippable.equipOnInteract = true;
    stack.setEquippable(std::move(equippable));
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
        Items::ensureInitialized();
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

// provides_trim_material 与 chicken/variant 都是 EitherHolder 形态的原始 JSON 组件。
// 它们此前只在 ItemStack::rawComponents 里登记，DataComponentPayloadCodec 的编解码 switch
// 里没有对应分支——编码会落到 default 当作空 compound 写出，NBT 往返后载荷静默丢失。
TEST_F(ItemStackComponentRoundTripTest, NbtRoundTrip_PreservesEitherHolderComponents)
{
    const Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);
    ItemStack stack(*stick, 1);

    // 原版形态：{"id": "minecraft:gold"}（引用注册表条目）
    const nlohmann::json trimMaterial = {{"id", "minecraft:gold"}};
    const nlohmann::json chickenVariant = {{"id", "minecraft:warm"}};
    stack.setComponent(
        item::component::DataComponentType::ProvidesTrimMaterial, item::component::DataComponentPayload{trimMaterial});
    stack.setComponent(
        item::component::DataComponentType::ChickenVariant, item::component::DataComponentPayload{chickenVariant});

    compound_tag tag;
    stack.toNbt(tag);
    auto result = ItemStack::fromNbt(tag);
    ASSERT_TRUE(result.success()) << "fromNbt 应成功";

    const auto& restored = result.value();
    const auto* restoredTrim = restored.rawComponent(item::component::DataComponentType::ProvidesTrimMaterial);
    ASSERT_NE(restoredTrim, nullptr) << "provides_trim_material 不得在 NBT 往返中丢失";
    EXPECT_EQ(*restoredTrim, trimMaterial);

    const auto* restoredChicken = restored.rawComponent(item::component::DataComponentType::ChickenVariant);
    ASSERT_NE(restoredChicken, nullptr) << "chicken/variant 不得在 NBT 往返中丢失";
    EXPECT_EQ(*restoredChicken, chickenVariant);
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

// 空堆写成空 compound（`{}`），对应原版 `ItemStack.OPTIONAL_CODEC`。
// 不能写 `id=minecraft:air`——`ItemStack.CODEC` 明确拒绝空气。
TEST_F(ItemStackComponentRoundTripTest, NbtRoundTrip_EmptyStack_WritesEmptyCompound)
{
    ItemStack empty;

    compound_tag tag;
    empty.toNbt(tag);

    EXPECT_TRUE(tag.value.empty()) << "empty stack must serialize to a bare '{}' compound";
    EXPECT_EQ(tag.value.find("id"), tag.value.end()) << "empty stack must not carry an id";
    EXPECT_EQ(tag.value.find("count"), tag.value.end()) << "empty stack must not carry a count";

    auto result = ItemStack::fromNbt(tag);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isEmpty());
}

// 反过来：把 air 当物品 id 写进去必须被拒绝，否则非法存档会被静默接受。
TEST_F(ItemStackComponentRoundTripTest, NbtDecode_RejectsAirItemId)
{
    compound_tag tag;
    tag.put("id", std::string("minecraft:air"));
    tag.put("count", static_cast<i32>(1));

    auto result = ItemStack::fromNbt(tag);
    EXPECT_FALSE(result.success()) << "vanilla ItemStack.CODEC rejects minecraft:air";
}

// 未知物品同样必须失败，不能被吞成空堆。
TEST_F(ItemStackComponentRoundTripTest, NbtDecode_RejectsUnknownItemId)
{
    compound_tag tag;
    tag.put("id", std::string("minecraft:definitely_not_an_item"));
    tag.put("count", static_cast<i32>(1));

    auto result = ItemStack::fromNbt(tag);
    EXPECT_FALSE(result.success());
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

// equippable 组件：slot 必填，其余字段可省略。JSON 与 NBT 两条路径都要能往返。
TEST_F(ItemStackComponentRoundTripTest, EquippableRoundTripsThroughJson)
{
    ItemStack stack(*Items::DIAMOND_CHESTPLATE, 1);
    item::component::EquippablePayload equippable{};
    equippable.slot = EquipmentSlot::Chest;
    equippable.equipSound = "minecraft:item.armor.equip_diamond";
    equippable.dispensable = false;
    equippable.swappable = false;
    equippable.damageOnHurt = false;
    equippable.equipOnInteract = true;
    equippable.canBeSheared = true;
    equippable.shearingSound = "minecraft:item.shears.snip";
    stack.setEquippable(equippable);

    const auto json = stack.toJson();
    ASSERT_TRUE(json.contains("minecraft:equippable"));
    EXPECT_EQ(json["minecraft:equippable"]["slot"], "chest");
    EXPECT_EQ(json["minecraft:equippable"]["damage_on_hurt"], false);

    const auto parsed = ItemStack::fromJson(json);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();
    const auto& restored = parsed.value().getEquippable();
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->slot, EquipmentSlot::Chest);
    EXPECT_EQ(restored->equipSound, "minecraft:item.armor.equip_diamond");
    EXPECT_FALSE(restored->dispensable);
    EXPECT_FALSE(restored->swappable);
    EXPECT_FALSE(restored->damageOnHurt);
    EXPECT_TRUE(restored->equipOnInteract);
    EXPECT_TRUE(restored->canBeSheared);
    EXPECT_EQ(restored->shearingSound, "minecraft:item.shears.snip");
}

// 默认值必须省略，与 vanilla 的 optionalFieldOf 一致；否则存档里会堆满冗余键。
TEST_F(ItemStackComponentRoundTripTest, EquippableOmitsDefaultFields)
{
    ItemStack stack(*Items::DIAMOND_CHESTPLATE, 1);
    item::component::EquippablePayload equippable{};
    equippable.slot = EquipmentSlot::Chest;
    stack.setEquippable(equippable);

    const auto json = stack.toJson();
    const auto& component = json["minecraft:equippable"];
    EXPECT_EQ(component.size(), 1u) << "only slot should be written for an all-default component";
    EXPECT_EQ(component["slot"], "chest");
}

// slot 缺失或名称非法时整条组件丢弃：装备槽位是必填语义，猜一个默认槽会让物品静默戴错位置。
TEST_F(ItemStackComponentRoundTripTest, EquippableRejectsMissingOrInvalidSlot)
{
    // 用没有默认 equippable 的物品（剑）作载体：护甲自带默认组件，断言会被默认值掩盖。
    const auto noSlot = ItemStack::fromJson(nlohmann::json{
        {"id", "minecraft:diamond_sword"}, {"Count", 1}, {"minecraft:equippable", nlohmann::json::object()}});
    ASSERT_TRUE(noSlot.success());
    EXPECT_FALSE(noSlot.value().getEquippable().has_value()) << "missing slot must drop the whole component";

    const auto badSlot = ItemStack::fromJson(
        nlohmann::json{{"id", "minecraft:diamond_sword"}, {"Count", 1}, {"minecraft:equippable", {{"slot", "tail"}}}});
    ASSERT_TRUE(badSlot.success());
    EXPECT_FALSE(badSlot.value().getEquippable().has_value()) << "unknown slot name must drop the whole component";
}

// NBT 路径同样要能编解码，否则存档往返会丢失装备语义。
TEST_F(ItemStackComponentRoundTripTest, EquippableRoundTripsThroughNbt)
{
    ItemStack stack(*Items::DIAMOND_CHESTPLATE, 1);
    item::component::EquippablePayload equippable{};
    equippable.slot = EquipmentSlot::Legs;
    equippable.damageOnHurt = false;
    equippable.canBeSheared = true;
    stack.setEquippable(equippable);

    const auto encoded = item::component::detail::payloadToNbt(
        item::component::DataComponentType::Equippable, item::component::DataComponentPayload{equippable});
    ASSERT_NE(encoded, nullptr);
    ASSERT_EQ(encoded->id(), TagId::Compound);

    const auto decoded =
        item::component::detail::nbtToPayload(item::component::DataComponentType::Equippable, *encoded);
    ASSERT_TRUE(std::holds_alternative<item::component::EquippablePayload>(decoded));
    const auto& restored = std::get<item::component::EquippablePayload>(decoded);
    EXPECT_EQ(restored.slot, EquipmentSlot::Legs);
    EXPECT_FALSE(restored.damageOnHurt);
    EXPECT_TRUE(restored.canBeSheared);
}

// 护甲/鞘翅等物品必须在构造时挂默认 equippable 组件，否则 LivingEntity::doHurtEquipment
// 会因为"无组件"而永不磨损——表现为护甲打不坏，与 vanilla 直接冲突。
TEST(DefaultEquippableComponentTest, ArmorItemsCarrySlotSpecificEquippable)
{
    struct Case {
        const char* label;
        const Item* item;
        EquipmentSlot expected;
    };

    const std::array<Case, 5> cases{{
        {"helmet", Items::DIAMOND_HELMET, EquipmentSlot::Head},
        {"chestplate", Items::DIAMOND_CHESTPLATE, EquipmentSlot::Chest},
        {"leggings", Items::DIAMOND_LEGGINGS, EquipmentSlot::Legs},
        {"boots", Items::DIAMOND_BOOTS, EquipmentSlot::Feet},
        {"elytra", Items::ELYTRA, EquipmentSlot::Chest},
    }};

    for (const auto& testCase : cases) {
        ASSERT_NE(testCase.item, nullptr) << testCase.label;
        ItemStack stack(*testCase.item, 1);
        const auto& equippable = stack.getEquippable();
        ASSERT_TRUE(equippable.has_value()) << testCase.label << " must carry a default equippable component";
        EXPECT_EQ(equippable->slot, testCase.expected) << testCase.label;
        // vanilla 默认 damage_on_hurt 为 true：护甲受伤会掉耐久。
        EXPECT_TRUE(equippable->damageOnHurt) << testCase.label;
    }
}

// 非装备物品没有该组件，因此不会被误磨损。
TEST(DefaultEquippableComponentTest, NonEquipmentItemsHaveNoEquippable)
{
    ItemStack stack(*Items::DIAMOND_SWORD, 1);
    EXPECT_FALSE(stack.getEquippable().has_value());
}
