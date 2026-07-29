/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

// EnchantmentNbtBuilder 专项测试。
//
// buildEnchantmentRegistryEntriesUncached 从 datapack 读 enchantment JSON，把 HolderSet #tag
// 引用展平为显式元素名列表，序列化为 Java 根 NBT 线格式（0x0A + body + End，无 root name）。
// 本测试构造最小 datapack（1 个 enchantment + 对应 ITEM 标签 + ENCHANTMENT exclusive_set 标签），
// 经真实 DataPackRepository（FolderResourcePack）+ ItemTagLoader（InMemoryResourcePack）加载，
// 断言：
//   1. 条目 data != nullopt；
//   2. 字节前缀为根 NBT 头 0x0A（无 root name——Java NbtIo.writeAnyTag 只写类型字节+body+End）；
//   3. 解析回 compound 后：顶层整数字段为 int_tag（非 byte_tag——jsonToNbt 会把 5 推断为 byte，
//      Java Enchantment.intRange 要求 int，故构建器须显式 put(name, i32) 走 int_tag 推断）；
//   4. supported_items 展平为非空字符串列表（走 ItemTags::getTag 路径）；
//   5. exclusive_set 展平为非空字符串列表（走直读 tags/enchantment/*.json 路径）。
//
// 路径约定：readTextResource(ServerData, "minecraft/enchantment/sharpness.json") 经
// FolderResourcePack 预置 "data/" 前缀落盘 root/data/minecraft/enchantment/sharpness.json，
// 故 datapack 目录内文件须放 data/ 下，而传给 readTextResource 的 path 不含 "data/" 前缀。

#include "server/network/EnchantmentNbtBuilder.hpp"

#include "common/TempDirHelper.hpp"
#include "common/entity/tag/EntityTypeTagLoader.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace mc;
using namespace mc::nbt::tags;
using namespace mc::server::net;

