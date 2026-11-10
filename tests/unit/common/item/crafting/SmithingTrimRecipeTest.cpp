/*
 * Copyright (c) 2026 Cubium Contributors
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

#include "common/item/crafting/SmithingTrimRecipe.hpp"
#include "common/TempDirHelper.hpp"
#include "common/item/armortrim/TrimMaterials.hpp"
#include "common/item/armortrim/TrimPatterns.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "item/Items.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace mc;

namespace {

/**
 * @brief 从物品表取一个必定存在的物品
 * @param path 物品资源路径
 * @return 物品指针
 */
const Item* _requireItem(const char* path)
{
    const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", path));
    EXPECT_NE(item, nullptr) << "missing item: minecraft:" << path;
    return item;
}

/**
 * @brief 给物品堆写入 `provides_trim_material` 组件（EitherHolder 文档态）
 * @param stack 目标物品堆
 * @param materialId 材质 id
 */
void _setTrimMaterial(ItemStack& stack, const char* materialId)
{
    stack.setComponent(item::component::DataComponentType::ProvidesTrimMaterial,
        item::component::DataComponentPayload{nlohmann::json{{"id", materialId}}});
}

/**
 * @brief 往目录写一个文本文件，父目录自动创建
 * @param path 目标路径
 * @param content 文件内容
 */
void _writeFile(const std::filesystem::path& path, const char* content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

/**
 * @brief 构造只含纹饰注册表的最小数据包
 *
 * 纹饰图案与材质都是纯数据包注册表，测试不能依赖外部数据包目录，因此自备一份最小集合。
 * 图案选 sentry，材质选 gold 与 copper（后者用于验证"同图案不同材质不算重复"）。
 *
 * @param root 数据包根目录
 */
void _createTrimPack(const std::filesystem::path& root)
{
    const auto packDir = root / "trim_pack";
    _writeFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":71,"description":"trim registries"}})");

    _writeFile(packDir / "data" / "minecraft" / "trim_pattern" / "sentry.json",
        R"({"asset_id":"minecraft:sentry","description":{"translate":"trim_pattern.minecraft.sentry"},"decal":false})");

    for (const char* material : {"gold", "copper"}) {
        const std::string json = std::string(R"({"asset_name":")") + material +
            R"(","description":{"translate":"trim_material.minecraft.)" + std::string(material) + R"("}})";
        _writeFile(packDir / "data" / "minecraft" / "trim_material" / (std::string(material) + ".json"), json.c_str());
    }
}

} // namespace

/**
 * @brief smithing_trim 配方装配行为
 *
 * 纹饰由两部分组成：材质来自 addition 物品的 `provides_trim_material` 组件，图案来自配方
 * 自身的 `pattern` 字段。装配结果是 base 的副本并带上 TRIM 组件。
 */
class SmithingTrimRecipeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::ensureInitialized();
        m_chestplate = _requireItem("diamond_chestplate");
        m_goldIngot = _requireItem("gold_ingot");
        m_template = _requireItem("bolt_armor_trim_smithing_template");

        // 纹饰注册表是纯数据包注册表，单元测试环境不会跑 RegistryBootstrap，
        // 因此这里自备最小数据包并显式加载，避免依赖外部数据包目录。
        m_packRoot = mc::test::makeUniqueTestDir("mc_trim_recipe_test_");
        _createTrimPack(m_packRoot);
        (void)m_repository.addPack(m_packRoot / "trim_pack", true, 0);

        mc::TrimMaterials::clearAll();
        ASSERT_TRUE(mc::TrimMaterials::loadFromDataPackRepository(m_repository).success());
        mc::TrimPatterns::clearAll();
        ASSERT_TRUE(mc::TrimPatterns::loadFromDataPackRepository(m_repository).success());
    }

    void TearDown() override
    {
        // 注册表是进程级单例：用例退出时清空，避免污染其它套件。
        mc::TrimMaterials::clearAll();
        mc::TrimPatterns::clearAll();
        std::error_code ec;
        std::filesystem::remove_all(m_packRoot, ec);
    }

    /**
     * @brief 构造一条 template/base/addition 三槽的纹饰配方
     * @param pattern 图案 id
     * @return 配方实例
     */
    std::unique_ptr<crafting::SmithingTrimRecipe> _makeRecipe(const char* pattern) const
    {
        return std::make_unique<crafting::SmithingTrimRecipe>(ResourceLocation("test", "trim_recipe"),
            crafting::Ingredient::fromItem(*m_template),
            crafting::Ingredient::fromItem(*m_chestplate),
            crafting::Ingredient::fromItem(*m_goldIngot),
            ResourceLocation(pattern));
    }

    const Item* m_chestplate = nullptr;
    const Item* m_goldIngot = nullptr;
    const Item* m_template = nullptr;
    resource::DataPackRepository m_repository;
    std::filesystem::path m_packRoot;
};

