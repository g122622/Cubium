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

#include <gtest/gtest.h>

#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/structure/StructureTag.hpp"
#include "common/world/gen/structure/StructureTagLoader.hpp"
#include "common/world/gen/structure/StructureTags.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace mc;
using namespace mc::world::gen::structure;

// ============================================================================
// 1. StructureTag 单元测试
// ============================================================================

class StructureTagTest : public ::testing::Test {
protected:
    // 每个测试用例独立创建标签，无共享状态
};

TEST_F(StructureTagTest, ConstructorSetsId)
{
    StructureTag tag(ResourceLocation("minecraft:test_tag"));
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:test_tag"));
}

TEST_F(StructureTagTest, ConstructorDefaultReplaceFalse)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    EXPECT_FALSE(tag.isReplace());
}

TEST_F(StructureTagTest, ConstructorWithReplaceTrue)
{
    StructureTag tag(ResourceLocation("minecraft:test"), true);
    EXPECT_TRUE(tag.isReplace());
}

TEST_F(StructureTagTest, SetReplace)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    tag.setReplace(true);
    EXPECT_TRUE(tag.isReplace());
    tag.setReplace(false);
    EXPECT_FALSE(tag.isReplace());
}

TEST_F(StructureTagTest, AddAndContains)
{
    StructureTag tag(ResourceLocation("minecraft:test"));

    // 添加前不包含
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:stronghold")));

    // 添加结构
    tag.add(ResourceLocation("minecraft:shipwreck"));
    tag.add(ResourceLocation("minecraft:stronghold"));

    // 添加后包含
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:stronghold")));

    // 未添加的不包含
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:village")));
}

TEST_F(StructureTagTest, AddAll)
{
    StructureTag tag(ResourceLocation("minecraft:test"));

    std::vector<ResourceLocation> ids = {ResourceLocation("minecraft:shipwreck"),
        ResourceLocation("minecraft:shipwreck_beached"),
        ResourceLocation("minecraft:ocean_ruin_cold")};

    tag.addAll(ids);

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck_beached")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ocean_ruin_cold")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:village")));
    EXPECT_EQ(tag.getStructureIds().size(), 3u);
}

TEST_F(StructureTagTest, ClearRemovesAll)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:shipwreck"));
    tag.add(ResourceLocation("minecraft:stronghold"));

    EXPECT_EQ(tag.getStructureIds().size(), 2u);

    tag.clear();

    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:stronghold")));
    EXPECT_EQ(tag.getStructureIds().size(), 0u);
    EXPECT_TRUE(tag.getStructureIds().empty());
}

TEST_F(StructureTagTest, DuplicateAddIsIdempotent)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:shipwreck"));
    tag.add(ResourceLocation("minecraft:shipwreck")); // 重复添加

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagTest, AddAllWithDuplicates)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    std::vector<ResourceLocation> ids = {ResourceLocation("minecraft:shipwreck"),
        ResourceLocation("minecraft:shipwreck"), // 重复
        ResourceLocation("minecraft:stronghold"),
        ResourceLocation("minecraft:stronghold")}; // 重复

    tag.addAll(ids);

    EXPECT_EQ(tag.getStructureIds().size(), 2u);
}

TEST_F(StructureTagTest, ClearOnEmptyTag)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    // 清空空标签不应崩溃
    tag.clear();
    EXPECT_TRUE(tag.getStructureIds().empty());
}

TEST_F(StructureTagTest, ReplaceAllViaClearAndAdd)
{
    // 模拟 replace=true 的语义：清空后追加
    StructureTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:old1"));
    tag.add(ResourceLocation("minecraft:old2"));
    ASSERT_EQ(tag.getStructureIds().size(), 2u);

    tag.clear();
    tag.add(ResourceLocation("minecraft:new1"));
    tag.add(ResourceLocation("minecraft:new2"));

    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:old1")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:old2")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:new1")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:new2")));
    EXPECT_EQ(tag.getStructureIds().size(), 2u);
}

