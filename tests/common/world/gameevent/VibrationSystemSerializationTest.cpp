/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::gameevent;

// ============================================================================
// VibrationInfo NBT 序列化测试
// ============================================================================

TEST(VibrationInfoSerializationTest, SaveToNBT_BasicFields)
{
    GameEvent event("block_activate", 16);
    Vector3d pos(10.5, 64.0, -3.25);
    VibrationInfo info(event, 7.5f, pos, nullptr);

    nbt::CompoundTag tag;
    info.saveToNBT(tag);

    // 验证 game_event
    auto eventId = entity::serialization::nbt_helper::tryGetString(tag, "game_event");
    ASSERT_TRUE(eventId.has_value());
    EXPECT_EQ(eventId.value(), "block_activate");

    // 验证 distance
    auto dist = entity::serialization::nbt_helper::tryGetFloat(tag, "distance");
    ASSERT_TRUE(dist.has_value());
    EXPECT_FLOAT_EQ(dist.value(), 7.5f);

    // 验证 pos
    auto* posList = entity::serialization::nbt_helper::tryGetList(tag, "pos");
    ASSERT_NE(posList, nullptr);
    EXPECT_EQ(posList->element_id(), nbt::TagId::Double);
    const auto& doubles = dynamic_cast<const nbt::tags::double_list_tag&>(*posList);
    ASSERT_GE(doubles.value.size(), 3u);
    EXPECT_DOUBLE_EQ(doubles.value[0], 10.5);
    EXPECT_DOUBLE_EQ(doubles.value[1], 64.0);
    EXPECT_DOUBLE_EQ(doubles.value[2], -3.25);

    // 无源实体时不应保存 source
    auto source = entity::serialization::nbt_helper::tryGetLong(tag, "source");
    EXPECT_FALSE(source.has_value());
}

TEST(VibrationInfoSerializationTest, SaveToNBT_WithSourceEntity)
{
    GameEvent event("step", 16);
    Vector3d pos(1.0, 2.0, 3.0);
    VibrationInfo info(event, 3.0f, pos, nullptr);
    info.sourceEntityId = 42;
    info.hasSourceEntity = true;

    nbt::CompoundTag tag;
    info.saveToNBT(tag);

    auto source = entity::serialization::nbt_helper::tryGetLong(tag, "source");
    ASSERT_TRUE(source.has_value());
    EXPECT_EQ(source.value(), 42);
}

TEST(VibrationInfoSerializationTest, LoadFromNBT_RoundTrip)
{
    GameEvent event("entity_damage", 16);
    Vector3d pos(100.0, -5.5, 200.0);
    VibrationInfo original(event, 12.3f, pos, nullptr);
    original.sourceEntityId = 99;
    original.hasSourceEntity = true;

    // 保存
    nbt::CompoundTag tag;
    original.saveToNBT(tag);

    // 加载
    VibrationInfo loaded;
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    EXPECT_NE(loaded.gameEvent, nullptr);
    EXPECT_STREQ(loaded.gameEvent->id(), "entity_damage");
    EXPECT_FLOAT_EQ(loaded.distance, 12.3f);
    EXPECT_DOUBLE_EQ(loaded.pos.x, 100.0);
    EXPECT_DOUBLE_EQ(loaded.pos.y, -5.5);
    EXPECT_DOUBLE_EQ(loaded.pos.z, 200.0);
    EXPECT_TRUE(loaded.hasSourceEntity);
    EXPECT_EQ(loaded.sourceEntityId, 99u);
}

TEST(VibrationInfoSerializationTest, LoadFromNBT_MissingGameEvent)
{
    nbt::CompoundTag tag;
    // 不设置 game_event，应返回 false
    VibrationInfo info;
    EXPECT_FALSE(info.loadFromNBT(tag));
}

TEST(VibrationInfoSerializationTest, LoadFromNBT_UnknownEvent)
{
    nbt::CompoundTag tag;
    tag.put("game_event", std::string("unknown_event_id"));
    tag.put("distance", 5.0f);

    VibrationInfo info;
    // 未知事件应成功加载，但 gameEvent 为 nullptr
    EXPECT_TRUE(info.loadFromNBT(tag));
    EXPECT_EQ(info.gameEvent, nullptr);
}

// ============================================================================
// VibrationInfo JSON 序列化测试
// ============================================================================