namespace {

// 把文本写入临时目录下的相对路径（自动创建父目录）。
void writeTextFile(const std::filesystem::path& root, const std::string& relPath, const std::string& text)
{
    const auto full = root / relPath;
    std::filesystem::create_directories(full.parent_path());
    std::ofstream f(full, std::ios::binary);
    f << text;
}

// 把 serializeRootCompoundToBytes 产出的根 NBT 字节解析回 compound_tag。
// 根 NBT 线格式 = 0x0A(类型字节) + body(entries + End)，**无 root name 前缀**
// （Java NbtIo.writeAnyTag/readAnyTag 不写/不读 name）。compound_tag::read 直接消费 body
// （[tagid][name][value]...[End]），故先跳过 1 字节类型字节。
std::unique_ptr<compound_tag> parseRootNbtBytes(const std::vector<u8>& bytes)
{
    if (bytes.size() < 2u) {
        return nullptr;
    }
    std::string s(reinterpret_cast<const char*>(bytes.data()) + 1, bytes.size() - 1);
    std::istringstream in(s);
    in >> mc::nbt::Contexts::java;
    return compound_tag::read(in);
}

class EnchantmentNbtBuilderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序须与 ItemTagLoaderTest 一致：方块 -> 物品 -> 方块物品 -> 物品标签。
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
        // effects 谓词 type 字段走 EntityTypeTags::getTag 展平（见 BaneOfArthropods... 测试）。
        mc::EntityTypeTags::initialize();
    }

    // 构造测试 datapack：tempDir/data/{pack.mcmeta, minecraft/enchantment/sharpness.json,
    //   minecraft/tags/item/enchantable/{sharp_weapon,melee_weapon}.json,
    //   minecraft/tags/enchantment/exclusive_set/damage.json}。
    // 返回 tempDir；调用方负责清理（removeTestDir）。
    static std::filesystem::path buildTestDatapack()
    {
        const auto dir = mc::test::makeUniqueTestDir("mc_ench_nbt_builder_");

        writeTextFile(dir, "pack.mcmeta", R"({"pack":{"pack_format":48,"description":"test"}})");

        // enchantment JSON（对齐 vanilla sharpness.json 字段集，但 supported_items/
        // primary_items 指向本测试自造的、无嵌套 # 的 ITEM 标签，便于确定性断言）。
        writeTextFile(dir, "data/minecraft/enchantment/sharpness.json", R"({
            "anvil_cost": 1,
            "description": {"translate": "enchantment.minecraft.sharpness"},
            "effects": {
                "minecraft:damage": [
                    {"effect": {"type": "minecraft:add", "value": {"type": "minecraft:linear", "base": 1.0, "per_level_above_first": 0.5}}}
                ]
            },
            "exclusive_set": "#minecraft:exclusive_set/damage",
            "max_cost": {"base": 21, "per_level_above_first": 11},
            "max_level": 5,
            "min_cost": {"base": 1, "per_level_above_first": 11},
            "primary_items": "#minecraft:enchantable/melee_weapon",
            "slots": ["mainhand"],
            "supported_items": "#minecraft:enchantable/sharp_weapon",
            "weight": 10
        })");

        // ITEM 标签（自包含，无嵌套 #）：用真实物品便于 ItemTags::getTag 路径返回名字。
        writeTextFile(dir,
            "data/minecraft/tags/item/enchantable/sharp_weapon.json",
            R"({"values": ["minecraft:diamond_sword", "minecraft:netherite_sword"]})");
        writeTextFile(dir,
            "data/minecraft/tags/item/enchantable/melee_weapon.json",
            R"({"values": ["minecraft:diamond_sword"]})");

        // ENCHANTMENT exclusive_set 标签（服务端未注册 ENCHANTMENT 标签，走直读 JSON 路径）。
        writeTextFile(dir,
            "data/minecraft/tags/enchantment/exclusive_set/damage.json",
            R"({"values": ["minecraft:sharpness", "minecraft:smite", "minecraft:bane_of_arthropods"]})");

        // bane_of_arthropods：effects.requirements.predicate.type 引用 ENTITY_TYPE 标签 #arrows，
        // 验证 effects 树内 type 字段的 #tag 展平（走 EntityTypeTags::getTag 路径）。
        writeTextFile(dir, "data/minecraft/enchantment/bane_of_arthropods.json", R"({
            "anvil_cost": 2,
            "description": {"translate": "enchantment.minecraft.bane_of_arthropods"},
            "effects": {
                "minecraft:damage": [
                    {"effect": {"type": "minecraft:add", "value": {"type": "minecraft:linear", "base": 2.5, "per_level_above_first": 2.5}},
                     "requirements": {"condition": "minecraft:entity_properties", "entity": "this", "predicate": {"type": "#minecraft:arrows"}}}
                ]
            },
            "exclusive_set": "#minecraft:exclusive_set/damage",
            "max_cost": {"base": 25, "per_level_above_first": 8},
            "max_level": 5,
            "min_cost": {"base": 5, "per_level_above_first": 8},
            "primary_items": "#minecraft:enchantable/melee_weapon",
            "slots": ["mainhand"],
            "supported_items": "#minecraft:enchantable/sharp_weapon",
            "weight": 5
        })");

        // wind_burst：effect.immune_blocks 引用 BLOCK 标签 #blocks_wind_charge_explosions（顶层效果
        // HolderSet 字段，非谓词），验证直读 BLOCK 标签 JSON 展平路径。supported_items 复用已加载的
        // ITEM 标签 sharp_weapon（避免空 supported_items 噪声 warn）。
        writeTextFile(dir, "data/minecraft/enchantment/wind_burst.json", R"({
            "anvil_cost": 4,
            "description": {"translate": "enchantment.minecraft.wind_burst"},
            "effects": {
                "minecraft:post_attack": [
                    {"affected": "attacker",
                     "effect": {"type": "minecraft:explode", "immune_blocks": "#minecraft:blocks_wind_charge_explosions", "radius": 3.5},
                     "enchanted": "attacker",
                     "requirements": {"condition": "minecraft:entity_properties", "entity": "direct_attacker", "predicate": {"flags": {"is_flying": false}, "movement": {"fall_distance": {"min": 1.5}}}}}
                ]
            },
            "max_cost": {"base": 65, "per_level_above_first": 9},
            "max_level": 3,
            "min_cost": {"base": 15, "per_level_above_first": 9},
            "slots": ["mainhand"],
            "supported_items": "#minecraft:enchantable/sharp_weapon",
            "weight": 2
        })");

        // BLOCK 标签（wind_burst.immune_blocks 用；纯名字无嵌套 #，走直读 JSON 路径）。
        writeTextFile(dir,
            "data/minecraft/tags/block/blocks_wind_charge_explosions.json",
            R"({"values": ["minecraft:barrier", "minecraft:bedrock"]})");

        return dir;
    }

    // 用与临时 datapack 相同的 ENTITY_TYPE 标签内容构造 InMemoryResourcePack，加载进 EntityTypeTags
    // （bane_of_arthropods 的 effects predicate.type:#arrows 走 EntityTypeTags::getTag 解析）。
    static void loadEntityTypeTagsIntoRegistry()
    {
        auto pack = std::make_unique<mc::InMemoryResourcePack>("ench_nbt_et_pack");
        pack->addServerDataResource(
            "minecraft/tags/entity_type/arrows.json", R"({"values": ["minecraft:arrow", "minecraft:spectral_arrow"]})");
        auto r = mc::EntityTypeTagLoader::loadFromResourcePack(*pack);
        ASSERT_TRUE(r.success()) << "EntityTypeTagLoader::loadFromResourcePack failed";
    }

    // 用与临时 datapack 相同的 ITEM 标签内容构造 InMemoryResourcePack，并加载进 ItemTags
    // （buildEnchantmentRegistryEntriesUncached 的 supported_items 走 ItemTags::getTag 解析）。
    //
    // 同时注入一个【与 ENTITY_TYPE 标签同名】的 ITEM 标签 #minecraft:arrows（含 tipped_arrow，对齐
    // vanilla item/arrows.json）。这是 effects #tag 跨注册表撞名的回归桩：bane_of_arthropods 的
    // predicate.type:#arrows 须按消费字段 key=type 走 ENTITY_TYPE（arrow/spectral_arrow），**不能**
    // 误选 ITEM 列表（含 tipped_arrow）——后者会让客户端按 ENTITY_TYPE 名解码 tipped_arrow 失败
    // （disconnect-2026-07-29_15.46.43-client.txt 的 power/punch "Failed to parse value" 根因）。
    static void loadItemTagsIntoRegistry()
    {
        auto pack = std::make_unique<mc::InMemoryResourcePack>("ench_nbt_test_pack");
        pack->addServerDataResource("minecraft/tags/item/enchantable/sharp_weapon.json",
            R"({"values": ["minecraft:diamond_sword", "minecraft:netherite_sword"]})");
        pack->addServerDataResource(
            "minecraft/tags/item/enchantable/melee_weapon.json", R"({"values": ["minecraft:diamond_sword"]})");
        // 跨注册表撞名桩：ITEM #arrows 含 tipped_arrow（vanilla item/arrows.json 实态）。
        pack->addServerDataResource("minecraft/tags/item/arrows.json",
            R"({"values": ["minecraft:arrow", "minecraft:tipped_arrow", "minecraft:spectral_arrow"]})");
        auto r = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
        ASSERT_TRUE(r.success()) << "ItemTagLoader::loadFromResourcePack failed";
    }
};

