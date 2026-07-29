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
// 引用展平为显式元素名列表，序列化为 Java 根 NBT 线格式（0x0A + 空 name + body + End）。
// 本测试构造最小 datapack（1 个 enchantment + 对应 ITEM 标签 + ENCHANTMENT exclusive_set 标签），
// 经真实 DataPackRepository（FolderResourcePack）+ ItemTagLoader（InMemoryResourcePack）加载，
// 断言：
//   1. 条目 data != nullopt；
//   2. 字节前缀为根 NBT 头 0x0A 0x00 0x00；
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
// 根 NBT 线格式 = 0x0A(类型) + 0x00 0x00(空 root name) + body(entries + End)。
// compound_tag::read 直接消费 body（[tagid][name][value]...[End]），故先跳过 3 字节根头。
std::unique_ptr<compound_tag> parseRootNbtBytes(const std::vector<u8>& bytes)
{
    if (bytes.size() < 4u) {
        return nullptr;
    }
    std::string s(reinterpret_cast<const char*>(bytes.data()) + 3, bytes.size() - 3);
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

        return dir;
    }

    // 用与临时 datapack 相同的 ITEM 标签内容构造 InMemoryResourcePack，并加载进 ItemTags
    // （buildEnchantmentRegistryEntriesUncached 的 supported_items 走 ItemTags::getTag 解析）。
    static void loadItemTagsIntoRegistry()
    {
        auto pack = std::make_unique<mc::InMemoryResourcePack>("ench_nbt_test_pack");
        pack->addServerDataResource("minecraft/tags/item/enchantable/sharp_weapon.json",
            R"({"values": ["minecraft:diamond_sword", "minecraft:netherite_sword"]})");
        pack->addServerDataResource(
            "minecraft/tags/item/enchantable/melee_weapon.json", R"({"values": ["minecraft:diamond_sword"]})");
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
    ASSERT_GE(sharpness->data->size(), 4u);

    const auto& bytes = *sharpness->data;
    // 根 NBT 头：0x0A(Compound 类型) + 0x00 0x00(空 root name 长度)。
    EXPECT_EQ(bytes[0], 0x0A) << "缺 compound 类型字节 0x0A";
    EXPECT_EQ(bytes[1], 0x00) << "root name 长度高字节应为 0";
    EXPECT_EQ(bytes[2], 0x00) << "root name 长度低字节应为 0（空 name）";
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
