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

#include "common/TestWorldHelper.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;
using namespace mc::gameevent;

// ============================================================================
// SculkSensorBlockEntity 测试
// ============================================================================

class SculkSensorBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(10, 64, 20); }

    BlockPos pos_;
};

TEST_F(SculkSensorBlockEntityTest, DefaultConstruction)
{
    auto entity = std::make_unique<SculkSensorBlockEntity>(pos_);

    EXPECT_EQ(entity->getType(), BlockEntityType::SculkSensor);
    EXPECT_EQ(entity->getLastVibrationFrequency(), 0);
    EXPECT_EQ(entity->getPos(), pos_);
}

TEST_F(SculkSensorBlockEntityTest, SetLastVibrationFrequency)
{
    auto entity = std::make_unique<SculkSensorBlockEntity>(pos_);

    entity->setLastVibrationFrequency(5);
    EXPECT_EQ(entity->getLastVibrationFrequency(), 5);

    entity->setLastVibrationFrequency(15);
    EXPECT_EQ(entity->getLastVibrationFrequency(), 15);

    entity->setLastVibrationFrequency(0);
    EXPECT_EQ(entity->getLastVibrationFrequency(), 0);
}

TEST_F(SculkSensorBlockEntityTest, VibrationDataAccess)
{
    auto entity = std::make_unique<SculkSensorBlockEntity>(pos_);

    // 默认 VibrationSystem::Data
    EXPECT_EQ(entity->getVibrationData().currentVibration(), nullptr);
    EXPECT_EQ(entity->getVibrationData().travelTimeInTicks(), 0);
    EXPECT_FALSE(entity->getVibrationData().shouldReloadVibrationParticle());

    // 设置振动数据
    GameEvent event("step", 16);
    VibrationInfo info(event, 5.0f, Vector3d(10.5, 64.5, 20.5), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data data(info, std::move(selector), 3, true);
    entity->setVibrationData(std::move(data));

    EXPECT_NE(entity->getVibrationData().currentVibration(), nullptr);
    EXPECT_EQ(entity->getVibrationData().travelTimeInTicks(), 3);
    EXPECT_TRUE(entity->getVibrationData().shouldReloadVibrationParticle());
}

// ============================================================================
// SculkSensorBlockEntity JSON 序列化测试
// ============================================================================

TEST_F(SculkSensorBlockEntityTest, JsonSerialization_EmptyData)
{
    auto entity = std::make_unique<SculkSensorBlockEntity>(pos_);

    nlohmann::json data;
    entity->save(data);

    // 验证基础字段
    EXPECT_TRUE(data.contains("listener"));
    EXPECT_TRUE(data["listener"].is_object());
    EXPECT_TRUE(data.contains("last_vibration_frequency"));
    EXPECT_EQ(data["last_vibration_frequency"], 0);
}

TEST_F(SculkSensorBlockEntityTest, JsonSerialization_WithVibrationAndFrequency)
{
    auto entity = std::make_unique<SculkSensorBlockEntity>(pos_);

    // 设置振动数据
    GameEvent event("block_activate", 16);
    VibrationInfo info(event, 7.5f, Vector3d(10.0, 64.0, 20.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data vData(info, std::move(selector), 4, false);
    entity->setVibrationData(std::move(vData));

    entity->setLastVibrationFrequency(5);

    nlohmann::json data;
    entity->save(data);

    // 验证 listener 数据
    EXPECT_TRUE(data["listener"].contains("event"));
    EXPECT_TRUE(data["listener"].contains("selector"));
    EXPECT_EQ(data["listener"]["event_delay"], 4);

    // 验证频率
    EXPECT_EQ(data["last_vibration_frequency"], 5);
}

TEST_F(SculkSensorBlockEntityTest, JsonRoundTrip)
{
    auto original = std::make_unique<SculkSensorBlockEntity>(pos_);

    GameEvent event("entity_damage", 16);
    VibrationInfo info(event, 3.0f, Vector3d(1.0, 2.0, 3.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data vData(info, std::move(selector), 2, false);
    original->setVibrationData(std::move(vData));
    original->setLastVibrationFrequency(11);

    // 保存
    nlohmann::json data;
    original->save(data);

    // 加载
    auto loaded = std::make_unique<SculkSensorBlockEntity>(pos_);
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getLastVibrationFrequency(), 11);

    // 验证振动数据
    const auto& vibData = loaded->getVibrationData();
    ASSERT_NE(vibData.currentVibration(), nullptr);
    EXPECT_STREQ(vibData.currentVibration()->gameEvent->id(), "entity_damage");
    EXPECT_FLOAT_EQ(vibData.currentVibration()->distance, 3.0f);
    EXPECT_EQ(vibData.travelTimeInTicks(), 2);
    // 从存档加载时 reloadVibrationParticle 必须为 true
    EXPECT_TRUE(vibData.shouldReloadVibrationParticle());
}

// ============================================================================
// SculkSensorBlockEntity NBT 序列化测试
// ============================================================================

TEST_F(SculkSensorBlockEntityTest, NBTSerialization_EmptyData)
{
    auto entity = std::make_unique<SculkSensorBlockEntity>(pos_);

    nbt::CompoundTag tag;
    entity->saveToNBT(tag);

    // 验证 listener
    const nbt::CompoundTag* listenerTag = entity::serialization::nbt_helper::tryGetCompound(tag, "listener");
    ASSERT_NE(listenerTag, nullptr);

    // 验证频率
    auto freq = entity::serialization::nbt_helper::tryGetInt(tag, "last_vibration_frequency");
    ASSERT_TRUE(freq.has_value());
    EXPECT_EQ(freq.value(), 0);
}

TEST_F(SculkSensorBlockEntityTest, NBTRoundTrip)
{
    auto original = std::make_unique<SculkSensorBlockEntity>(pos_);

    GameEvent event("explode", 16);
    VibrationInfo info(event, 14.0f, Vector3d(50.0, 30.0, -10.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data vData(info, std::move(selector), 6, false);
    original->setVibrationData(std::move(vData));
    original->setLastVibrationFrequency(14);

    // 保存
    nbt::CompoundTag tag;
    original->saveToNBT(tag);

    // 加载
    auto loaded = std::make_unique<SculkSensorBlockEntity>(pos_);
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_EQ(loaded->getLastVibrationFrequency(), 14);

    const auto& vibData = loaded->getVibrationData();
    ASSERT_NE(vibData.currentVibration(), nullptr);
    EXPECT_STREQ(vibData.currentVibration()->gameEvent->id(), "explode");
    EXPECT_FLOAT_EQ(vibData.currentVibration()->distance, 14.0f);
    EXPECT_EQ(vibData.travelTimeInTicks(), 6);
    EXPECT_TRUE(vibData.shouldReloadVibrationParticle());
}

// ============================================================================
// SculkSensorBlockEntity Clone 测试
// ============================================================================

TEST_F(SculkSensorBlockEntityTest, ClonePreservesData)
{
    auto original = std::make_unique<SculkSensorBlockEntity>(pos_);
    original->setLastVibrationFrequency(7);

    GameEvent event("resonate_5", 16);
    VibrationInfo info(event, 2.0f, Vector3d(0, 0, 0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data vData(info, std::move(selector), 1, true);
    original->setVibrationData(std::move(vData));

    auto clone = original->clone();
    ASSERT_NE(clone, nullptr);

    auto* cloned = dynamic_cast<SculkSensorBlockEntity*>(clone.get());
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getLastVibrationFrequency(), 7);
    EXPECT_EQ(cloned->getPos(), pos_);
    ASSERT_NE(cloned->getVibrationData().currentVibration(), nullptr);
    EXPECT_STREQ(cloned->getVibrationData().currentVibration()->gameEvent->id(), "resonate_5");
}

// ============================================================================
// SculkShriekerBlockEntity 测试
// ============================================================================

class SculkShriekerBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { pos_ = BlockPos(5, -10, 100); }

    BlockPos pos_;
};

TEST_F(SculkShriekerBlockEntityTest, DefaultConstruction)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    EXPECT_EQ(entity->getType(), BlockEntityType::SculkShrieker);
    EXPECT_EQ(entity->getWarningLevel(), 0);
    EXPECT_FALSE(entity->canSummonWarden());
    EXPECT_EQ(entity->getPos(), pos_);
}

TEST_F(SculkShriekerBlockEntityTest, SetWarningLevelValues)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(1);
    EXPECT_EQ(entity->getWarningLevel(), 1);
    EXPECT_FALSE(entity->canSummonWarden());

    entity->setWarningLevel(2);
    EXPECT_EQ(entity->getWarningLevel(), 2);

    entity->setWarningLevel(3);
    EXPECT_EQ(entity->getWarningLevel(), 3);

    entity->setWarningLevel(4);
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityTest, WarningLevelMax)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 设置到最大值
    entity->setWarningLevel(4);
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityTest, SetWarningLevel)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    entity->setWarningLevel(2);
    EXPECT_EQ(entity->getWarningLevel(), 2);
    EXPECT_FALSE(entity->canSummonWarden());

    entity->setWarningLevel(4);
    EXPECT_EQ(entity->getWarningLevel(), 4);
    EXPECT_TRUE(entity->canSummonWarden());
}

TEST_F(SculkShriekerBlockEntityTest, VibrationDataAccess)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    // 设置振动数据
    GameEvent event("shriek", 32);
    VibrationInfo info(event, 15.0f, Vector3d(100.0, 0.0, -50.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data data(info, std::move(selector), 10, true);
    entity->setVibrationData(std::move(data));

    EXPECT_NE(entity->getVibrationData().currentVibration(), nullptr);
    EXPECT_EQ(entity->getVibrationData().travelTimeInTicks(), 10);
}

// ============================================================================
// SculkShriekerBlockEntity JSON 序列化测试
// ============================================================================

TEST_F(SculkShriekerBlockEntityTest, JsonSerialization_EmptyData)
{
    auto entity = std::make_unique<SculkShriekerBlockEntity>(pos_);

    nlohmann::json data;
    entity->save(data);

    EXPECT_TRUE(data.contains("listener"));
    EXPECT_TRUE(data["listener"].is_object());
    EXPECT_TRUE(data.contains("warning_level"));
    EXPECT_EQ(data["warning_level"], 0);
}

TEST_F(SculkShriekerBlockEntityTest, JsonRoundTrip)
{
    auto original = std::make_unique<SculkShriekerBlockEntity>(pos_);

    GameEvent event("shriek", 32);
    VibrationInfo info(event, 8.0f, Vector3d(10.0, 20.0, 30.0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data vData(info, std::move(selector), 5, false);
    original->setVibrationData(std::move(vData));

    // 设置警告等级到 3
    original->setWarningLevel(3);
    EXPECT_EQ(original->getWarningLevel(), 3);

    nlohmann::json data;
    original->save(data);

    auto loaded = std::make_unique<SculkShriekerBlockEntity>(pos_);
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getWarningLevel(), 3);
    EXPECT_FALSE(loaded->canSummonWarden());

    const auto& vibData = loaded->getVibrationData();
    ASSERT_NE(vibData.currentVibration(), nullptr);
    EXPECT_STREQ(vibData.currentVibration()->gameEvent->id(), "shriek");
    EXPECT_EQ(vibData.travelTimeInTicks(), 5);
    EXPECT_TRUE(vibData.shouldReloadVibrationParticle());
}

// ============================================================================
// SculkShriekerBlockEntity NBT 序列化测试
// ============================================================================

TEST_F(SculkShriekerBlockEntityTest, NBTRoundTrip)
{
    auto original = std::make_unique<SculkShriekerBlockEntity>(pos_);

    GameEvent event("shriek", 32);
    VibrationInfo info(event, 25.0f, Vector3d(0, 0, 0), nullptr);
    VibrationSelector selector;
    VibrationSystem::Data vData(info, std::move(selector), 12, false);
    original->setVibrationData(std::move(vData));

    // 设置到 4 级（可召唤监守者）
    original->setWarningLevel(4);

    nbt::CompoundTag tag;
    original->saveToNBT(tag);

    auto loaded = std::make_unique<SculkShriekerBlockEntity>(pos_);
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_EQ(loaded->getWarningLevel(), 4);
    EXPECT_TRUE(loaded->canSummonWarden());

    const auto& vibData = loaded->getVibrationData();
    ASSERT_NE(vibData.currentVibration(), nullptr);
    EXPECT_STREQ(vibData.currentVibration()->gameEvent->id(), "shriek");
    EXPECT_FLOAT_EQ(vibData.currentVibration()->distance, 25.0f);
    EXPECT_EQ(vibData.travelTimeInTicks(), 12);
}

// ============================================================================
// SculkShriekerBlockEntity Clone 测试
// ============================================================================

TEST_F(SculkShriekerBlockEntityTest, ClonePreservesData)
{
    auto original = std::make_unique<SculkShriekerBlockEntity>(pos_);
    original->setWarningLevel(2);

    auto clone = original->clone();
    ASSERT_NE(clone, nullptr);

    auto* cloned = dynamic_cast<SculkShriekerBlockEntity*>(clone.get());
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getWarningLevel(), 2);
    EXPECT_EQ(cloned->getPos(), pos_);
}