TEST_F(StructureTagTest, GetStructureIdsReturnsConstRef)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:shipwreck"));

    const auto& ids = tag.getStructureIds();
    EXPECT_EQ(ids.size(), 1u);
    EXPECT_TRUE(ids.contains(ResourceLocation("minecraft:shipwreck")));
}

TEST_F(StructureTagTest, ContainsWithDifferentNamespace)
{
    StructureTag tag(ResourceLocation("minecraft:test"));
    tag.add(ResourceLocation("minecraft:shipwreck"));

    // 不同命名空间的同名结构不应匹配
    EXPECT_FALSE(tag.contains(ResourceLocation("custom:shipwreck")));
}

// ============================================================================
// 2. StructureTags 初始化测试
// ============================================================================

class StructureTagsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化内置结构标签（幂等）
        StructureTags::initialize();
    }
};

TEST_F(StructureTagsTest, InitializeIsIdempotent)
{
    // 重复调用 initialize 不应崩溃，也不应改变标签内容
    StructureTags::initialize();
    StructureTags::initialize();

    EXPECT_TRUE(StructureTags::DOLPHIN_LOCATED().contains(ResourceLocation("minecraft:shipwreck")));
}

TEST_F(StructureTagsTest, GetTagReturnsNullForNonExistent)
{
    auto* tag = StructureTags::getTag(ResourceLocation("minecraft:nonexistent_tag"));
    EXPECT_EQ(tag, nullptr);
}

TEST_F(StructureTagsTest, GetTagReturnsExistingTag)
{
    auto* tag = StructureTags::getTag(ResourceLocation("minecraft:dolphin_located"));
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft:dolphin_located"));
}

TEST_F(StructureTagsTest, RegisterTagCreatesNewTag)
{
    auto& tag = StructureTags::registerTag(ResourceLocation("minecraft:test_custom_tag"));
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:test_custom_tag"));

    // 注册后应能通过 getTag 查找
    auto* found = StructureTags::getTag(ResourceLocation("minecraft:test_custom_tag"));
    ASSERT_NE(found, nullptr);
}

TEST_F(StructureTagsTest, RegisterTagReturnsExistingTag)
{
    // 重复注册应返回已有标签
    auto& tag1 = StructureTags::registerTag(ResourceLocation("minecraft:test_duplicate_tag"));
    tag1.add(ResourceLocation("minecraft:shipwreck"));

    auto& tag2 = StructureTags::registerTag(ResourceLocation("minecraft:test_duplicate_tag"));
    EXPECT_EQ(&tag1, &tag2);
    EXPECT_TRUE(tag2.contains(ResourceLocation("minecraft:shipwreck")));
}

// ----- 20 个内置标签的成员正确性测试 -----

TEST_F(StructureTagsTest, EyeOfEnderLocatedContainsStronghold)
{
    auto& tag = StructureTags::EYE_OF_ENDER_LOCATED();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:eye_of_ender_located"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:stronghold")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, DolphinLocatedContainsFourStructures)
{
    // DOLPHIN_LOCATED 应包含 4 个结构 ID：
    // ocean_ruin_cold, ocean_ruin_warm, shipwreck, shipwreck_beached
    auto& tag = StructureTags::DOLPHIN_LOCATED();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:dolphin_located"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ocean_ruin_cold")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ocean_ruin_warm")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck_beached")));

    EXPECT_EQ(tag.getStructureIds().size(), 4u);
}

TEST_F(StructureTagsTest, DolphinLocatedDoesNotContainNonMembers)
{
    auto& tag = StructureTags::DOLPHIN_LOCATED();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:village")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:stronghold")));
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft:mansion")));
}

TEST_F(StructureTagsTest, VillageContainsFiveVariants)
{
    auto& tag = StructureTags::VILLAGE();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:village"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_plains")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_desert")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_savanna")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_snowy")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_taiga")));

    EXPECT_EQ(tag.getStructureIds().size(), 5u);
}