TEST(VibrationInfoSerializationTest, SaveToJson_BasicFields)
{
    GameEvent event("shriek", 32);
    Vector3d pos(5.0, 10.0, 15.0);
    VibrationInfo info(event, 20.0f, pos, nullptr);

    nlohmann::json data;
    info.saveToJson(data);

    EXPECT_EQ(data["game_event"], "shriek");
    EXPECT_FLOAT_EQ(data["distance"].get<f32>(), 20.0f);
    ASSERT_TRUE(data["pos"].is_array());
    ASSERT_EQ(data["pos"].size(), 3u);
    EXPECT_DOUBLE_EQ(data["pos"][0].get<f64>(), 5.0);
    EXPECT_DOUBLE_EQ(data["pos"][1].get<f64>(), 10.0);
    EXPECT_DOUBLE_EQ(data["pos"][2].get<f64>(), 15.0);
    EXPECT_FALSE(data.contains("source"));
}

TEST(VibrationInfoSerializationTest, LoadFromJson_RoundTrip)
{
    GameEvent event("resonate_5", 16);
    Vector3d pos(-10.0, 0.5, 300.0);
    VibrationInfo original(event, 8.0f, pos, nullptr);
    original.sourceEntityId = 123;
    original.hasSourceEntity = true;

    nlohmann::json data;
    original.saveToJson(data);

    VibrationInfo loaded;
    ASSERT_TRUE(loaded.loadFromJson(data));

    EXPECT_NE(loaded.gameEvent, nullptr);
    EXPECT_STREQ(loaded.gameEvent->id(), "resonate_5");
    EXPECT_FLOAT_EQ(loaded.distance, 8.0f);
    EXPECT_DOUBLE_EQ(loaded.pos.x, -10.0);
    EXPECT_DOUBLE_EQ(loaded.pos.y, 0.5);
    EXPECT_DOUBLE_EQ(loaded.pos.z, 300.0);
    EXPECT_TRUE(loaded.hasSourceEntity);
    EXPECT_EQ(loaded.sourceEntityId, 123u);
}

// ============================================================================
// VibrationSelector NBT 序列化测试
// ============================================================================

TEST(VibrationSelectorSerializationTest, SaveToNBT_NoCandidate)
{
    VibrationSelector selector;

    nbt::CompoundTag tag;
    selector.saveToNBT(tag);

    // 无候选时 tick 应为 -1，无 event 字段
    auto tick = entity::serialization::nbt_helper::tryGetLong(tag, "tick");
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick.value(), -1);

    auto* eventTag = entity::serialization::nbt_helper::tryGetCompound(tag, "event");
    EXPECT_EQ(eventTag, nullptr);
}

TEST(VibrationSelectorSerializationTest, SaveToNBT_WithCandidate)
{
    VibrationSelector selector;
    GameEvent event("block_place", 16);
    VibrationInfo info(event, 5.0f, Vector3d(0, 0, 0), nullptr);
    selector.addCandidate(info, 100);

    nbt::CompoundTag tag;
    selector.saveToNBT(tag);

    auto tick = entity::serialization::nbt_helper::tryGetLong(tag, "tick");
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick.value(), 100);

    auto* eventTag = entity::serialization::nbt_helper::tryGetCompound(tag, "event");
    ASSERT_NE(eventTag, nullptr);

    auto eventId = entity::serialization::nbt_helper::tryGetString(*eventTag, "game_event");
    ASSERT_TRUE(eventId.has_value());
    EXPECT_EQ(eventId.value(), "block_place");
}

TEST(VibrationSelectorSerializationTest, LoadFromNBT_RoundTrip)
{
    VibrationSelector selector;
    GameEvent event("swim", 16);
    VibrationInfo info(event, 3.5f, Vector3d(10.0, 20.0, 30.0), nullptr);
    selector.addCandidate(info, 42);

    nbt::CompoundTag tag;
    selector.saveToNBT(tag);

    VibrationSelector loaded;
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    // 在 tick 43 应该能选择到候选
    auto chosen = loaded.chosenCandidate(43);
    ASSERT_TRUE(chosen.has_value());
    EXPECT_NE(chosen->gameEvent, nullptr);
    EXPECT_STREQ(chosen->gameEvent->id(), "swim");
    EXPECT_FLOAT_EQ(chosen->distance, 3.5f);
}

TEST(VibrationSelectorSerializationTest, LoadFromNBT_Empty)
{
    nbt::CompoundTag tag;
    tag.put("tick", static_cast<i64>(-1));

    VibrationSelector loaded;
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    auto chosen = loaded.chosenCandidate(100);
    EXPECT_FALSE(chosen.has_value());
}

// ============================================================================
// VibrationSelector JSON 序列化测试
// ============================================================================

