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

#include <vector>

#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/world/gameevent/GameEventTagLoader.hpp"
#include "common/world/gameevent/GameEventTags.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

using namespace mc;
using namespace mc::gameevent;

class GameEventTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { GameEventTags::initialize(); }
};

TEST_F(GameEventTagLoaderTest, BuiltinTagsInitialized)
{
    // 内置标签须与数据包一致
    auto& ignoreSneaking = GameEventTags::IGNORE_VIBRATIONS_SNEAKING();
    EXPECT_TRUE(ignoreSneaking.contains(GameEvents::STEP));
    EXPECT_TRUE(ignoreSneaking.contains(GameEvents::SWIM));
    EXPECT_TRUE(ignoreSneaking.contains(GameEvents::HIT_GROUND));
    EXPECT_FALSE(ignoreSneaking.contains(GameEvents::EXPLODE));

    EXPECT_TRUE(GameEventTags::ALLAY_CAN_LISTEN().contains(GameEvents::NOTE_BLOCK_PLAY));
    EXPECT_TRUE(GameEventTags::SHRIEKER_CAN_LISTEN().contains(GameEvents::SCULK_SENSOR_TENDRILS_CLICKING));
    EXPECT_TRUE(GameEventTags::WARDEN_CAN_LISTEN().contains(GameEvents::SHRIEK));
    EXPECT_TRUE(GameEventTags::WARDEN_CAN_LISTEN().contains(GameEvents::STEP));
}

TEST_F(GameEventTagLoaderTest, IsIgnoredBySneakingUsesTag)
{
    // VibrationSystem::isIgnoredBySneaking 须经标签查询（而非硬编码）
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::STEP));
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::ITEM_INTERACT_FINISH));
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::EXPLODE));
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::BLOCK_PLACE));
}

TEST_F(GameEventTagLoaderTest, LoadFromResourcePackBasicTag)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("game_event_tag_pack");
    pack->addServerDataResource("minecraft/tags/game_event/test_custom_events.json",
        R"({
            "values": [
                "minecraft:step",
                "minecraft:swim"
            ]
        })");

    auto result = GameEventTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value(), 1u);

    auto* tag = GameEventTags::getTag(ResourceLocation("minecraft", "test_custom_events"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(GameEvents::STEP));
    EXPECT_TRUE(tag->contains(GameEvents::SWIM));
    EXPECT_EQ(tag->getEvents().size(), 2u);
}

TEST_F(GameEventTagLoaderTest, UnknownGameEventIsSkipped)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("unknown_game_event_pack");
    pack->addServerDataResource("minecraft/tags/game_event/test_unknown_event.json",
        R"({
            "values": [
                "minecraft:step",
                "minecraft:not_a_real_event"
            ]
        })");

    auto result = GameEventTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = GameEventTags::getTag(ResourceLocation("minecraft", "test_unknown_event"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(GameEvents::STEP));
    EXPECT_EQ(tag->getEvents().size(), 1u);
}

TEST_F(GameEventTagLoaderTest, TagReferenceResolvesMembers)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("ref_game_event_pack");
    pack->addServerDataResource("minecraft/tags/game_event/test_event_a.json", R"({"values": ["minecraft:step"]})");
    pack->addServerDataResource("minecraft/tags/game_event/test_event_b.json", R"({"values": ["minecraft:swim"]})");
    pack->addServerDataResource("minecraft/tags/game_event/test_event_all.json",
        R"({"values": ["#minecraft:test_event_a", "#minecraft:test_event_b"]})");

    auto result = GameEventTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = GameEventTags::getTag(ResourceLocation("minecraft", "test_event_all"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(GameEvents::STEP));
    EXPECT_TRUE(tag->contains(GameEvents::SWIM));
    EXPECT_EQ(tag->getEvents().size(), 2u);
}

TEST_F(GameEventTagLoaderTest, ReplaceSemanticsClearsBuiltinTag)
{
    // 预注册一个含内置成员的标签，数据包 replace=true 应清空后仅保留新事件
    auto& builtin = GameEventTags::registerTag(ResourceLocation("minecraft", "test_event_replace"));
    builtin.add(&GameEvents::STEP);

    auto pack = std::make_unique<mc::InMemoryResourcePack>("replace_game_event_pack");
    pack->addServerDataResource("minecraft/tags/game_event/test_event_replace.json",
        R"({
            "replace": true,
            "values": ["minecraft:explode"]
        })");

    auto result = GameEventTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = GameEventTags::getTag(ResourceLocation("minecraft", "test_event_replace"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(GameEvents::EXPLODE));
    // 原有成员（step）被 replace 清除
    EXPECT_FALSE(tag->contains(GameEvents::STEP));
    EXPECT_EQ(tag->getEvents().size(), 1u);
}

TEST_F(GameEventTagLoaderTest, IgnoreSneakingTagDrivesVibrationSystem)
{
    // 验证 isIgnoredBySneaking 完全由 ignore_vibrations_sneaking 标签驱动：
    // 向该标签追加一个原本不忽略的事件后，消费侧应随之改变。
    GameEventTags::IGNORE_VIBRATIONS_SNEAKING().add(&GameEvents::EXPLODE);
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::EXPLODE));
    // 复原，避免污染后续用例
    auto& tag = GameEventTags::IGNORE_VIBRATIONS_SNEAKING();
    std::vector<const GameEvent*> remaining;
    for (const auto* event : tag.getEvents()) {
        if (event != &GameEvents::EXPLODE) {
            remaining.push_back(event);
        }
    }
    tag.clear();
    tag.addAll(remaining);
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::EXPLODE));
}