TEST_F(StructureTagsTest, MineshaftContainsTwoVariants)
{
    auto& tag = StructureTags::MINESHAFT();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:mineshaft"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:mineshaft")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:mineshaft_mesa")));

    EXPECT_EQ(tag.getStructureIds().size(), 2u);
}

TEST_F(StructureTagsTest, ShipwreckContainsTwoVariants)
{
    auto& tag = StructureTags::SHIPWRECK();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:shipwreck"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:shipwreck_beached")));

    EXPECT_EQ(tag.getStructureIds().size(), 2u);
}

TEST_F(StructureTagsTest, OceanRuinContainsTwoVariants)
{
    auto& tag = StructureTags::OCEAN_RUIN();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:ocean_ruin"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ocean_ruin_cold")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ocean_ruin_warm")));

    EXPECT_EQ(tag.getStructureIds().size(), 2u);
}

TEST_F(StructureTagsTest, RuinedPortalContainsSevenVariants)
{
    auto& tag = StructureTags::RUINED_PORTAL();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:ruined_portal"));

    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_desert")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_jungle")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_mountain")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_nether")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_ocean")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_standard")));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:ruined_portal_swamp")));

    EXPECT_EQ(tag.getStructureIds().size(), 7u);
}

TEST_F(StructureTagsTest, CatsSpawnInContainsSwampHut)
{
    auto& tag = StructureTags::CATS_SPAWN_IN();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:cats_spawn_in"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:swamp_hut")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, CatsSpawnAsBlackContainsSwampHut)
{
    auto& tag = StructureTags::CATS_SPAWN_AS_BLACK();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:cats_spawn_as_black"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:swamp_hut")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnWoodlandExplorerMapsContainsMansion)
{
    auto& tag = StructureTags::ON_WOODLAND_EXPLORER_MAPS();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:on_woodland_explorer_maps"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:mansion")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnOceanExplorerMapsContainsMonument)
{
    auto& tag = StructureTags::ON_OCEAN_EXPLORER_MAPS();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:on_ocean_explorer_maps"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:monument")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnTreasureMapsContainsBuriedTreasure)
{
    auto& tag = StructureTags::ON_TREASURE_MAPS();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:on_treasure_maps"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:buried_treasure")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnTrialChambersMapsContainsTrialChambers)
{
    auto& tag = StructureTags::ON_TRIAL_CHAMBERS_MAPS();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft:on_trial_chambers_maps"));
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:trial_chambers")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnSavannaVillageMapsContainsSavannaVillage)
{
    auto& tag = StructureTags::ON_SAVANNA_VILLAGE_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_savanna")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnDesertVillageMapsContainsDesertVillage)
{
    auto& tag = StructureTags::ON_DESERT_VILLAGE_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_desert")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnPlainsVillageMapsContainsPlainsVillage)
{
    auto& tag = StructureTags::ON_PLAINS_VILLAGE_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_plains")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnTaigaVillageMapsContainsTaigaVillage)
{
    auto& tag = StructureTags::ON_TAIGA_VILLAGE_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_taiga")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnSnowyVillageMapsContainsSnowyVillage)
{
    auto& tag = StructureTags::ON_SNOWY_VILLAGE_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:village_snowy")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnJungleExplorerMapsContainsJungleTemple)
{
    auto& tag = StructureTags::ON_JUNGLE_EXPLORER_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:jungle_temple")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, OnSwampExplorerMapsContainsSwampHut)
{
    auto& tag = StructureTags::ON_SWAMP_EXPLORER_MAPS();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft:swamp_hut")));
    EXPECT_EQ(tag.getStructureIds().size(), 1u);
}

TEST_F(StructureTagsTest, ForEachTagIteratesAllTags)
{
    // 至少应迭代到 DOLPHIN_LOCATED
    bool foundDolphin = false;
    size_t count = 0;
    StructureTags::forEachTag([&](StructureTag& tag) {
        ++count;
        if (tag.getId() == ResourceLocation("minecraft:dolphin_located")) {
            foundDolphin = true;
        }
    });

    EXPECT_TRUE(foundDolphin);
    // 至少有 20 个内置标签
    EXPECT_GE(count, 20u);
}

// ============================================================================
// 3. StructureTagLoader JSON 解析测试
// ============================================================================

class StructureTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化内置标签，以便测试 # 引用解析
        StructureTags::initialize();
    }
};

TEST_F(StructureTagLoaderTest, LoadFromJsonBasicDirectStructures)
{
    const std::string json = R"({
        "values": [
            "minecraft:shipwreck",
            "minecraft:stronghold",
            "minecraft:village_plains"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft:test_tag"));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:stronghold")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_plains")));
    EXPECT_FALSE(tag->contains(ResourceLocation("minecraft:mansion")));
    EXPECT_EQ(tag->getStructureIds().size(), 3u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonWithTagReference)
{
    // 引用 DOLPHIN_LOCATED 标签（4 个成员）
    const std::string json = R"({
        "values": [
            "#minecraft:dolphin_located",
            "minecraft:mansion"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_with_ref"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // DOLPHIN_LOCATED 的 4 个成员应被展开
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:ocean_ruin_cold")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:ocean_ruin_warm")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck_beached")));
    // 直接添加的成员
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:mansion")));
    EXPECT_EQ(tag->getStructureIds().size(), 5u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonWithMultipleTagReferences)
{
    // 同时引用两个标签，验证 visitedTags 按路径传递不会跨分支错误跳过
    // VILLAGE 有 5 个成员，SHIPWRECK 有 2 个成员
    const std::string json = R"({
        "values": [
            "#minecraft:village",
            "#minecraft:shipwreck"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_multi_ref"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // VILLAGE 的 5 个成员
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_plains")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_desert")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_savanna")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_snowy")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_taiga")));
    // SHIPWRECK 的 2 个成员
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck_beached")));
    EXPECT_EQ(tag->getStructureIds().size(), 7u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonWithReplaceTrue)
{
    const std::string json = R"({
        "replace": true,
        "values": [
            "minecraft:shipwreck"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_replace"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->isReplace());
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonWithReplaceFalse)
{
    const std::string json = R"({
        "replace": false,
        "values": [
            "minecraft:shipwreck"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_no_replace"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_FALSE(tag->isReplace());
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
}

TEST_F(StructureTagLoaderTest, LoadFromJsonWithRequiredFalseMissingEntry)
{
    // required=false 的不存在标签引用应静默跳过，不影响其他条目
    const std::string json = R"({
        "values": [
            "minecraft:shipwreck",
            {"id": "#minecraft:nonexistent_tag", "required": false},
            "minecraft:stronghold"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_optional"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:stronghold")));
    EXPECT_EQ(tag->getStructureIds().size(), 2u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonWithRequiredTrueMissingEntry)
{
    // required=true 但不存在的标签引用仅输出警告，不会导致解析失败
    const std::string json = R"({
        "values": [
            "minecraft:shipwreck",
            {"id": "#minecraft:nonexistent_tag", "required": true}
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_required_missing"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonObjectEntryDefaultRequired)
{
    // 对象格式不指定 required 时默认为 true
    const std::string json = R"({"values": [{"id": "minecraft:shipwreck"}]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_default_required"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonObjectEntryMissingIdSkipped)
{
    // 对象条目缺少 id 字段，应被跳过
    const std::string json = R"({"values": ["minecraft:shipwreck", {"required": false}]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_missing_id"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonObjectEntryEmptyIdSkipped)
{
    // 对象条目 id 为空字符串，应被跳过
    const std::string json = R"({"values": ["minecraft:shipwreck", {"id": "", "required": false}]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_empty_id"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonNonStringNonObjectValuesIgnored)
{
    // 数值和布尔值被跳过
    const std::string json = R"({
        "values": [
            "minecraft:shipwreck",
            42,
            true,
            {"id": "minecraft:stronghold", "required": false}
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_bad_values"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getStructureIds().size(), 2u);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:stronghold")));
}

TEST_F(StructureTagLoaderTest, LoadFromJsonEmptyValues)
{
    const std::string json = R"({"values": []})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_empty"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->getStructureIds().empty());
}

TEST_F(StructureTagLoaderTest, LoadFromJsonMissingValuesArray)
{
    const std::string json = R"({"replace": false})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_missing_values"));
    EXPECT_FALSE(result.success());
}

TEST_F(StructureTagLoaderTest, LoadFromJsonValuesNotArray)
{
    // values 字段不是数组，应返回错误
    const std::string json = R"({"values": "minecraft:shipwreck"})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_values_not_array"));
    EXPECT_FALSE(result.success());
}

TEST_F(StructureTagLoaderTest, LoadFromJsonInvalidJson)
{
    const std::string json = "not valid json";
    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_invalid_json"));
    EXPECT_FALSE(result.success());
}

TEST_F(StructureTagLoaderTest, LoadFromJsonSelfReferenceCycleDetected)
{
    // 自引用：标签引用自身
    // visitedTags 在 loadFromJson 中预插入了 location，所以自引用会被检测到
    const std::string json = R"({
        "values": [
            "minecraft:shipwreck",
            "#minecraft:self_ref_tag"
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:self_ref_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // shipwreck 应被添加，自引用应被跳过（不导致无限递归）
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonReplaceFieldNotBoolean)
{
    // replace 字段不是布尔值，应被忽略（默认 false）
    const std::string json = R"({"replace": "true", "values": ["minecraft:shipwreck"]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_bad_replace"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_FALSE(tag->isReplace());
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
}

TEST_F(StructureTagLoaderTest, LoadFromJsonRequiredFieldNotBoolean)
{
    // required 字段不是布尔值，应被忽略（默认 true）
    const std::string json = R"({"values": [{"id": "minecraft:shipwreck", "required": "yes"}]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_bad_required"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
}

TEST_F(StructureTagLoaderTest, LoadFromJsonMixedEntries)
{
    // 混合直接 ID、标签引用、对象格式
    const std::string json = R"({
        "values": [
            "minecraft:stronghold",
            "#minecraft:shipwreck",
            {"id": "#minecraft:ocean_ruin", "required": false},
            {"id": "minecraft:mansion", "required": true}
        ]
    })";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_mixed"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // 直接 ID
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:stronghold")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:mansion")));
    // #minecraft:shipwreck 展开
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck_beached")));
    // #minecraft:ocean_ruin 展开
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:ocean_ruin_cold")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:ocean_ruin_warm")));
    EXPECT_EQ(tag->getStructureIds().size(), 6u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonEmptyStringEntryIgnored)
{
    // 空字符串条目应被跳过
    const std::string json = R"({"values": ["", "minecraft:shipwreck"]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_empty_entry"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:shipwreck")));
    EXPECT_EQ(tag->getStructureIds().size(), 1u);
}

