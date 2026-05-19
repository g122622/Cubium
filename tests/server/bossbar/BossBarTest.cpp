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
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/CustomServerBossInfo.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/bossbar/ServerBossInfo.hpp"

using namespace mc;
using namespace mc::server;

/**
 * @brief Boss 栏系统测试套件
 *
 * 测试 Boss 栏系统的核心功能：
 * - BossInfo 基类属性设置
 * - 颜色和样式枚举转换
 * - NBT 持久化基础功能
 */
class BossBarTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

// ========== BossInfo 颜色和样式测试 ==========

TEST_F(BossBarTest, ColorFromName_AllColors)
{
    EXPECT_EQ(bossInfoColorFromName("pink"), BossInfoColor::Pink);
    EXPECT_EQ(bossInfoColorFromName("blue"), BossInfoColor::Blue);
    EXPECT_EQ(bossInfoColorFromName("red"), BossInfoColor::Red);
    EXPECT_EQ(bossInfoColorFromName("green"), BossInfoColor::Green);
    EXPECT_EQ(bossInfoColorFromName("yellow"), BossInfoColor::Yellow);
    EXPECT_EQ(bossInfoColorFromName("purple"), BossInfoColor::Purple);
    EXPECT_EQ(bossInfoColorFromName("white"), BossInfoColor::White);
}

TEST_F(BossBarTest, ColorFromName_InvalidReturnsWhite)
{
    EXPECT_EQ(bossInfoColorFromName("invalid"), BossInfoColor::White);
    EXPECT_EQ(bossInfoColorFromName(""), BossInfoColor::White);
    EXPECT_EQ(bossInfoColorFromName("BLACK"), BossInfoColor::White);
}

TEST_F(BossBarTest, ColorToName_AllColors)
{
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::Pink), "pink");
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::Blue), "blue");
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::Red), "red");
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::Green), "green");
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::Yellow), "yellow");
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::Purple), "purple");
    EXPECT_EQ(bossInfoColorToName(BossInfoColor::White), "white");
}

TEST_F(BossBarTest, OverlayFromName_AllStyles)
{
    EXPECT_EQ(bossInfoOverlayFromName("progress"), BossInfoOverlay::Progress);
    EXPECT_EQ(bossInfoOverlayFromName("notched_6"), BossInfoOverlay::Notched6);
    EXPECT_EQ(bossInfoOverlayFromName("notched_10"), BossInfoOverlay::Notched10);
    EXPECT_EQ(bossInfoOverlayFromName("notched_12"), BossInfoOverlay::Notched12);
    EXPECT_EQ(bossInfoOverlayFromName("notched_20"), BossInfoOverlay::Notched20);
}

TEST_F(BossBarTest, OverlayFromName_InvalidReturnsProgress)
{
    EXPECT_EQ(bossInfoOverlayFromName("invalid"), BossInfoOverlay::Progress);
    EXPECT_EQ(bossInfoOverlayFromName(""), BossInfoOverlay::Progress);
}

TEST_F(BossBarTest, OverlayToName_AllStyles)
{
    EXPECT_EQ(bossInfoOverlayToName(BossInfoOverlay::Progress), "progress");
    EXPECT_EQ(bossInfoOverlayToName(BossInfoOverlay::Notched6), "notched_6");
    EXPECT_EQ(bossInfoOverlayToName(BossInfoOverlay::Notched10), "notched_10");
    EXPECT_EQ(bossInfoOverlayToName(BossInfoOverlay::Notched12), "notched_12");
    EXPECT_EQ(bossInfoOverlayToName(BossInfoOverlay::Notched20), "notched_20");
}

// ========== BossInfo 基类测试 ==========

TEST_F(BossBarTest, BossInfo_BasicProperties)
{
    auto name = std::make_unique<text::StringTextComponent>("Test Boss");
    BossInfo bossInfo(12345ULL, std::move(name), BossInfoColor::Red, BossInfoOverlay::Notched10);

    EXPECT_EQ(bossInfo.uuid(), 12345ULL);
    EXPECT_EQ(bossInfo.name().getUnformattedText(), "Test Boss");
    EXPECT_EQ(bossInfo.color(), BossInfoColor::Red);
    EXPECT_EQ(bossInfo.overlay(), BossInfoOverlay::Notched10);
    EXPECT_FLOAT_EQ(bossInfo.percent(), 1.0f); // 默认 100%
    EXPECT_FALSE(bossInfo.darkenSky());
    EXPECT_FALSE(bossInfo.playEndBossMusic());
    EXPECT_FALSE(bossInfo.createFog());
    EXPECT_TRUE(bossInfo.visible()); // 默认可见
}