// 在 entries 中按 id 查找条目；找不到返回 nullptr。
const mc::network::ir::configuration::RegistryEntry* findEntry(
    const std::vector<mc::network::ir::configuration::RegistryEntry>& entries, const std::string& id)
{
    for (const auto& e : entries) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

} // namespace

// buildEnchantmentRegistryEntriesUncached 对所有 43 个 enchantment id 都尝试构建；
// 临时 datapack 仅含 sharpness.json，其余 42 个 readTextResource 失败 → data=nullopt。
// 本测试聚焦 sharpness 条目（其 JSON 字段集覆盖全部构建路径）。
TEST_F(EnchantmentNbtBuilderTest, SharpnessEntryHasInlineNbtAndFlattenedHolderSets)
{
    loadItemTagsIntoRegistry();

    const auto dir = buildTestDatapack();
    mc::resource::DataPackRepository repo;
    auto addResult = repo.addPack(dir, /*enabled=*/true, /*priority=*/0);
    ASSERT_TRUE(addResult.success() && addResult.value().initialized) << "addPack failed for temp datapack";

    const auto entries = buildEnchantmentRegistryEntriesUncached(repo);

    // 清理临时目录（断言已取完所需数据）。
    mc::test::removeTestDir(dir);

    ASSERT_EQ(entries.size(), 43u) << "应构造全部 43 个 enchantment 条目";

    const auto* sharpness = findEntry(entries, "minecraft:sharpness");
    ASSERT_NE(sharpness, nullptr);
    ASSERT_TRUE(sharpness->data.has_value()) << "sharpness 应有内联 NBT（data != nullopt）";
    ASSERT_GE(sharpness->data->size(), 2u);

    const auto& bytes = *sharpness->data;
    // 根 NBT 头：仅 0x0A(Compound 类型字节)，无 root name 前缀（Java NbtIo.writeAnyTag 不写 name）。
    // 第二字节起即首个 entry 的 [tagid][name]...，故不再断言 bytes[1]/bytes[2] 为 0。
    EXPECT_EQ(bytes[0], 0x0A) << "缺 compound 类型字节 0x0A";
    EXPECT_NE(bytes[1], 0x00) << "第二字节应是首个 entry 的 tag id（非 0）；若为 0 说明误加 root name 致空 compound";
    EXPECT_EQ(bytes.back(), 0x00) << "应以 End 0x00 结尾";

    auto root = parseRootNbtBytes(bytes);
    ASSERT_NE(root, nullptr) << "根 NBT 字节应可解析回 compound_tag";

    // 顶层整数字段须为 int_tag（id()==TagId::Int，非 Byte）。这是构建器显式 put(name, i32)
    // 的核心契约：jsonToNbt 会把 5 推断为 byte_tag，Java Enchantment.intRange 拒绝 byte_tag。
    ASSERT_TRUE(root->value.count("anvil_cost")) << "缺 anvil_cost";
    EXPECT_EQ(root->value.at("anvil_cost")->id(), mc::nbt::TagId::Int) << "anvil_cost 应为 int_tag（非 byte_tag）";
    EXPECT_EQ(root->get<int_tag>("anvil_cost"), 1);

    ASSERT_TRUE(root->value.count("max_level"));
    EXPECT_EQ(root->value.at("max_level")->id(), mc::nbt::TagId::Int);
    EXPECT_EQ(root->get<int_tag>("max_level"), 5);

    ASSERT_TRUE(root->value.count("weight"));
    EXPECT_EQ(root->value.at("weight")->id(), mc::nbt::TagId::Int);
    EXPECT_EQ(root->get<int_tag>("weight"), 10);

    // Cost 子 compound 的 base/per_level_above_first 亦须为 int_tag。
    ASSERT_TRUE(root->value.count("min_cost"));
    auto& minCost = dynamic_cast<compound_tag&>(*root->value.at("min_cost"));
    ASSERT_TRUE(minCost.value.count("base"));
    EXPECT_EQ(minCost.value.at("base")->id(), mc::nbt::TagId::Int) << "min_cost.base 应为 int_tag";
    EXPECT_EQ(minCost.get<int_tag>("base"), 1);
    ASSERT_TRUE(minCost.value.count("per_level_above_first"));
    EXPECT_EQ(minCost.value.at("per_level_above_first")->id(), mc::nbt::TagId::Int);
    EXPECT_EQ(minCost.get<int_tag>("per_level_above_first"), 11);

    // supported_items：走 ItemTags::getTag 路径展平为名字列表（非 #tag 引用）。
    ASSERT_TRUE(root->value.count("supported_items"));
    auto& supported = dynamic_cast<list_tag&>(*root->value.at("supported_items"));
    EXPECT_EQ(supported.element_id(), mc::nbt::TagId::String) << "supported_items 应为字符串列表";
    EXPECT_GE(supported.size(), 1u) << "supported_items 应展平为非空列表（ItemTags 路径）";
    // 收集名字，验证为物品全限定名（无 # 前缀），且包含 diamond_sword。
    std::vector<std::string> supportedNames;
    for (size_t i = 0; i < supported.size(); ++i) {
        auto elem = supported[i];
        supportedNames.push_back(dynamic_cast<string_tag&>(*elem).value);
    }
    bool hasDiamondSword = false;
    for (const auto& name : supportedNames) {
        EXPECT_EQ(name.find('#'), std::string::npos) << "展平后不应残留 # tag 引用: " << name;
        if (name == "minecraft:diamond_sword") {
            hasDiamondSword = true;
        }
    }
    EXPECT_TRUE(hasDiamondSword) << "supported_items 应含 minecraft:diamond_sword";

    // primary_items：同样走 ItemTags 路径展平。
    ASSERT_TRUE(root->value.count("primary_items"));
    auto& primary = dynamic_cast<list_tag&>(*root->value.at("primary_items"));
    EXPECT_EQ(primary.element_id(), mc::nbt::TagId::String);
    EXPECT_GE(primary.size(), 1u);

    // exclusive_set：走直读 tags/enchantment/exclusive_set/damage.json 路径（ENCHANTMENT 标签
    // 未在服务端注册，ItemTags::getTag 返回 nullptr 回退直读）。
    ASSERT_TRUE(root->value.count("exclusive_set"));
    auto& exclusive = dynamic_cast<list_tag&>(*root->value.at("exclusive_set"));
    EXPECT_EQ(exclusive.element_id(), mc::nbt::TagId::String);
    EXPECT_EQ(exclusive.size(), 3u) << "exclusive_set/damage.json 含 3 个 enchantment 名字";
    auto e0 = exclusive[0];
    EXPECT_EQ(dynamic_cast<string_tag&>(*e0).value, "minecraft:sharpness");

    // slots：字符串列表。
    ASSERT_TRUE(root->value.count("slots"));
    auto& slots = dynamic_cast<list_tag&>(*root->value.at("slots"));
    EXPECT_EQ(slots.element_id(), mc::nbt::TagId::String);
    EXPECT_EQ(slots.size(), 1u);
    EXPECT_EQ(dynamic_cast<string_tag&>(*slots[0]).value, "mainhand");

    // description：jsonToNbt 透传的 Component compound。
    ASSERT_TRUE(root->value.count("description"));
    EXPECT_EQ(root->value.at("description")->id(), mc::nbt::TagId::Compound);
    auto& desc = dynamic_cast<compound_tag&>(*root->value.at("description"));
    ASSERT_TRUE(desc.value.count("translate"));
    EXPECT_EQ(desc.get<string_tag>("translate"), "enchantment.minecraft.sharpness");

    // effects：jsonToNbt 透传（类型安全，Java FloatCodec/NumberProvider 接受任意数值 tag）。
    ASSERT_TRUE(root->value.count("effects"));
    EXPECT_EQ(root->value.at("effects")->id(), mc::nbt::TagId::Compound);
}