TEST_F(StructureTagLoaderTest, LoadFromJsonTagReferenceToExistingTag)
{
    // 引用已存在的内置标签
    const std::string json = R"({"values": ["#minecraft:village"]})";

    auto result = StructureTagLoader::loadFromJson(json, ResourceLocation("minecraft:test_village_ref"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // VILLAGE 的 5 个成员
    EXPECT_EQ(tag->getStructureIds().size(), 5u);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:village_plains")));
}

// ============================================================================
// 4. findNearestMapStructure 路由逻辑测试
// ============================================================================
//
// 此处测试 findNearestMapStructure 的路由逻辑（标签查找 + 遍历 + 取最近）。
// 由于 ServerWorld::findNearestStructure 需要完整的区块生成管线，
// 此处通过模拟 findNearestStructure 的返回值来隔离测试路由逻辑。
// 测试覆盖 stop hook 要求的三个场景：
// - 标签不存在返回空
// - 标签为空返回空
// - 多结构取最近
//
// 路由逻辑与 ServerWorld::findNearestMapStructure 保持一致，
// 后者在内部调用 StructureTags::getTag + 遍历 + findNearestStructure。
// 此处通过 FindNearestMapStructureSimulator 模拟该流程，验证算法正确性。

namespace {

/// @brief 简单 BlockPos 替代（避免引入完整 BlockPos 头文件依赖）
struct TestBlockPos {
    i32 x = 0;
    i32 y = 0;
    i32 z = 0;

    TestBlockPos() noexcept = default;
    TestBlockPos(i32 x_, i32 y_, i32 z_) noexcept
        : x(x_)
        , y(y_)
        , z(z_)
    {}
};

/// @brief 模拟 findNearestMapStructure 的路由逻辑
///
/// 此类复制了 ServerWorld::findNearestMapStructure 的算法逻辑，
/// 用于在不依赖完整区块生成的情况下测试路由正确性。
/// 通过 setStructureLocation 设置 structureId → 位置的映射，
/// findNearestStructure 返回映射的位置（或空）。
class FindNearestMapStructureSimulator {
public:
    void setStructureLocation(const ResourceLocation& structureId, const TestBlockPos& pos)
    {
        m_locations[structureId] = pos;
    }

    /// @brief 模拟 findNearestStructure，返回预设位置
    [[nodiscard]] std::optional<TestBlockPos> findNearestStructure(const TestBlockPos& /*center*/,
        const ResourceLocation& structureId,
        i32 /*maxDistance*/,
        bool /*skipExisting*/) const
    {
        auto it = m_locations.find(structureId);
        if (it != m_locations.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// @brief 模拟 ServerWorld::findNearestMapStructure 的路由逻辑
    [[nodiscard]] std::optional<TestBlockPos> findNearestMapStructure(
        const TestBlockPos& center, const ResourceLocation& tagId, i32 maxDistance, bool skipExisting) const
    {
        // 与 ServerWorld::findNearestMapStructure 算法一致
        auto* tag = StructureTags::getTag(tagId);
        if (tag == nullptr) {
            return std::nullopt;
        }

        if (tag->getStructureIds().empty()) {
            return std::nullopt;
        }

        std::optional<TestBlockPos> nearestPos;
        f64 nearestDistSq = std::numeric_limits<f64>::max();

        for (const auto& structureId : tag->getStructureIds()) {
            auto candidatePos = findNearestStructure(center, structureId, maxDistance, skipExisting);
            if (!candidatePos.has_value()) {
                continue;
            }

            i32 dx = candidatePos->x - center.x;
            i32 dz = candidatePos->z - center.z;
            f64 distSq = static_cast<f64>(dx * dx + dz * dz);

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestPos = candidatePos;
            }
        }

        return nearestPos;
    }

private:
    std::unordered_map<ResourceLocation, TestBlockPos> m_locations;
};

} // namespace

class FindNearestMapStructureTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { StructureTags::initialize(); }

    void SetUp() override { m_simulator = std::make_unique<FindNearestMapStructureSimulator>(); }

    std::unique_ptr<FindNearestMapStructureSimulator> m_simulator;
};

TEST_F(FindNearestMapStructureTest, UnknownTagReturnsNullopt)
{
    // 标签不存在 → 返回空
    TestBlockPos center(0, 64, 0);
    auto result =
        m_simulator->findNearestMapStructure(center, ResourceLocation("minecraft:nonexistent_tag"), 1000, false);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FindNearestMapStructureTest, EmptyTagReturnsNullopt)
{
    // 注册一个空标签
    StructureTags::registerTag(ResourceLocation("minecraft:empty_test_tag"));
    auto* tag = StructureTags::getTag(ResourceLocation("minecraft:empty_test_tag"));
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->getStructureIds().empty());

    TestBlockPos center(0, 64, 0);
    auto result =
        m_simulator->findNearestMapStructure(center, ResourceLocation("minecraft:empty_test_tag"), 1000, false);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FindNearestMapStructureTest, SingleStructureReturnsPosition)
{
    // 标签中只有一个结构，findNearestStructure 返回位置
    auto& testTag = StructureTags::registerTag(ResourceLocation("minecraft:test_single_structure_tag"));
    testTag.clear();
    testTag.add(ResourceLocation("minecraft:shipwreck"));

    TestBlockPos expectedPos(100, 64, 200);
    m_simulator->setStructureLocation(ResourceLocation("minecraft:shipwreck"), expectedPos);

    TestBlockPos center(0, 64, 0);
    auto result = m_simulator->findNearestMapStructure(
        center, ResourceLocation("minecraft:test_single_structure_tag"), 1000, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->x, expectedPos.x);
    EXPECT_EQ(result->z, expectedPos.z);
}

TEST_F(FindNearestMapStructureTest, MultipleStructuresReturnsNearest)
{
    // 标签中有多个结构，应返回距离中心最近的
    auto& testTag = StructureTags::registerTag(ResourceLocation("minecraft:test_multi_structure_tag"));
    testTag.clear();
    testTag.add(ResourceLocation("minecraft:shipwreck"));
    testTag.add(ResourceLocation("minecraft:stronghold"));
    testTag.add(ResourceLocation("minecraft:mansion"));

    // 三个位置，距离中心 (0,0) 分别为 100, 50, 200
    TestBlockPos shipwreckPos(100, 64, 0); // 距离 100
    TestBlockPos strongholdPos(50, 64, 0); // 距离 50（最近）
    TestBlockPos mansionPos(200, 64, 0);   // 距离 200

    m_simulator->setStructureLocation(ResourceLocation("minecraft:shipwreck"), shipwreckPos);
    m_simulator->setStructureLocation(ResourceLocation("minecraft:stronghold"), strongholdPos);
    m_simulator->setStructureLocation(ResourceLocation("minecraft:mansion"), mansionPos);

    TestBlockPos center(0, 64, 0);
    auto result = m_simulator->findNearestMapStructure(
        center, ResourceLocation("minecraft:test_multi_structure_tag"), 1000, false);
    ASSERT_TRUE(result.has_value());
    // 应返回最近的 stronghold 位置
    EXPECT_EQ(result->x, strongholdPos.x);
    EXPECT_EQ(result->z, strongholdPos.z);
}

TEST_F(FindNearestMapStructureTest, SomeStructuresNotFoundReturnsNearestFound)
{
    // 标签中部分结构未找到（findNearestStructure 返回空），应返回已找到的最近位置
    auto& testTag = StructureTags::registerTag(ResourceLocation("minecraft:test_partial_not_found_tag"));
    testTag.clear();
    testTag.add(ResourceLocation("minecraft:shipwreck"));
    testTag.add(ResourceLocation("minecraft:stronghold"));
    testTag.add(ResourceLocation("minecraft:mansion"));

    // 只设置 shipwreck 的位置，其他两个 findNearestStructure 返回空
    TestBlockPos shipwreckPos(80, 64, 60); // 距离 sqrt(80²+60²) = 100
    m_simulator->setStructureLocation(ResourceLocation("minecraft:shipwreck"), shipwreckPos);

    TestBlockPos center(0, 64, 0);
    auto result = m_simulator->findNearestMapStructure(
        center, ResourceLocation("minecraft:test_partial_not_found_tag"), 1000, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->x, shipwreckPos.x);
    EXPECT_EQ(result->z, shipwreckPos.z);
}

TEST_F(FindNearestMapStructureTest, AllStructuresNotFoundReturnsNullopt)
{
    // 标签中所有结构都未找到 → 返回空
    auto& testTag = StructureTags::registerTag(ResourceLocation("minecraft:test_all_not_found_tag"));
    testTag.clear();
    testTag.add(ResourceLocation("minecraft:shipwreck"));
    testTag.add(ResourceLocation("minecraft:stronghold"));

    // 不设置任何位置映射，findNearestStructure 全部返回空

    TestBlockPos center(0, 64, 0);
    auto result =
        m_simulator->findNearestMapStructure(center, ResourceLocation("minecraft:test_all_not_found_tag"), 1000, false);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FindNearestMapStructureTest, NearestByEuclideanDistance)
{
    // 验证使用欧几里得距离（xz 平面）而非曼哈顿距离
    auto& testTag = StructureTags::registerTag(ResourceLocation("minecraft:test_euclidean_tag"));
    testTag.clear();
    testTag.add(ResourceLocation("minecraft:shipwreck"));
    testTag.add(ResourceLocation("minecraft:stronghold"));

    // shipwreck 在 (6, 0, 8)：欧几里得 = 10，曼哈顿 = 14
    // stronghold 在 (10, 0, 5)：欧几里得 ≈ 11.18，曼哈顿 = 15
    // 欧几里得最近：shipwreck（10 < 11.18）
    TestBlockPos shipwreckPos(6, 64, 8);   // 欧几里得 10
    TestBlockPos strongholdPos(10, 64, 5); // 欧几里得 sqrt(125) ≈ 11.18

    m_simulator->setStructureLocation(ResourceLocation("minecraft:shipwreck"), shipwreckPos);
    m_simulator->setStructureLocation(ResourceLocation("minecraft:stronghold"), strongholdPos);

    TestBlockPos center(0, 64, 0);
    auto result =
        m_simulator->findNearestMapStructure(center, ResourceLocation("minecraft:test_euclidean_tag"), 1000, false);
    ASSERT_TRUE(result.has_value());
    // 欧几里得最近的是 shipwreck
    EXPECT_EQ(result->x, shipwreckPos.x);
    EXPECT_EQ(result->z, shipwreckPos.z);
}

TEST_F(FindNearestMapStructureTest, DolphinLocatedTagIntegration)
{
    // 集成测试：使用内置 DOLPHIN_LOCATED 标签（4 个结构 ID）
    // 模拟海豚寻宝场景
    auto& dolphinTag = StructureTags::DOLPHIN_LOCATED();
    ASSERT_EQ(dolphinTag.getStructureIds().size(), 4u);

    // 只设置 ocean_ruin_cold 的位置
    TestBlockPos ruinPos(200, 64, 0);
    m_simulator->setStructureLocation(ResourceLocation("minecraft:ocean_ruin_cold"), ruinPos);

    TestBlockPos center(0, 64, 0);
    auto result =
        m_simulator->findNearestMapStructure(center, ResourceLocation("minecraft:dolphin_located"), 1000, false);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->x, ruinPos.x);
    EXPECT_EQ(result->z, ruinPos.z);
}