TEST(VibrationSelectorSerializationTest, SaveToJson_NoCandidate)
{
    VibrationSelector selector;

    nlohmann::json data;
    selector.saveToJson(data);

    EXPECT_EQ(data["tick"], -1);
    EXPECT_FALSE(data.contains("event"));
}

TEST(VibrationSelectorSerializationTest, LoadFromJson_RoundTrip)
{
    VibrationSelector selector;
    GameEvent event("explode", 16);
    VibrationInfo info(event, 10.0f, Vector3d(5, 5, 5), nullptr);
    selector.addCandidate(info, 200);

    nlohmann::json data;
    selector.saveToJson(data);

    VibrationSelector loaded;
    ASSERT_TRUE(loaded.loadFromJson(data));

    auto chosen = loaded.chosenCandidate(201);
    ASSERT_TRUE(chosen.has_value());
    EXPECT_NE(chosen->gameEvent, nullptr);
    EXPECT_STREQ(chosen->gameEvent->id(), "explode");
    EXPECT_FLOAT_EQ(chosen->distance, 10.0f);
}

// ============================================================================
// VibrationSystem::Data NBT 序列化测试
// ============================================================================

TEST(VibrationDataSerializationTest, SaveToNBT_EmptyData)
{
    VibrationSystem::Data data;

    nbt::CompoundTag tag;
    data.saveToNBT(tag);

    // 空数据不应有 event 字段
    auto* eventTag = entity::serialization::nbt_helper::tryGetCompound(tag, "event");
    EXPECT_EQ(eventTag, nullptr);

    // selector 应存在
    auto* selectorTag = entity::serialization::nbt_helper::tryGetCompound(tag, "selector");
    EXPECT_NE(selectorTag, nullptr);

    // event_delay 应为 0
    auto delay = entity::serialization::nbt_helper::tryGetInt(tag, "event_delay");
    ASSERT_TRUE(delay.has_value());
    EXPECT_EQ(delay.value(), 0);
}