// 缺失 datapack 文件时该条目 data=nullopt（构建失败回退），不影响其余条目。
TEST_F(EnchantmentNbtBuilderTest, MissingEnchantmentFileYieldsNullopt)
{
    loadItemTagsIntoRegistry();

    const auto dir = buildTestDatapack();
    mc::resource::DataPackRepository repo;
    ASSERT_TRUE(repo.addPack(dir, true, 0).success());

    const auto entries = buildEnchantmentRegistryEntriesUncached(repo);
    mc::test::removeTestDir(dir);

    // aqua_affinity 不在临时 datapack 中 → data=nullopt。
    const auto* aqua = findEntry(entries, "minecraft:aqua_affinity");
    ASSERT_NE(aqua, nullptr);
    EXPECT_FALSE(aqua->data.has_value()) << "缺失文件的 enchantment 应 data=nullopt";

    // sharpness 仍在（文件存在）。
    const auto* sharpness = findEntry(entries, "minecraft:sharpness");
    ASSERT_NE(sharpness, nullptr);
    EXPECT_TRUE(sharpness->data.has_value());
}

// effects 树内 predicate.type 的 ENTITY_TYPE #tag 展平（走 EntityTypeTags::getTag 路径）。
// bane_of_arthropods 的 requirements.predicate.type:"#minecraft:arrows" 须展平为名字列表
// ["minecraft:arrow","minecraft:spectral_arrow"]，无残留 #。验证 effects 递归展平命中谓词 type 字段。
TEST_F(EnchantmentNbtBuilderTest, BaneOfArthropodsPredicateTypeFlattened)
{
    loadItemTagsIntoRegistry();
    loadEntityTypeTagsIntoRegistry();

    const auto dir = buildTestDatapack();
    mc::resource::DataPackRepository repo;
    ASSERT_TRUE(repo.addPack(dir, true, 0).success());

    const auto entries = buildEnchantmentRegistryEntriesUncached(repo);
    mc::test::removeTestDir(dir);

    const auto* boa = findEntry(entries, "minecraft:bane_of_arthropods");
    ASSERT_NE(boa, nullptr);
    ASSERT_TRUE(boa->data.has_value()) << "bane_of_arthropods 应有内联 NBT";
    auto root = parseRootNbtBytes(*boa->data);
    ASSERT_NE(root, nullptr);

    // effects."minecraft:damage"[0].requirements.predicate.type 须为 string list（非 # 串）。
    ASSERT_TRUE(root->value.count("effects"));
    auto& effects = dynamic_cast<compound_tag&>(*root->value.at("effects"));
    ASSERT_TRUE(effects.value.count("minecraft:damage"));
    auto& damageList = dynamic_cast<list_tag&>(*effects.value.at("minecraft:damage"));
    ASSERT_GE(damageList.size(), 1u);
    // list_tag::operator[] 按值返回 unique_ptr 副本，须先具名捕获再取引用（见 WindBurst 测试注释）。
    auto dmgElem0 = damageList[0];
    auto& entry0 = dynamic_cast<compound_tag&>(*dmgElem0);
    ASSERT_TRUE(entry0.value.count("requirements"));
    auto& req = dynamic_cast<compound_tag&>(*entry0.value.at("requirements"));
    ASSERT_TRUE(req.value.count("predicate"));
    auto& pred = dynamic_cast<compound_tag&>(*req.value.at("predicate"));
    ASSERT_TRUE(pred.value.count("type"));
    // type 须已展平为 string list（id()==List，element_id()==String），非残留 # 字符串。
    auto& typeField = *pred.value.at("type");
    EXPECT_EQ(typeField.id(), mc::nbt::TagId::List) << "predicate.type 应展平为列表（非 # 字符串）";
    auto& typeList = dynamic_cast<list_tag&>(typeField);
    EXPECT_EQ(typeList.element_id(), mc::nbt::TagId::String);
    EXPECT_EQ(typeList.size(), 2u) << "arrows ENTITY_TYPE 标签含 arrow + spectral_arrow（非 ITEM 标签的 3 个）";
    // 收集名字，验证含 minecraft:arrow 且无 # 残留；且【不含 tipped_arrow】——后者是同名 ITEM 标签
    // 的成员，若误选 ITEM 列表会混入，致客户端按 ENTITY_TYPE 名解码失败（撞名回归断言）。
    std::vector<std::string> names;
    for (usize i = 0; i < typeList.size(); ++i) {
        names.push_back(dynamic_cast<string_tag&>(*typeList[i]).value);
    }
    bool hasArrow = false;
    for (const auto& n : names) {
        EXPECT_EQ(n.find('#'), std::string::npos) << "展平后不应残留 # tag 引用: " << n;
        EXPECT_NE(n, "minecraft:tipped_arrow") << "tipped_arrow 是 ITEM 标签成员，不应出现在 ENTITY_TYPE 列表";
        if (n == "minecraft:arrow") {
            hasArrow = true;
        }
    }
    EXPECT_TRUE(hasArrow) << "predicate.type 应含 minecraft:arrow";
}