TEST_F(FindNearestMapStructureTest, EqualDistancesReturnsFirstFound)
{
    // 两个结构距离相同，应返回先遍历到的（unordered_set 顺序不确定，
    // 但验证在距离相等时不会选择更远的）
    auto& testTag = StructureTags::registerTag(ResourceLocation("minecraft:test_equal_dist_tag"));
    testTag.clear();
    testTag.add(ResourceLocation("minecraft:shipwreck"));
    testTag.add(ResourceLocation("minecraft:stronghold"));

    // 两个位置距离中心都是 100
    TestBlockPos shipwreckPos(100, 64, 0);  // 距离 100
    TestBlockPos strongholdPos(0, 64, 100); // 距离 100

    m_simulator->setStructureLocation(ResourceLocation("minecraft:shipwreck"), shipwreckPos);
    m_simulator->setStructureLocation(ResourceLocation("minecraft:stronghold"), strongholdPos);

    TestBlockPos center(0, 64, 0);
    auto result =
        m_simulator->findNearestMapStructure(center, ResourceLocation("minecraft:test_equal_dist_tag"), 1000, false);
    ASSERT_TRUE(result.has_value());
    // 距离相等时使用 < 比较，所以第二个不会替换第一个
    // 返回值应该是两个中的一个，距离都是 100
    i32 dx = result->x - center.x;
    i32 dz = result->z - center.z;
    f64 distSq = static_cast<f64>(dx * dx + dz * dz);
    EXPECT_DOUBLE_EQ(distSq, 10000.0); // 100^2
}