TEST(VibrationDataSerializationTest, SaveToNBT_WithCurrentVibration)
{
    GameEvent event("step", 16);
    VibrationInfo info(event, 4.0f, Vector3d(1.0, 2.0, 3.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data data(info, std::move(selector), 5, false);

    nbt::CompoundTag tag;
    data.saveToNBT(tag);

    // event 应存在
    auto* eventTag = entity::serialization::nbt_helper::tryGetCompound(tag, "event");
    ASSERT_NE(eventTag, nullptr);

    auto eventId = entity::serialization::nbt_helper::tryGetString(*eventTag, "game_event");
    ASSERT_TRUE(eventId.has_value());
    EXPECT_EQ(eventId.value(), "step");

    // event_delay 应为 5
    auto delay = entity::serialization::nbt_helper::tryGetInt(tag, "event_delay");
    ASSERT_TRUE(delay.has_value());
    EXPECT_EQ(delay.value(), 5);
}

TEST(VibrationDataSerializationTest, LoadFromNBT_RoundTrip)
{
    // 创建带振动的 Data
    GameEvent event("block_activate", 16);
    VibrationInfo info(event, 6.0f, Vector3d(10.0, 20.0, 30.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data original(info, std::move(selector), 8, false);

    // 保存
    nbt::CompoundTag tag;
    original.saveToNBT(tag);

    // 加载
    VibrationSystem::Data loaded;
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    // 验证加载后数据
    ASSERT_NE(loaded.currentVibration(), nullptr);
    EXPECT_STREQ(loaded.currentVibration()->gameEvent->id(), "block_activate");
    EXPECT_FLOAT_EQ(loaded.currentVibration()->distance, 6.0f);
    EXPECT_EQ(loaded.travelTimeInTicks(), 8);

    // 关键：从存档加载时 reloadVibrationParticle 必须为 true
    EXPECT_TRUE(loaded.shouldReloadVibrationParticle());
}

TEST(VibrationDataSerializationTest, LoadFromNBT_SetsReloadVibrationParticleTrue)
{
    // 即使保存时 reloadVibrationParticle = false，
    // 加载后也必须为 true（对齐 MC 原版 CODEC 行为）
    VibrationSystem::Data original(std::nullopt, VibrationSelector(), 0, false);
    EXPECT_FALSE(original.shouldReloadVibrationParticle());

    nbt::CompoundTag tag;
    original.saveToNBT(tag);

    VibrationSystem::Data loaded;
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    // 从存档加载后 reloadVibrationParticle 必须为 true
    EXPECT_TRUE(loaded.shouldReloadVibrationParticle());
}

TEST(VibrationDataSerializationTest, LoadFromNBT_EmptyData)
{
    // 加载空 listener 数据
    nbt::CompoundTag tag;
    // 只设置 selector 为空
    auto selectorTag = std::make_unique<nbt::CompoundTag>();
    selectorTag->put("tick", static_cast<i64>(-1));
    tag.value.emplace("selector", std::move(selectorTag));
    tag.put("event_delay", static_cast<i32>(0));

    VibrationSystem::Data loaded;
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    EXPECT_EQ(loaded.currentVibration(), nullptr);
    EXPECT_EQ(loaded.travelTimeInTicks(), 0);
    EXPECT_TRUE(loaded.shouldReloadVibrationParticle());
}

// ============================================================================
// VibrationSystem::Data JSON 序列化测试
// ============================================================================

TEST(VibrationDataSerializationTest, SaveToJson_EmptyData)
{
    VibrationSystem::Data data;

    nlohmann::json json;
    data.saveToJson(json);

    EXPECT_FALSE(json.contains("event"));
    EXPECT_TRUE(json.contains("selector"));
    EXPECT_TRUE(json.contains("event_delay"));
    EXPECT_EQ(json["event_delay"], 0);
}

TEST(VibrationDataSerializationTest, LoadFromJson_RoundTrip)
{
    GameEvent event("eat", 16);
    VibrationInfo info(event, 2.0f, Vector3d(0, 0, 0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data original(info, std::move(selector), 3, true);

    nlohmann::json json;
    original.saveToJson(json);

    VibrationSystem::Data loaded;
    ASSERT_TRUE(loaded.loadFromJson(json));

    ASSERT_NE(loaded.currentVibration(), nullptr);
    EXPECT_STREQ(loaded.currentVibration()->gameEvent->id(), "eat");
    EXPECT_FLOAT_EQ(loaded.currentVibration()->distance, 2.0f);
    EXPECT_EQ(loaded.travelTimeInTicks(), 3);
    EXPECT_TRUE(loaded.shouldReloadVibrationParticle());
}

TEST(VibrationDataSerializationTest, LoadFromJson_SetsReloadVibrationParticleTrue)
{
    VibrationSystem::Data original(std::nullopt, VibrationSelector(), 0, false);

    nlohmann::json json;
    original.saveToJson(json);

    VibrationSystem::Data loaded;
    ASSERT_TRUE(loaded.loadFromJson(json));

    EXPECT_TRUE(loaded.shouldReloadVibrationParticle());
}

// ============================================================================
// GameEvents::getGameEventById 测试
// ============================================================================

TEST(GameEventsLookupTest, GetGameEventById_KnownEvents)
{
    // 测试已知事件的查找
    const GameEvent* step = GameEvents::getGameEventById("step");
    ASSERT_NE(step, nullptr);
    EXPECT_STREQ(step->id(), "step");

    const GameEvent* blockActivate = GameEvents::getGameEventById("block_activate");
    ASSERT_NE(blockActivate, nullptr);
    EXPECT_STREQ(blockActivate->id(), "block_activate");

    const GameEvent* shriek = GameEvents::getGameEventById("shriek");
    ASSERT_NE(shriek, nullptr);
    EXPECT_STREQ(shriek->id(), "shriek");
}

TEST(GameEventsLookupTest, GetGameEventById_ResonateEvents)
{
    // 测试共鸣事件
    for (i32 i = 1; i <= 15; ++i) {
        std::string id = "resonate_" + std::to_string(i);
        const GameEvent* event = GameEvents::getGameEventById(id);
        ASSERT_NE(event, nullptr) << "Missing resonate event: " << id;
        EXPECT_EQ(VibrationSystem::getGameEventFrequency(*event), i);
    }
}

TEST(GameEventsLookupTest, GetGameEventById_UnknownEvent)
{
    const GameEvent* unknown = GameEvents::getGameEventById("nonexistent_event");
    EXPECT_EQ(unknown, nullptr);
}

TEST(GameEventsLookupTest, GetGameEventById_ConsistencyWithConstants)
{
    // 验证查找结果与内联常量是同一指针
    EXPECT_EQ(GameEvents::getGameEventById("step"), &GameEvents::STEP);
    EXPECT_EQ(GameEvents::getGameEventById("shriek"), &GameEvents::SHRIEK);
    EXPECT_EQ(GameEvents::getGameEventById("block_activate"), &GameEvents::BLOCK_ACTIVATE);
    EXPECT_EQ(GameEvents::getGameEventById("resonate_7"), &GameEvents::RESONATE_7);
}