// effects 树内 effect.immune_blocks 的 BLOCK #tag 展平（走直读 BLOCK 标签 JSON 路径）。
// wind_burst 的 effect.immune_blocks:"#minecraft:blocks_wind_charge_explosions" 须展平为名字列表
// ["minecraft:barrier","minecraft:bedrock"]，无残留 #。验证 effects 递归展平命中【顶层效果 HolderSet
// 字段】（immune_blocks 非 type/blocks/items 谓词字段，按 # 开头值盲展平判据覆盖）。
TEST_F(EnchantmentNbtBuilderTest, WindBurstImmuneBlocksFlattened)
{
    loadItemTagsIntoRegistry();

    const auto dir = buildTestDatapack();
    mc::resource::DataPackRepository repo;
    ASSERT_TRUE(repo.addPack(dir, true, 0).success());

    const auto entries = buildEnchantmentRegistryEntriesUncached(repo);
    mc::test::removeTestDir(dir);

    const auto* wb = findEntry(entries, "minecraft:wind_burst");
    ASSERT_NE(wb, nullptr);
    ASSERT_TRUE(wb->data.has_value()) << "wind_burst 应有内联 NBT";
    auto root = parseRootNbtBytes(*wb->data);
    ASSERT_NE(root, nullptr);

    // effects."minecraft:post_attack"[0].effect.immune_blocks 须为 string list（非 # 串）。
    ASSERT_TRUE(root->value.count("effects"));
    auto& effects = dynamic_cast<compound_tag&>(*root->value.at("effects"));
    ASSERT_TRUE(effects.value.count("minecraft:post_attack"));
    auto& paList = dynamic_cast<list_tag&>(*effects.value.at("minecraft:post_attack"));
    ASSERT_GE(paList.size(), 1u);
    // list_tag::operator[] 返回 std::unique_ptr<tag>（按值副本），须先捕获到具名变量再取引用，
    // 否则 dynamic_cast 绑定到临时 unique_ptr 的对象，表达式结束即销毁→悬垂引用 UB。
    auto elem0 = paList[0];
    auto& entry0 = dynamic_cast<compound_tag&>(*elem0);
    ASSERT_TRUE(entry0.value.count("effect"));
    auto& effect = dynamic_cast<compound_tag&>(*entry0.value.at("effect"));
    ASSERT_TRUE(effect.value.count("immune_blocks")) << "effect 应含 immune_blocks 字段";
    // immune_blocks 须已展平为 string list（id()==List，element_id()==String），非残留 # 字符串。
    auto& ibField = *effect.value.at("immune_blocks");
    EXPECT_EQ(ibField.id(), mc::nbt::TagId::List) << "immune_blocks 应展平为列表（非 # 字符串）";
    auto& ibList = dynamic_cast<list_tag&>(ibField);
    EXPECT_EQ(ibList.element_id(), mc::nbt::TagId::String);
    EXPECT_EQ(ibList.size(), 2u) << "blocks_wind_charge_explosions 含 barrier + bedrock";
    std::vector<std::string> names;
    for (usize i = 0; i < ibList.size(); ++i) {
        names.push_back(dynamic_cast<string_tag&>(*ibList[i]).value);
    }
    bool hasBarrier = false;
    for (const auto& n : names) {
        EXPECT_EQ(n.find('#'), std::string::npos) << "展平后不应残留 # tag 引用: " << n;
        if (n == "minecraft:barrier") {
            hasBarrier = true;
        }
    }
    EXPECT_TRUE(hasBarrier) << "immune_blocks 应含 minecraft:barrier";
}