TEST_F(BossBarTest, BossInfo_SetPercent_Clamped)
{
    auto name = std::make_unique<text::StringTextComponent>("Test");
    BossInfo bossInfo(1ULL, std::move(name), BossInfoColor::White, BossInfoOverlay::Progress);

    // 测试正常范围
    bossInfo.setPercent(0.5f);
    EXPECT_FLOAT_EQ(bossInfo.percent(), 0.5f);

    // 测试下限
    bossInfo.setPercent(-0.5f);
    EXPECT_FLOAT_EQ(bossInfo.percent(), 0.0f);

    // 测试上限
    bossInfo.setPercent(1.5f);
    EXPECT_FLOAT_EQ(bossInfo.percent(), 1.0f);
}

TEST_F(BossBarTest, BossInfo_SetName)
{
    auto name = std::make_unique<text::StringTextComponent>("Old Name");
    BossInfo bossInfo(1ULL, std::move(name), BossInfoColor::White, BossInfoOverlay::Progress);

    EXPECT_EQ(bossInfo.name().getUnformattedText(), "Old Name");

    auto newName = std::make_unique<text::StringTextComponent>("New Name");
    bossInfo.setName(std::move(newName));
    EXPECT_EQ(bossInfo.name().getUnformattedText(), "New Name");
}

TEST_F(BossBarTest, BossInfo_SetFlags)
{
    auto name = std::make_unique<text::StringTextComponent>("Test");
    BossInfo bossInfo(1ULL, std::move(name), BossInfoColor::White, BossInfoOverlay::Progress);

    bossInfo.setDarkenSky(true);
    EXPECT_TRUE(bossInfo.darkenSky());

    bossInfo.setPlayEndBossMusic(true);
    EXPECT_TRUE(bossInfo.playEndBossMusic());

    bossInfo.setCreateFog(true);
    EXPECT_TRUE(bossInfo.createFog());

    bossInfo.setVisible(false);
    EXPECT_FALSE(bossInfo.visible());
}

// ========== NBT 序列化/反序列化测试 ==========

TEST_F(BossBarTest, BossInfoColor_RoundTrip)
{
    // 测试颜色名称转换的双向一致性
    for (const auto& name : {"pink", "blue", "red", "green", "yellow", "purple", "white"}) {
        BossInfoColor color = bossInfoColorFromName(name);
        std::string resultName = bossInfoColorToName(color);
        EXPECT_EQ(resultName, name);
    }
}

TEST_F(BossBarTest, BossInfoOverlay_RoundTrip)
{
    // 测试样式名称转换的双向一致性
    for (const auto& name : {"progress", "notched_6", "notched_10", "notched_12", "notched_20"}) {
        BossInfoOverlay overlay = bossInfoOverlayFromName(name);
        std::string resultName = bossInfoOverlayToName(overlay);
        EXPECT_EQ(resultName, name);
    }
}

// ========== ResourceLocation 测试（用于 Boss 栏 ID） ==========

TEST_F(BossBarTest, ResourceLocation_BossBarId)
{
    ResourceLocation id1("minecraft:test_bossbar");
    EXPECT_EQ(id1.namespace_(), "minecraft");
    EXPECT_EQ(id1.path(), "test_bossbar");
    EXPECT_EQ(id1.toString(), "minecraft:test_bossbar");

    ResourceLocation id2("my_bossbar");
    EXPECT_EQ(id2.namespace_(), "minecraft"); // 默认命名空间
    EXPECT_EQ(id2.path(), "my_bossbar");

    ResourceLocation id3("custom", "my_boss");
    EXPECT_EQ(id3.namespace_(), "custom");
    EXPECT_EQ(id3.path(), "my_boss");
}

// ========== NBT 基础测试 ==========

TEST_F(BossBarTest, NbtString_RoundTrip)
{
    // 测试 NBT 字符串读写（用于 Boss 栏持久化）
    nbt::tags::compound_tag tag;
    tag.put("Name", std::string("Test Boss"));
    tag.put("Color", std::string("red"));
    tag.put("Value", static_cast<i32>(50));
    tag.put("Max", static_cast<i32>(100));

    auto nameIt = tag.value.find("Name");
    ASSERT_NE(nameIt, tag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*nameIt->second).value, "Test Boss");

    auto colorIt = tag.value.find("Color");
    ASSERT_NE(colorIt, tag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*colorIt->second).value, "red");

    auto valueIt = tag.value.find("Value");
    ASSERT_NE(valueIt, tag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*valueIt->second).value, 50);

    auto maxIt = tag.value.find("Max");
    ASSERT_NE(maxIt, tag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*maxIt->second).value, 100);
}
