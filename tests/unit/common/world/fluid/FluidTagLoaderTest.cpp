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
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidTagLoader.hpp"
#include "common/world/fluid/FluidTags.hpp"

using namespace mc;

class FluidTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 流体注册表 + 内置流体标签初始化（随 VanillaBlocks 一并完成）
        VanillaBlocks::initialize();
    }
};

TEST_F(FluidTagLoaderTest, LoadFromResourcePackBasicTag)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("fluid_tag_pack");
    pack->addServerDataResource("minecraft/tags/fluid/test_water_like.json",
        R"({
            "values": [
                "minecraft:water",
                "minecraft:flowing_water"
            ]
        })");

    auto result = FluidTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value(), 1u);

    auto* tag = fluid::FluidTags::getTag(ResourceLocation("minecraft", "test_water_like"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->fluids().count(ResourceLocation("minecraft:water")) > 0);
    EXPECT_TRUE(tag->fluids().count(ResourceLocation("minecraft:flowing_water")) > 0);
    EXPECT_EQ(tag->fluids().size(), 2u);
}

TEST_F(FluidTagLoaderTest, UnknownFluidMemberIsSkipped)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("unknown_fluid_pack");
    pack->addServerDataResource("minecraft/tags/fluid/test_unknown_fluid.json",
        R"({
            "values": [
                "minecraft:lava",
                "minecraft:not_a_real_fluid"
            ]
        })");

    auto result = FluidTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = fluid::FluidTags::getTag(ResourceLocation("minecraft", "test_unknown_fluid"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->fluids().count(ResourceLocation("minecraft:lava")) > 0);
    EXPECT_EQ(tag->fluids().size(), 1u);
}

TEST_F(FluidTagLoaderTest, TagReferenceResolvesMembers)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("ref_fluid_pack");
    pack->addServerDataResource("minecraft/tags/fluid/test_fluid_a.json", R"({"values": ["minecraft:water"]})");
    pack->addServerDataResource("minecraft/tags/fluid/test_fluid_b.json", R"({"values": ["minecraft:lava"]})");
    pack->addServerDataResource("minecraft/tags/fluid/test_fluid_all.json",
        R"({"values": ["#minecraft:test_fluid_a", "#minecraft:test_fluid_b"]})");

    auto result = FluidTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = fluid::FluidTags::getTag(ResourceLocation("minecraft", "test_fluid_all"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->fluids().count(ResourceLocation("minecraft:water")) > 0);
    EXPECT_TRUE(tag->fluids().count(ResourceLocation("minecraft:lava")) > 0);
    EXPECT_EQ(tag->fluids().size(), 2u);
}

TEST_F(FluidTagLoaderTest, ReplaceSemanticsClearsBuiltinTag)
{
    auto& builtin = fluid::FluidTags::registerTag(ResourceLocation("minecraft", "test_fluid_replace_builtin"));
    builtin.add(ResourceLocation("minecraft:flowing_lava"));

    auto pack = std::make_unique<mc::InMemoryResourcePack>("replace_fluid_pack");
    pack->addServerDataResource("minecraft/tags/fluid/test_fluid_replace_builtin.json",
        R"({
            "replace": true,
            "values": ["minecraft:water"]
        })");

    auto result = FluidTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = fluid::FluidTags::getTag(ResourceLocation("minecraft", "test_fluid_replace_builtin"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->fluids().count(ResourceLocation("minecraft:water")) > 0);
    EXPECT_EQ(tag->fluids().count(ResourceLocation("minecraft:flowing_lava")), 0u);
}

TEST_F(FluidTagLoaderTest, DataPackAppendsToBuiltinTag)
{
    // 数据包可对已注册的标签追加成员，且保留原有成员
    auto& builtin = fluid::FluidTags::registerTag(ResourceLocation("minecraft", "test_fluid_builtin_append"));
    builtin.add(ResourceLocation("minecraft:water"));

    auto pack = std::make_unique<mc::InMemoryResourcePack>("append_fluid_builtin_pack");
    pack->addServerDataResource(
        "minecraft/tags/fluid/test_fluid_builtin_append.json", R"({"values": ["minecraft:lava"]})");

    auto result = FluidTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* after = fluid::FluidTags::getTag(ResourceLocation("minecraft", "test_fluid_builtin_append"));
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->fluids().count(ResourceLocation("minecraft:water")) > 0);
    EXPECT_TRUE(after->fluids().count(ResourceLocation("minecraft:lava")) > 0);
}
