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

/**
 * @file test_structure_set_loader.cpp
 * @brief StructureSetLoader preferred_biomes 解析单元测试
 *
 * 验证 ConcentricRings 放置（要塞）的 preferred_biomes 标签引用能解析为
 * BiomeId 列表：标签已填充时非空且与标签一致；标签未注册时回退空列表不崩。
 */

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "common/world/gen/structure/StructureSetLoader.hpp"
#include "common/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::world::gen::structure::placement;

namespace {

/// 要塞结构集合 JSON（placement 为 concentric_rings，preferred_biomes 引用标签）
const char* strongholdJson(const std::string& tagRef)
{
    static std::string buf;
    buf = R"({
        "structures": [ { "structure": "minecraft:stronghold", "weight": 1 } ],
        "placement": {
            "type": "minecraft:concentric_rings",
            "distance": 32,
            "spread": 3,
            "count": 128,
            "preferred_biomes": ")" +
        tagRef + R"(",
            "salt": 0
        }
    })";
    return buf.c_str();
}

/// 取已注册的 stronghold_biased_to 标签并填充若干 BiomeId（若尚未注册返回 nullptr）
world::biome::BiomeTag* fillStrongholdTag(const std::vector<BiomeId>& ids)
{
    auto* tag = world::biome::BiomeTags::getTag(ResourceLocation("minecraft", "stronghold_biased_to"));
    if (tag != nullptr) {
        for (BiomeId id : ids) {
            tag->add(id);
        }
    }
    return tag;
}

} // namespace

class StructureSetLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // StructureSetLoader::loadFromJson(string) 不写全局注册表，直接返回 unique_ptr，
        // 故无需 clear。BiomeTags::getTag 内部 call_once 初始化标签键。
    }
};

/// preferred_biomes 标签已填充时，解析出的 ConcentricRings 持有对应 BiomeId 列表
TEST_F(StructureSetLoaderTest, PreferredBiomesResolvedFromTag)
{
    const std::vector<BiomeId> expected = {1, 2, 3, 42};
    auto* tag = fillStrongholdTag(expected);
    ASSERT_NE(tag, nullptr);

    auto result = StructureSetLoader::loadFromJson(
        strongholdJson("#minecraft:stronghold_biased_to"), ResourceLocation("minecraft", "strongholds"));
    ASSERT_TRUE(result.success()) << result.error().message();
    auto set = result.value();
    ASSERT_NE(set, nullptr);

    const auto* rings = dynamic_cast<const ConcentricRingsStructurePlacement*>(&set->placement());
    ASSERT_NE(rings, nullptr);
    const auto& resolved = rings->preferredBiomes();
    EXPECT_EQ(resolved.size(), expected.size());
    for (BiomeId id : expected) {
        EXPECT_NE(std::find(resolved.begin(), resolved.end(), id), resolved.end()) << "missing BiomeId " << id;
    }
}

/// preferred_biomes 引用的标签未注册时，回退空列表不崩（与硬编码兜底一致）
TEST_F(StructureSetLoaderTest, PreferredBiomesFallsBackEmptyForUnknownTag)
{
    auto result = StructureSetLoader::loadFromJson(
        strongholdJson("#minecraft:nonexistent_tag"), ResourceLocation("minecraft", "strongholds"));
    ASSERT_TRUE(result.success()) << result.error().message();
    auto set = result.value();
    ASSERT_NE(set, nullptr);

    const auto* rings = dynamic_cast<const ConcentricRingsStructurePlacement*>(&set->placement());
    ASSERT_NE(rings, nullptr);
    EXPECT_TRUE(rings->preferredBiomes().empty());
}

/// 无 preferred_biomes 字段时，ConcentricRings preferredBiomes 为空
TEST_F(StructureSetLoaderTest, PreferredBiomesEmptyWhenAbsent)
{
    const std::string json = R"({
        "structures": [ { "structure": "minecraft:stronghold", "weight": 1 } ],
        "placement": {
            "type": "minecraft:concentric_rings",
            "distance": 32,
            "spread": 3,
            "count": 128,
            "salt": 0
        }
    })";

    auto result = StructureSetLoader::loadFromJson(json, ResourceLocation("minecraft", "strongholds"));
    ASSERT_TRUE(result.success()) << result.error().message();
    auto set = result.value();
    ASSERT_NE(set, nullptr);

    const auto* rings = dynamic_cast<const ConcentricRingsStructurePlacement*>(&set->placement());
    ASSERT_NE(rings, nullptr);
    EXPECT_TRUE(rings->preferredBiomes().empty());
}