TEST_F(SmithingTrimRecipeTest, Assemble_WritesTrimComponent)
{
    // 材料声明自己是 gold 纹饰，图案为哨兵纹（sentry）
    ItemStack addition(*m_goldIngot, 1);
    _setTrimMaterial(addition, "minecraft:gold");

    ItemStack base(*m_chestplate, 1);
    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, base);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    auto recipe = _makeRecipe("minecraft:sentry");
    const ItemStack result = recipe->assemble(inventory);

    ASSERT_FALSE(result.isEmpty()) << "material 与 pattern 都可用时必须产出纹饰结果";
    EXPECT_EQ(result.getItem(), m_chestplate);
    EXPECT_EQ(result.getCount(), 1);

    // TRIM 载荷形态与原版 ArmorTrim record 一致
    const nlohmann::json* trim = result.rawComponent(item::component::DataComponentType::Trim);
    ASSERT_NE(trim, nullptr) << "assemble 必须写入 TRIM 组件";
    EXPECT_EQ((*trim)["material"].get<std::string>(), "minecraft:gold");
    EXPECT_EQ((*trim)["pattern"].get<std::string>(), "minecraft:sentry");
}

TEST_F(SmithingTrimRecipeTest, Assemble_DoesNotMutateBase)
{
    // 装配返回副本，原底材不得被写入 TRIM
    ItemStack addition(*m_goldIngot, 1);
    _setTrimMaterial(addition, "minecraft:gold");

    ItemStack base(*m_chestplate, 1);
    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, base);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    auto recipe = _makeRecipe("minecraft:sentry");
    const ItemStack result = recipe->assemble(inventory);

    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(base.rawComponent(item::component::DataComponentType::Trim), nullptr)
        << "底材本身不应被写入 TRIM，只有产物带纹饰";
}

TEST_F(SmithingTrimRecipeTest, Assemble_MaterialWithoutComponent_ReturnsEmpty)
{
    // addition 没有 provides_trim_material：无法确定材质，不产出
    ItemStack addition(*m_goldIngot, 1);
    ItemStack base(*m_chestplate, 1);

    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, base);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    auto recipe = _makeRecipe("minecraft:sentry");
    EXPECT_TRUE(recipe->assemble(inventory).isEmpty());
}

TEST_F(SmithingTrimRecipeTest, Assemble_UnknownPattern_ReturnsEmpty)
{
    // 配方引用了注册表里不存在的图案：数据包错误，不静默产出无图案物品
    ItemStack addition(*m_goldIngot, 1);
    _setTrimMaterial(addition, "minecraft:gold");
    ItemStack base(*m_chestplate, 1);

    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, base);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    auto recipe = _makeRecipe("minecraft:no_such_trim_pattern");
    EXPECT_TRUE(recipe->assemble(inventory).isEmpty());
}

TEST_F(SmithingTrimRecipeTest, Assemble_SameTrimTwice_ReturnsEmpty)
{
    // 底材已带相同纹饰时再次应用：原版返回空（去重），避免重复消耗材料
    ItemStack addition(*m_goldIngot, 1);
    _setTrimMaterial(addition, "minecraft:gold");

    ItemStack base(*m_chestplate, 1);
    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, base);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    auto recipe = _makeRecipe("minecraft:sentry");
    const ItemStack first = recipe->assemble(inventory);
    ASSERT_FALSE(first.isEmpty());

    // 用第一次的结果作为新的底材，再应用同一纹饰
    blockentity::SimpleInventory second(3);
    second.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    second.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, first);
    second.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    EXPECT_TRUE(recipe->assemble(second).isEmpty()) << "相同纹饰重复应用应返回空";
}

TEST_F(SmithingTrimRecipeTest, Assemble_DifferentMaterial_ProducesNewTrim)
{
    // 相同图案、不同材质属于不同纹饰，必须能覆盖写入
    ItemStack goldAddition(*m_goldIngot, 1);
    _setTrimMaterial(goldAddition, "minecraft:gold");

    ItemStack base(*m_chestplate, 1);
    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, base);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, goldAddition);

    auto recipe = _makeRecipe("minecraft:sentry");
    const ItemStack golden = recipe->assemble(inventory);
    ASSERT_FALSE(golden.isEmpty());

    // 换成 copper 材质，图案不变
    ItemStack copperAddition(*m_goldIngot, 1);
    _setTrimMaterial(copperAddition, "minecraft:copper");
    blockentity::SimpleInventory second(3);
    second.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    second.setItem(crafting::SmithingTrimRecipe::SLOT_BASE, golden);
    second.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, copperAddition);

    const ItemStack copper = recipe->assemble(second);
    ASSERT_FALSE(copper.isEmpty()) << "不同材质不算相同纹饰";
    const nlohmann::json* trim = copper.rawComponent(item::component::DataComponentType::Trim);
    ASSERT_NE(trim, nullptr);
    EXPECT_EQ((*trim)["material"].get<std::string>(), "minecraft:copper");
    EXPECT_EQ((*trim)["pattern"].get<std::string>(), "minecraft:sentry");
}

TEST_F(SmithingTrimRecipeTest, Assemble_EmptyBase_ReturnsEmpty)
{
    ItemStack addition(*m_goldIngot, 1);
    _setTrimMaterial(addition, "minecraft:gold");

    blockentity::SimpleInventory inventory(3);
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_TEMPLATE, ItemStack(*m_template, 1));
    inventory.setItem(crafting::SmithingTrimRecipe::SLOT_ADDITION, addition);

    auto recipe = _makeRecipe("minecraft:sentry");
    EXPECT_TRUE(recipe->assemble(inventory).isEmpty());
}
